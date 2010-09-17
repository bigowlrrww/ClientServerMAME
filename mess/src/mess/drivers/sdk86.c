/***************************************************************************

        Intel SDK-86

        12/05/2009 Skeleton driver by Micko
        29/11/2009 Some fleshing out by Lord Nightmare

    TODO:
    Add 8251A for serial
    Add 8279 for keypad reading and led display handling
    Add optional 2x 8255A

****************************************************************************/

#include "emu.h"
#include "cpu/i86/i86.h"

static ADDRESS_MAP_START(sdk86_mem, ADDRESS_SPACE_PROGRAM, 16)
	AM_RANGE(0x00000, 0x03fff) AM_RAM
	AM_RANGE(0xfe000, 0xfffff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( sdk86_io , ADDRESS_SPACE_IO, 16)
	ADDRESS_MAP_UNMAP_HIGH
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( sdk86 )
INPUT_PORTS_END


static MACHINE_RESET(sdk86)
{
}

static VIDEO_START( sdk86 )
{
}

static VIDEO_UPDATE( sdk86 )
{
    return 0;
}

static MACHINE_DRIVER_START( sdk86 )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",I8086, XTAL_14_7456MHz/3) /* divided down by i8284 clock generator; jumper selection allows it to be slowed to 2.5MHz, hence changing divider from 3 to 6 */
    MDRV_CPU_PROGRAM_MAP(sdk86_mem)
    MDRV_CPU_IO_MAP(sdk86_io)

    MDRV_MACHINE_RESET(sdk86)

    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(640, 480)
    MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)

    MDRV_VIDEO_START(sdk86)
    MDRV_VIDEO_UPDATE(sdk86)
MACHINE_DRIVER_END

/* ROM definition */
ROM_START( sdk86 )
	ROM_REGION( 0x100000, "maincpu", ROMREGION_ERASEFF ) // all are Intel D2616 ?eproms with the windows painted over? (factory programmed eproms? this would match the 'i8642' marking on the factory programmed eprom version of the AT keyboard mcu...)
	// Note that the rom pairs at FE000-FEFFF and FF000-FFFFF are interchangable; the ones at FF000-FFFFF are the ones which start on bootup, and the other ones live at FE000-FEFFF and can be switched in by the user. One pair is the Serial RS232 Monitor and the other is the Keypad/front panel monitor. On the SDK-86 I (LN) dumped, the Keypad monitor was primary, but the other SDK-86 I know of has the roms in the opposite arrangement (Serial primary).
	// Serial Monitor Version 1.2 (says "  86   1.2" on LED display at startup, and sends a data prompt over serial)
	ROM_LOAD16_BYTE( "0456_104531-001.a36", 0xfe000, 0x0800, CRC(f9c4a809) SHA1(aea324c3f52dd393f1eed2b856ba11f050a35b93)) /* Label: "iD2616 // T142099WS // (C)INTEL '77 // 0456 // 104531-001" */
	ROM_LOAD16_BYTE( "0457_104532-001.a37", 0xfe001, 0x0800, CRC(a245ba5c) SHA1(7f67277f866fca5377cb123e9cc405b5fdfe61d3)) /* Label: "iD2616 // T145054WS // (C)INTEL '77 // 0457 // 104532-001" */
	// Keypad Monitor Version 1.1 (says "- 86   1.1" on LED display at startup)
	ROM_LOAD16_BYTE( "0169_102042-001.a27", 0xff000, 0x0800, CRC(3f46311a) SHA1(a97e6861b736f26230b9adbf5cd2576a9f60d626)) /* Label: "iD2616 // T142094WS // (C)INTEL '77 // 0169 // 102042-001" */
	ROM_LOAD16_BYTE( "0170_102043-001.a30", 0xff001, 0x0800, CRC(65924471) SHA1(5d258695bf585f89179dfa0a113a0eeeabd5ee2b)) /* Label: "iD2616 // T145056WS // (C)INTEL '77 // 0170 // 102043-001" */

	/* proms:
     * dumped 11/21/09 thru 11/29/09 by LN
     * purposes: (according to sdk-86 user manual from http://www.bitsavers.org/pdf/intel/8086/9800698A_SDK-86_Users_Man_Apr79.pdf)
     * A12: main address decoding (selects ram or rom or open bus/offboard, see page 2-7)
     * A22: I/O decoding for 8251, 8279 and optional pair of 8255 chips (in the FFE8-FFFF I/O area; see page 2-6)
     * A26: ROM address decoding for selecting which of the 4 pairs of roms is active (note that to use the FCxxx and FDxxx pairs requires wiring them into the prototype area, they are not standard; see page 2-5)
     * A29: RAM address decoding (see page 2-4)
     */
	ROM_REGION(0x1000, "proms", 0 ) // all are Intel D3625A 1kx4 (82s137A equivalent)
	ROM_LOAD( "0036_101993-001.a12", 0x0000, 0x0400, CRC(bb7edbfd) SHA1(8847f9815c7cb8695986743199673920a7d4390d)) /* Label: "iD3625A 0036 // 8142 // 101993-001" */
	ROM_LOAD( "0035_101992-001.a22", 0x0400, 0x0400, CRC(76aced0c) SHA1(89fa39473e19d8cb6b65d6430d3d683ae2398fb3)) /* Label: "iD3625A 0035 // 8142 // 101992-001" */
	ROM_LOAD( "0037_101994-001.a26", 0x0800, 0x0400, CRC(d6f33d30) SHA1(41e794bf202266fa57516403e6a80ebbf6c95fdc)) /* Label: "iD3625A 0037 // 8142 // 101994-001" */
	ROM_LOAD( "0038_101995-001.a29", 0x0C00, 0x0400, CRC(3d2c18bc) SHA1(5e1935cd07fef26b2cf3d8fa7612fe0d8e678c06)) /* Label: "iD3625A 0038 // 8142 // 101995-001" */
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    COMPANY   FULLNAME       FLAGS */
COMP( 1979, sdk86,  0,       0, 	sdk86,	sdk86,	 0, 		 "Intel",   "SDK-86",		GAME_NOT_WORKING | GAME_NO_SOUND)
