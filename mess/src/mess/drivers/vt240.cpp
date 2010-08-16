/***************************************************************************

        DEC VT240

        31/03/2010 Skeleton driver.


    mc7105 - failing the rom check. If you skip that, it is waiting on i/o.

****************************************************************************/

#include "emu.h"
#include "cpu/i8085/i8085.h"
#include "cpu/t11/t11.h"
#include "devices/messram.h"

static ADDRESS_MAP_START(vt240_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x7fff) AM_ROM
	AM_RANGE(0x8000, 0x87ff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( vt240_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( vt240 )
INPUT_PORTS_END


static MACHINE_RESET(vt240)
{
}

static VIDEO_START( vt240 )
{
}

static VIDEO_UPDATE( vt240 )
{
	return 0;
}

static MACHINE_DRIVER_START( vt240 )
	/* basic machine hardware */
	MDRV_CPU_ADD("maincpu", I8085A, XTAL_16MHz / 4)
	MDRV_CPU_PROGRAM_MAP(vt240_mem)
	MDRV_CPU_IO_MAP(vt240_io)

	MDRV_MACHINE_RESET(vt240)

	/* video hardware */
	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_REFRESH_RATE(50)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(640, 480)
	MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
	MDRV_PALETTE_LENGTH(2)
	MDRV_PALETTE_INIT(black_and_white)

	MDRV_VIDEO_START(vt240)
	MDRV_VIDEO_UPDATE(vt240)
MACHINE_DRIVER_END

/* ROM definition */
ROM_START( mc7105 )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "027.bin", 0x8000, 0x8000, CRC(a159b412) SHA1(956097ccc2652d494258b3682498cfd3096d7d4f))
	ROM_LOAD( "028.bin", 0x0000, 0x8000, CRC(b253151f) SHA1(22ffeef8eb5df3c38bfe91266f26d1e7822cdb53))
	ROM_FILL( 0x006e, 3, 0 )	// hack to skip the failing rom test

	ROM_REGION( 0x20000, "subcpu", ROMREGION_ERASEFF )
	ROM_LOAD16_BYTE( "029.bin", 0x00000, 0x8000, CRC(4a6db217) SHA1(47637325609ea19ffab61fe31e2700d72fa50729))
	ROM_LOAD16_BYTE( "031.bin", 0x00001, 0x8000, CRC(47129579) SHA1(39de9e2e26f90c5da5e72a09ff361c1a94b9008a))

	ROM_LOAD16_BYTE( "030.bin", 0x10000, 0x8000, CRC(05fd7b75) SHA1(2ad8c14e76accfa1b9b8748c58e9ebbc28844a47))
	ROM_LOAD16_BYTE( "032.bin", 0x10001, 0x8000, CRC(e81d93c4) SHA1(982412a7a6e65d6f6b4f66bd093e54ee16f31384))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    CONFIG COMPANY   FULLNAME       FLAGS */
//COMP( 1983, vt240,  0,       0,   vt220,  vt220,   0,          "DEC",   "VT240",      GAME_NOT_WORKING | GAME_NO_SOUND)
//COMP( 1983, vt241,  0,       0,   vt220,  vt220,   0,          "DEC",   "VT241",      GAME_NOT_WORKING | GAME_NO_SOUND)
COMP( 1983, mc7105,  0,       0,	vt240,	vt240,	 0, 		 "Elektronika",   "MC7105",		GAME_NOT_WORKING | GAME_NO_SOUND)
