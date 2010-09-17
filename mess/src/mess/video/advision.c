/***************************************************************************

  video/advision.c

  Routines to control the Adventurevision video hardware

  Video hardware is composed of a vertical array of 40 LEDs which is
  reflected off a spinning mirror, to give a resolution of 150 x 40 at 15 FPS.

***************************************************************************/

#include "emu.h"
#include "includes/advision.h"

/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/
VIDEO_START( advision )
{
	advision_state *state = machine->driver_data<advision_state>();

	state->video_hpos = 0;
	state->display = auto_alloc_array(machine, UINT8, 8 * 8 * 256);
	memset(state->display, 0, 8 * 8 * 256);
}

/***************************************************************************

  Initialise the palette.

***************************************************************************/

PALETTE_INIT( advision )
{
	int i;

	for( i = 0; i < 8; i++ )
	{
		/* 8 shades of RED */
		palette_set_color_rgb(machine, i, i * 0x22, 0x00, 0x00);
	}
}

/***************************************************************************

  Update the display data.

***************************************************************************/

void advision_vh_write(running_machine *machine, int data)
{
	advision_state *state = machine->driver_data<advision_state>();

	if (state->video_bank >= 1 && state->video_bank <=5)
	{
		state->led_latch[state->video_bank] = data;
	}
}

void advision_vh_update(running_machine *machine, int x)
{
	advision_state *state = machine->driver_data<advision_state>();

	UINT8 *dst = &state->display[x];
	int y;

	for( y = 0; y < 8; y++ )
	{
		UINT8 data = state->led_latch[7-y];

		if( (data & 0x80) == 0 ) dst[0 * 256] = 8;
		if( (data & 0x40) == 0 ) dst[1 * 256] = 8;
		if( (data & 0x20) == 0 ) dst[2 * 256] = 8;
		if( (data & 0x10) == 0 ) dst[3 * 256] = 8;
		if( (data & 0x08) == 0 ) dst[4 * 256] = 8;
		if( (data & 0x04) == 0 ) dst[5 * 256] = 8;
		if( (data & 0x02) == 0 ) dst[6 * 256] = 8;
		if( (data & 0x01) == 0 ) dst[7 * 256] = 8;

		state->led_latch[7-y] = 0xff;

		dst += 8 * 256;
	}
}


/***************************************************************************

  Refresh the video screen

***************************************************************************/

VIDEO_UPDATE( advision )
{
	advision_state *state = screen->machine->driver_data<advision_state>();

	int x, y;

	static int framecount = 0;

	if( (framecount++ % 4) == 0 )
	{
		state->frame_start = 1;
		state->video_hpos = 0;
	}

	for (x = 0; x < 150; x++)
	{
		UINT8 *led = &state->display[x];

		for( y = 0; y < 128; y+=2 )
		{
			if( *led > 0 )
				*BITMAP_ADDR16(bitmap, 30 + y, 85 + x) = --(*led);
			else
				*BITMAP_ADDR16(bitmap, 30 + y, 85 + x) = 0;

			led += 256;
		}
	}
	return 0;
}

