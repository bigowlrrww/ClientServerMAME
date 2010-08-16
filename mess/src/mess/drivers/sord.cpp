/******************************************************************************

    drivers/sord.c

    Sord m5 system driver

    Thankyou to Roman Stec and Jan P. Naidr for the documentation and much
    help.

    http://falabella.lf2.cuni.cz/~naidr/sord/

    PI-5 is the parallel interface using a 8255.
    FD-5 is the disc operating system and disc interface.
    FD-5 is connected to M5 via PI-5.


    Kevin Thacker [MESS driver]


    TODO:

        - There are 3 different 64K RAM expansions, Masterchess checks
          for all of them
        - Serial interface SI-5
        - Floppy interface ROM isn't dumped
        - Interrupts are wrong

 ******************************************************************************/

#include "emu.h"
#include "video/tms9928a.h"
#include "sound/sn76496.h"
#include "cpu/z80/z80.h"
#include "cpu/z80/z80daisy.h"
#include "machine/ctronics.h"
#include "machine/z80ctc.h"
#include "devices/cartslot.h"
#include "devices/cassette.h"
#include "formats/sord_cas.h"

/* FD-5 floppy support */
#include "machine/8255ppi.h"
#include "devices/flopdrv.h"
#include "formats/basicdsk.h"
#include "machine/upd765.h"


#define SORD_DEBUG 1
#define LOG(x) do { if (SORD_DEBUG) logerror x; } while (0)

/*********************************************************************************************/
/* FD5 disk interface */
/* - Z80 CPU */
/* - 27128 ROM (16K) */
/* - 2x6116 RAM */
/* - Intel8272/UPD765 */
/* - IRQ of UPD765 is connected to INT of Z80 */
/* PI-5 interface is required. mode 2 of the 8255 is used to communicate with the FD-5 */


class sord_state
{
public:
	static void *alloc(running_machine &machine) { return auto_alloc_clear(&machine, sord_state(machine)); }

	sord_state(running_machine &machine) { }

	UINT8 fd5_databus;
	int fd5_port_0x020_data;
	int obfa;
	int ibfa;
	int intra;
};

static MACHINE_RESET( sord_m5 );

static ADDRESS_MAP_START( sord_fd5_mem , ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0x03fff) AM_ROM	/* internal rom */
	AM_RANGE(0x4000, 0x0ffff) AM_RAM
ADDRESS_MAP_END


/* stb and ack automatically set on read/write? */
static WRITE8_HANDLER(fd5_communication_w)
{
	sord_state *state = (sord_state *)space->machine->driver_data;

	cpu_yield(space->cpu);

	state->fd5_port_0x020_data = data;
	LOG(("fd5 0x020: %02x %04x\n",data,cpu_get_pc(space->cpu)));
}

static  READ8_HANDLER(fd5_communication_r)
{
	sord_state *state = (sord_state *)space->machine->driver_data;
	int data;

	cpu_yield(space->cpu);

	data = (state->obfa<<3)|(state->ibfa<<2)|2;
	LOG(("fd5 0x030: %02x %04x\n",data, cpu_get_pc(space->cpu)));

	return data;
}

static READ8_HANDLER(fd5_data_r)
{
	sord_state *state = (sord_state *)space->machine->driver_data;

	cpu_yield(space->cpu);

	LOG(("fd5 0x010 r: %02x %04x\n",state->fd5_databus,cpu_get_pc(space->cpu)));

	ppi8255_set_port_c(space->machine->device("ppi8255"), 0x50);
	ppi8255_set_port_c(space->machine->device("ppi8255"), 0x10);
	ppi8255_set_port_c(space->machine->device("ppi8255"), 0x50);

	return state->fd5_databus;
}

static WRITE8_HANDLER(fd5_data_w)
{
	sord_state *state = (sord_state *)space->machine->driver_data;

	LOG(("fd5 0x010 w: %02x %04x\n",data,cpu_get_pc(space->cpu)));

	state->fd5_databus = data;

	/* set stb on data write */
	ppi8255_set_port_c(space->machine->device("ppi8255"), 0x50);
	ppi8255_set_port_c(space->machine->device("ppi8255"), 0x40);
	ppi8255_set_port_c(space->machine->device("ppi8255"), 0x50);

	cpu_yield(space->cpu);
}

static WRITE8_HANDLER( fd5_drive_control_w )
{
	int state;

	if (data==0)
		state = 0;
	else
		state = 1;

	LOG(("fd5 drive state w: %02x\n",state));

	floppy_mon_w(floppy_get_device(space->machine, 0), !state);
	floppy_mon_w(floppy_get_device(space->machine, 1), !state);
	floppy_drive_set_ready_state(floppy_get_device(space->machine, 0), 1,1);
	floppy_drive_set_ready_state(floppy_get_device(space->machine, 1), 1,1);
}

static WRITE8_HANDLER( fd5_tc_w )
{
	running_device *fdc = space->machine->device("upd765");
	upd765_tc_w(fdc, 1);
	upd765_tc_w(fdc, 0);
}

/* 0x020 fd5 writes to this port to communicate with m5 */
/* 0x010 data bus to read/write from m5 */
/* 0x030 fd5 reads from this port to communicate with m5 */
/* 0x040 */
/* 0x050 */
static ADDRESS_MAP_START(sord_fd5_io, ADDRESS_SPACE_IO, 8)
	AM_RANGE(0x000, 0x000) AM_DEVREAD( "upd765", upd765_status_r)
	AM_RANGE(0x001, 0x001) AM_DEVREADWRITE("upd765", upd765_data_r, upd765_data_w)
	AM_RANGE(0x010, 0x010) AM_READWRITE(fd5_data_r, fd5_data_w)
	AM_RANGE(0x020, 0x020) AM_WRITE(fd5_communication_w)
	AM_RANGE(0x030, 0x030) AM_READ(fd5_communication_r)
	AM_RANGE(0x040, 0x040) AM_WRITE(fd5_drive_control_w)
	AM_RANGE(0x050, 0x050) AM_WRITE(fd5_tc_w)
ADDRESS_MAP_END

/* upd765 data request is connected to interrupt of z80 inside fd5 interface */
static WRITE_LINE_DEVICE_HANDLER( sord_fd5_fdc_interrupt )
{
	cputag_set_input_line(device->machine, "floppy", 0, state? HOLD_LINE : CLEAR_LINE);
}

static const struct upd765_interface sord_fd5_upd765_interface=
{
	DEVCB_LINE(sord_fd5_fdc_interrupt),
	NULL,
	NULL,
	UPD765_RDY_PIN_CONNECTED,
	{FLOPPY_0, FLOPPY_1, NULL, NULL}
};

static MACHINE_RESET( sord_m5_fd5 )
{
	MACHINE_RESET_CALL(sord_m5);
	ppi8255_set_port_c(machine->device("ppi8255"), 0x50);
}


/*********************************************************************************************/
/* PI-5 */

static READ8_DEVICE_HANDLER(sord_ppi_porta_r)
{
	sord_state *state = (sord_state *)device->machine->driver_data;

	cpu_yield(device->machine->device("maincpu"));

	return state->fd5_databus;
}

static READ8_DEVICE_HANDLER(sord_ppi_portb_r)
{
	cpu_yield(device->machine->device("maincpu"));

	LOG(("m5 read from pi5 port b %04x\n", cpu_get_pc(device->machine->device("maincpu"))));

	return 0x0ff;
}

static READ8_DEVICE_HANDLER(sord_ppi_portc_r)
{
	sord_state *state = (sord_state *)device->machine->driver_data;

	cpu_yield(device->machine->device("maincpu"));

	LOG(("m5 read from pi5 port c %04x\n", cpu_get_pc(device->machine->device("maincpu"))));

/* from fd5 */
/* 00 = 0000 = write */
/* 02 = 0010 = write */
/* 06 = 0110 = read */
/* 04 = 0100 = read */

/* m5 expects */
/*00 = READ */
/*01 = POTENTIAL TO READ BUT ALSO RESET */
/*10 = WRITE */
/*11 = FORCE RESET AND WRITE */

	/* FD5 bit 0 -> M5 bit 2 */
	/* FD5 bit 2 -> M5 bit 1 */
	/* FD5 bit 1 -> M5 bit 0 */
	return (
			/* FD5 bit 0-> M5 bit 2 */
			((state->fd5_port_0x020_data & 0x01)<<2) |
			/* FD5 bit 2-> M5 bit 1 */
			((state->fd5_port_0x020_data & 0x04)>>1) |
			/* FD5 bit 1-> M5 bit 0 */
			((state->fd5_port_0x020_data & 0x02)>>1)
			);
}

static WRITE8_DEVICE_HANDLER(sord_ppi_porta_w)
{
	sord_state *state = (sord_state *)device->machine->driver_data;

	cpu_yield(device->machine->device("maincpu"));

	state->fd5_databus = data;
}

static WRITE8_DEVICE_HANDLER(sord_ppi_portb_w)
{
	cpu_yield(device->machine->device("maincpu"));

	/* f0, 40 */
	/* 1111 */
	/* 0100 */

	if (data == 0x0f0)
	{
		cputag_set_input_line(device->machine, "floppy", INPUT_LINE_RESET, ASSERT_LINE);
		cputag_set_input_line(device->machine, "floppy", INPUT_LINE_RESET, CLEAR_LINE);
	}
	LOG(("m5 write to pi5 port b: %02x %04x\n", data, cpu_get_pc(device->machine->device("maincpu"))));
}

/* A,  B,  C,  D,  E,   F,  G,  H,  I,  J, K,  L,  M,   N, O, P, Q, R,   */
/* 41, 42, 43, 44, 45, 46, 47, 48, 49, 4a, 4b, 4c, 4d, 4e, 4f, 50, 51, 52*/

/* C,H,N */


static WRITE8_DEVICE_HANDLER(sord_ppi_portc_w)
{
	sord_state *state = (sord_state *)device->machine->driver_data;

	state->obfa = (data & 0x80) ? 1 : 0;
	state->intra = (data & 0x08) ? 1 : 0;
	state->ibfa = (data & 0x20) ? 1 : 0;

	cpu_yield(device->machine->device("maincpu"));
	LOG(("m5 write to pi5 port c: %02x %04x\n", data, cpu_get_pc(device->machine->device("maincpu"))));
}

static const ppi8255_interface sord_ppi8255_interface =
{
	DEVCB_HANDLER(sord_ppi_porta_r),
	DEVCB_HANDLER(sord_ppi_portb_r),
	DEVCB_HANDLER(sord_ppi_portc_r),
	DEVCB_HANDLER(sord_ppi_porta_w),
	DEVCB_HANDLER(sord_ppi_portb_w),
	DEVCB_HANDLER(sord_ppi_portc_w)
};



/***************************************************************************
    MACHINE EMULATION
***************************************************************************/

static void sordm5_video_interrupt_callback(running_machine *machine, int state)
{
	if (state)
	{
		z80ctc_trg3_w(machine->device("z80ctc"), 1);
		z80ctc_trg3_w(machine->device("z80ctc"), 0);
	}
}

static INTERRUPT_GEN( sord_interrupt )
{
	if (TMS9928A_interrupt(device->machine))
		cputag_set_input_line(device->machine, "maincpu", 0, HOLD_LINE);
}

/* read */
/* bit 0 is cassette read data */
/* bit 1 is printer busy */
/* bit 7 is the reset/halt key */
static READ8_HANDLER( sord_sts_r )
{
	running_device *printer = space->machine->device("centronics");
	running_device *cassette = space->machine->device("cassette");
	UINT8 data = 0;

	data |= cassette_input(cassette) >= 0 ? 1 : 0;
	data |= centronics_busy_r(printer) << 1;
	data |= input_port_read(space->machine, "reset");

	logerror("sts read: %02x\n", data);

	return data;
}

/* write */
/* bit 0 is strobe to printer or cassette write data */
/* bit 1 is cassette remote */
static WRITE8_HANDLER( sord_com_w )
{
	running_device *printer = space->machine->device("centronics");
	running_device *cassette = space->machine->device("cassette");

	/* cassette data */
	cassette_output(cassette, BIT(data, 0) ? -1.0 : 1.0);

	centronics_strobe_w(printer, BIT(data, 0));

	/* cassette remote */
	cassette_change_state(cassette,	BIT(data, 1) ? CASSETTE_MOTOR_ENABLED : CASSETTE_MOTOR_DISABLED,
		CASSETTE_MASK_MOTOR);

	logerror("com write: %02x\n",data);
}


/***************************************************************************
    ADDRESS MAPS
***************************************************************************/

static ADDRESS_MAP_START( sord_m5_mem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x1fff) AM_ROM	/* internal rom */
	AM_RANGE(0x2000, 0x6fff) AM_ROM /* cartridge */
	AM_RANGE(0x7000, 0x7fff) AM_RAM /* internal ram */
	AM_RANGE(0x8000, 0xffff) AM_RAM /* 32k expand box */
ADDRESS_MAP_END

static ADDRESS_MAP_START( sord_m5_io, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0x00, 0x03) AM_MIRROR(0x0c) AM_DEVREADWRITE("z80ctc", z80ctc_r, z80ctc_w)
	AM_RANGE(0x10, 0x10) AM_MIRROR(0x0e) AM_READWRITE(TMS9928A_vram_r, TMS9928A_vram_w)
	AM_RANGE(0x11, 0x11) AM_MIRROR(0x0e) AM_READWRITE(TMS9928A_register_r, TMS9928A_register_w)
	AM_RANGE(0x20, 0x20) AM_MIRROR(0x0f) AM_DEVWRITE("sn76489a", sn76496_w)
	AM_RANGE(0x30, 0x30) AM_READ_PORT("keyboard_row_0")
	AM_RANGE(0x31, 0x31) AM_READ_PORT("keyboard_row_1")
	AM_RANGE(0x32, 0x32) AM_READ_PORT("keyboard_row_2")
	AM_RANGE(0x33, 0x33) AM_READ_PORT("keyboard_row_3")
	AM_RANGE(0x34, 0x34) AM_READ_PORT("keyboard_row_4")
	AM_RANGE(0x35, 0x35) AM_READ_PORT("keyboard_row_5")
	AM_RANGE(0x36, 0x36) AM_READ_PORT("keyboard_row_6")
	AM_RANGE(0x37, 0x37) AM_READ_PORT("joypad_direction")
	AM_RANGE(0x40, 0x40) AM_MIRROR(0x0f) AM_DEVWRITE("centronics", centronics_data_w)
	AM_RANGE(0x50, 0x50) AM_MIRROR(0x0f) AM_READWRITE(sord_sts_r, sord_com_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( srdm5fd5_io, ADDRESS_SPACE_IO, 8 )
	AM_IMPORT_FROM(sord_m5_io)
	AM_RANGE(0x70, 0x73) AM_MIRROR(0x0c) AM_DEVREADWRITE("ppi8255", ppi8255_r, ppi8255_w)
ADDRESS_MAP_END


/***************************************************************************
    INPUT PORTS
***************************************************************************/

/* 2008-05 FP:
Small note about natural keyboard: currently
- "Reset" is mapped to 'Esc'
- "Func" is mapped to 'F1'
*/
static INPUT_PORTS_START( sord_m5 )
	PORT_START("keyboard_row_0")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Ctrl") PORT_CODE(KEYCODE_LCONTROL) PORT_CHAR(UCHAR_SHIFT_2)
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Func") PORT_CODE(KEYCODE_TAB) PORT_CHAR(UCHAR_MAMEKEY(F1))
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_LSHIFT)		PORT_CHAR(UCHAR_SHIFT_1)
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_RSHIFT)		PORT_CHAR(UCHAR_MAMEKEY(RSHIFT))
	PORT_BIT(0x10, 0x00, IPT_UNUSED)
	PORT_BIT(0x20, 0x00, IPT_UNUSED)
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_SPACE)		PORT_CHAR(' ')
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_ENTER)		PORT_CHAR(13)

	PORT_START("keyboard_row_1")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_1)			PORT_CHAR('1') PORT_CHAR('!')
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_2)			PORT_CHAR('2') PORT_CHAR('"')
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_3)			PORT_CHAR('3') PORT_CHAR('#')
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_4)			PORT_CHAR('4') PORT_CHAR('$')
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_5)			PORT_CHAR('5') PORT_CHAR('%')
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_6)			PORT_CHAR('6') PORT_CHAR('&')
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_7)			PORT_CHAR('7') PORT_CHAR('\'')
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_8)			PORT_CHAR('8') PORT_CHAR('(')

	PORT_START("keyboard_row_2")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_Q)			PORT_CHAR('q') PORT_CHAR('Q')
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_W)			PORT_CHAR('w') PORT_CHAR('W')
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_E)			PORT_CHAR('e') PORT_CHAR('E')
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_R)			PORT_CHAR('r') PORT_CHAR('R')
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_T)			PORT_CHAR('t') PORT_CHAR('T')
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_Y)			PORT_CHAR('y') PORT_CHAR('Y')
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_U)			PORT_CHAR('u') PORT_CHAR('U')
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_I)			PORT_CHAR('i') PORT_CHAR('I')

	PORT_START("keyboard_row_3")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_A)			PORT_CHAR('a') PORT_CHAR('A')
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_S)			PORT_CHAR('s') PORT_CHAR('S')
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_D)			PORT_CHAR('d') PORT_CHAR('D')
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_F)			PORT_CHAR('f') PORT_CHAR('F')
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_G)			PORT_CHAR('g') PORT_CHAR('G')
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_H)			PORT_CHAR('h') PORT_CHAR('H')
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_J)			PORT_CHAR('j') PORT_CHAR('J')
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_K)			PORT_CHAR('k') PORT_CHAR('K')

	PORT_START("keyboard_row_4")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_Z)			PORT_CHAR('z') PORT_CHAR('Z')
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_X)			PORT_CHAR('x') PORT_CHAR('X')
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_C)			PORT_CHAR('c') PORT_CHAR('C')
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_V)			PORT_CHAR('v') PORT_CHAR('V')
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_B)			PORT_CHAR('b') PORT_CHAR('B')
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_N)			PORT_CHAR('n') PORT_CHAR('N')
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_M)			PORT_CHAR('m') PORT_CHAR('M')
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_COMMA)		PORT_CHAR(',') PORT_CHAR('<')

	PORT_START("keyboard_row_5")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_9)			PORT_CHAR('9') PORT_CHAR(')')
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_0)			PORT_CHAR('0')
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_MINUS)		PORT_CHAR('-') PORT_CHAR('=')
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_EQUALS)	PORT_CHAR('^') PORT_CHAR('~')
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_STOP)		PORT_CHAR('.') PORT_CHAR('>')
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_SLASH)		PORT_CHAR('/') PORT_CHAR('?') PORT_CHAR(UCHAR_MAMEKEY(DOWN))
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("_  Triangle") PORT_CODE(KEYCODE_TILDE) PORT_CHAR('_')
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_BACKSLASH2)	PORT_CHAR('\\') PORT_CHAR('|')

	PORT_START("keyboard_row_6")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_O)			PORT_CHAR('o') PORT_CHAR('O')
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_P)			PORT_CHAR('p') PORT_CHAR('P')
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_OPENBRACE)	PORT_CHAR('@') PORT_CHAR('`') PORT_CHAR(UCHAR_MAMEKEY(UP))
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_CLOSEBRACE)	PORT_CHAR('[') PORT_CHAR('{')
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_L)			PORT_CHAR('l') PORT_CHAR('L')
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_COLON)		PORT_CHAR(';') PORT_CHAR('+') PORT_CHAR(UCHAR_MAMEKEY(LEFT))
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_QUOTE)		PORT_CHAR(':') PORT_CHAR('*') PORT_CHAR(UCHAR_MAMEKEY(RIGHT))
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_BACKSLASH)	PORT_CHAR(']') PORT_CHAR('}')

	PORT_START("joypad_direction")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT) PORT_PLAYER(1)
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP)    PORT_PLAYER(1)
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT)  PORT_PLAYER(1)
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN)  PORT_PLAYER(1)
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT) PORT_PLAYER(2)
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP)    PORT_PLAYER(2)
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT)  PORT_PLAYER(2)
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN)  PORT_PLAYER(2)

	PORT_START("reset")
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Reset") PORT_CODE(KEYCODE_ESC) PORT_CHAR(UCHAR_MAMEKEY(ESC)) /* 1st line, 1st key from right! */
INPUT_PORTS_END


/***************************************************************************
    MACHINE DRIVERS
***************************************************************************/

static const z80_daisy_config sord_m5_daisy_chain[] =
{
	{ "z80ctc" },
	{ NULL }
};

static Z80CTC_INTERFACE( sord_m5_ctc_intf )
{
	0,
	DEVCB_CPU_INPUT_LINE("maincpu", INPUT_LINE_IRQ0),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL
};

static const cassette_config sordm5_cassette_config =
{
	sordm5_cassette_formats,
	NULL,
	(cassette_state)(CASSETTE_PLAY),
	NULL
};

static const TMS9928a_interface tms9928a_interface =
{
	TMS9929A,
	0x4000,
	0, 0,
	sordm5_video_interrupt_callback
};


static DEVICE_IMAGE_LOAD( sord_cart )
{
	UINT32 size;
	UINT8 *ptr = memory_region(image.device().machine, "maincpu");
	
	if (image.software_entry() == NULL)
	{
		size = image.length();
		if (image.fread(ptr + 0x2000, size) != size)
			return IMAGE_INIT_FAIL;
	}
	else
	{
		size = image.get_software_region_length("rom");
		memcpy(ptr + 0x2000, image.get_software_region("rom"), size);
	}
	
	return IMAGE_INIT_PASS;
}

static MACHINE_START( sord_m5 )
{
	TMS9928A_configure(&tms9928a_interface);
}

static MACHINE_RESET( sord_m5 )
{
	TMS9928A_reset();
}


static MACHINE_DRIVER_START( sord_m5 )

	MDRV_DRIVER_DATA( sord_state )

	/* basic machine hardware */
	MDRV_CPU_ADD("maincpu", Z80, XTAL_14_31818MHz/4)
	MDRV_CPU_PROGRAM_MAP(sord_m5_mem)
	MDRV_CPU_IO_MAP(sord_m5_io)
	MDRV_CPU_VBLANK_INT("screen", sord_interrupt)
	MDRV_CPU_CONFIG(sord_m5_daisy_chain)

	MDRV_MACHINE_START(sord_m5)
	MDRV_MACHINE_RESET(sord_m5)

	/* video hardware */
	MDRV_IMPORT_FROM(tms9928a)
	MDRV_SCREEN_MODIFY("screen")
	MDRV_SCREEN_REFRESH_RATE(50)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD("sn76489a", SN76489A, XTAL_14_31818MHz/4)	/* 3.579545 MHz */
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)

	MDRV_Z80CTC_ADD("z80ctc", XTAL_14_31818MHz/4, sord_m5_ctc_intf)

	/* printer */
	MDRV_CENTRONICS_ADD("centronics", standard_centronics)

	MDRV_CASSETTE_ADD("cassette", sordm5_cassette_config)

	/* cartridge */
	MDRV_CARTSLOT_ADD("cart")
	MDRV_CARTSLOT_EXTENSION_LIST("bin,rom")
	MDRV_CARTSLOT_MANDATORY
	MDRV_CARTSLOT_LOAD(sord_cart)
	MDRV_CARTSLOT_INTERFACE("m5_cart")

	/* software lists */
	MDRV_SOFTWARE_LIST_ADD("cart_list","sordm5")

MACHINE_DRIVER_END


static FLOPPY_OPTIONS_START( sordm5 )
	FLOPPY_OPTION( sordm5, "dsk", "Sord M5 disk image", basicdsk_identify_default, basicdsk_construct_default,
		HEADS([1])
		TRACKS([40])
		SECTORS([18])
		SECTOR_LENGTH([256])
		FIRST_SECTOR_ID([1]))
FLOPPY_OPTIONS_END

static const floppy_config sordm5_floppy_config =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	FLOPPY_STANDARD_5_25_SSDD_40,
	FLOPPY_OPTIONS_NAME(sordm5),
	NULL
};

static MACHINE_DRIVER_START( sord_m5_fd5 )
	MDRV_IMPORT_FROM( sord_m5 )

	MDRV_CPU_REPLACE("maincpu", Z80, XTAL_14_31818MHz/4)
	MDRV_CPU_IO_MAP(srdm5fd5_io)

	/* floppy */
	MDRV_CPU_ADD("floppy", Z80, XTAL_14_31818MHz/4)
	MDRV_CPU_PROGRAM_MAP(sord_fd5_mem)
	MDRV_CPU_IO_MAP(sord_fd5_io)

	MDRV_PPI8255_ADD("ppi8255", sord_ppi8255_interface)
	MDRV_UPD765A_ADD("upd765", sord_fd5_upd765_interface)

	MDRV_QUANTUM_TIME(HZ(1200))
	MDRV_MACHINE_RESET(sord_m5_fd5)

	MDRV_FLOPPY_4_DRIVES_ADD(sordm5_floppy_config)

	MDRV_CARTSLOT_MODIFY("cart")
	MDRV_CARTSLOT_NOT_MANDATORY
MACHINE_DRIVER_END


/***************************************************************************
    ROM DEFINITIONS
***************************************************************************/

ROM_START( sordm5 )
	ROM_REGION(0x10000, "maincpu", 0)
	ROM_SYSTEM_BIOS(0, "int", "International")
	ROMX_LOAD("sordint.rom", 0x0000, 0x2000, CRC(78848d39) SHA1(ac042c4ae8272ad6abe09ae83492ef9a0026d0b2),ROM_BIOS(1))
	ROM_SYSTEM_BIOS(1, "jap", "Japanese")
    ROMX_LOAD("sordjap.rom", 0x0000, 0x2000, CRC(92cf9353) SHA1(b0a4b3658fde68cb1f344dfb095bac16a78e9b3e),ROM_BIOS(2))	
ROM_END


ROM_START( sordm5fd5 )
	ROM_REGION(0x10000, "maincpu", 0)
	ROM_LOAD("sordint.rom",0x0000, 0x2000, CRC(78848d39) SHA1(ac042c4ae8272ad6abe09ae83492ef9a0026d0b2))

	ROM_REGION(0x4000, "floppy", 0)
	ROM_LOAD("sordfd5.rom",0x0000, 0x4000, NO_DUMP)
ROM_END


/***************************************************************************
    SYSTEM CONFIG
***************************************************************************/
/* different ram sizes need to be emulated */
#ifdef UNUSED_FUNCTION
	4K
	36K
#endif

/***************************************************************************
    GAME DRIVERS
***************************************************************************/

/*    YEAR  NAME       PARENT  COMPAT  MACHINE      INPUT    INIT  COMPANY  FULLNAME               FLAGS */
COMP( 1983, sordm5,    0,      0,      sord_m5,	    sord_m5, 0,    "Sord",  "Sord M5",             0 )
COMP( 1983, sordm5fd5, sordm5, 0,      sord_m5_fd5, sord_m5, 0,    "Sord",  "Sord M5 + PI5 + FD5", GAME_NOT_WORKING )