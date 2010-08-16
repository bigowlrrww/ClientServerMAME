
/*
 * FM Towns video hardware
 *
 * Resolution: from 320x200 to 768x512
 *
 * Up to two graphics layers
 *
 * Sprites
 *
 * CRTC registers (16-bit):
 *
 *  0:  HSync width 1
 *  1:  HSync width 2
 *  4:  HSync total
 *  5:  VSync width 1
 *  6:  VSync width 2
 *  7:  Equalising pulse accountable time (what?)
 *  8:  VSync total
 *
 *  9:
 *  10: Graphic layer 0 horizontal start/end
 *  11:
 *  12: Graphic layer 1 horizontal start/end
 *
 *  13:
 *  14: Graphic layer 0 vertical start/end
 *  15:
 *  16: Graphic layer 1 vertical start/end
 *
 *  17: Graphic layer 0 initial address?
 *  18: Graphic layer 0 horizontal adjust
 *  19: Graphic layer 0 field indirect address offset
 *  20: Graphic layer 0 line indirect address offset
 *
 *  21-24: As above, but for Graphic layer 1
 *
 *  27: Layer zoom.     bit 0 = x2 horizontal zoom layer 0
 *  to be confirmed     bit 5 = x2 vertical zoom layer 0
 *                      bit 9 = x2 horizontal zoom layer 1
 *                      bit 13 = x2 vertical zoom layer 1
 *
 *  28: Control register 0
 *      VSync enable (bit 15) (blank display?)
 *      Scroll type (layer 0 = bit 4, layer 1 = bit 5)
 *          0 = spherical scroll, 1 = cylindrical scroll
 *
 *  29: Control register 1
 *      Dot clock (bits 1 and 0)
 *      0x00 = 28.6363MHz
 *      0x01 = 24.5454MHz
 *      0x02 = 25.175MHz
 *      0x03 = 21.0525MHz (default?)
 *
 *  30: Dummy register
 *
 *  31: Control register 2
 *
 *  Video registers:
 *
 *  0:  Graphic layer(s) type: (others likely exist)
 *      bit 4 = 2 layers
 *      bits 2-3 = layer 1 mode
 *      bits 0-1 = layer 0 mode
 *          mode: 1 = 16 colours, 2 = 256 colours, 3 = highcolour (16-bit)
 *                0 = disabled?
 *
 *  1:  Layer reverse (priority?) (bit 0)
 *      YM (bit 2) - unknown
 *      peltype (bits 4 and 5)
 *
 *
 *  Sprite registers:
 *
 *  0,1:    Maximum sprite (last one to render?) (10-bit)
 *
 *  1 (bit 7):  Enable sprite display
 *
 *  2,3:    X offset (9-bit)
 *
 *  4,5:    Y offset (9-bit)
 *
 *  6 (bit 4):  VRAM location (0=0x40000,1=0x60000)
 *
 */

#include "emu.h"
#include "machine/pic8259.h"
#include "devices/messram.h"
#include "includes/fmtowns.h"

//#define CRTC_REG_DISP 1
//#define SPR_DEBUG 1

//extern UINT32* towns_vram;
//extern UINT8* towns_gfxvram;
//extern UINT8* towns_txtvram;
//extern UINT8* towns_sprram;

//extern UINT32 towns_mainmem_enable;
//extern UINT32 towns_ankcg_enable;

//static UINT32 pshift;  // for debugging

void towns_crtc_refresh_mode(running_machine* machine)
{
	towns_state* state = (towns_state*)machine->driver_data;
	rectangle scr;
	unsigned int width,height;

	scr.min_x = scr.min_y = 0;
	scr.max_x = state->video.towns_crtc_reg[4];
	scr.max_y = state->video.towns_crtc_reg[8] / 2;

	// layer 0
	width = state->video.towns_crtc_reg[10] - state->video.towns_crtc_reg[9];
	height = (state->video.towns_crtc_reg[14] - state->video.towns_crtc_reg[13]) / 2;
	state->video.towns_crtc_layerscr[0].min_x = (scr.max_x / 2) - (width / 2);
	state->video.towns_crtc_layerscr[0].min_y = (scr.max_y / 2) - (height / 2);
	state->video.towns_crtc_layerscr[0].max_x = (scr.max_x / 2) + (width / 2);
	state->video.towns_crtc_layerscr[0].max_y = (scr.max_y / 2) + (height / 2);

	// layer 1
	width = state->video.towns_crtc_reg[12] - state->video.towns_crtc_reg[11];
	height = (state->video.towns_crtc_reg[16] - state->video.towns_crtc_reg[15]) / 2;
	state->video.towns_crtc_layerscr[1].min_x = (scr.max_x / 2) - (width / 2);
	state->video.towns_crtc_layerscr[1].min_y = (scr.max_y / 2) - (height / 2);
	state->video.towns_crtc_layerscr[1].max_x = (scr.max_x / 2) + (width / 2);
	state->video.towns_crtc_layerscr[1].max_y = (scr.max_y / 2) + (height / 2);

	// sanity checks
	if(scr.max_x == 0 || scr.max_y == 0)
		return;
	if(scr.max_x <= scr.min_x || scr.max_y <= scr.min_y)
		return;

	machine->primary_screen->configure(scr.max_x+1,scr.max_y+1,scr,HZ_TO_ATTOSECONDS(60));
}

READ8_HANDLER( towns_gfx_high_r )
{
	towns_state* state = (towns_state*)space->machine->driver_data;
	return state->towns_gfxvram[offset];
}

WRITE8_HANDLER( towns_gfx_high_w )
{
	towns_state* state = (towns_state*)space->machine->driver_data;
	state->towns_gfxvram[offset] = data;
}

READ8_HANDLER( towns_gfx_r )
{
	towns_state* state = (towns_state*)space->machine->driver_data;
	UINT8 ret = 0;

	if(state->towns_mainmem_enable != 0)
		return messram_get_ptr(state->messram)[offset+0xc0000];

	offset = offset << 2;

	if(state->video.towns_vram_page_sel != 0)
		offset += 0x20000;

	ret = (((state->towns_gfxvram[offset] >> state->video.towns_vram_rplane) << 7) & 0x80)
		| (((state->towns_gfxvram[offset] >> state->video.towns_vram_rplane) << 2) & 0x40)
		| (((state->towns_gfxvram[offset+1] >> state->video.towns_vram_rplane) << 5) & 0x20)
		| (((state->towns_gfxvram[offset+1] >> state->video.towns_vram_rplane)) & 0x10)
		| (((state->towns_gfxvram[offset+2] >> state->video.towns_vram_rplane) << 3) & 0x08)
		| (((state->towns_gfxvram[offset+2] >> state->video.towns_vram_rplane) >> 2) & 0x04)
		| (((state->towns_gfxvram[offset+3] >> state->video.towns_vram_rplane) << 1) & 0x02)
		| (((state->towns_gfxvram[offset+3] >> state->video.towns_vram_rplane) >> 4) & 0x01);

	return ret;
}

WRITE8_HANDLER( towns_gfx_w )
{
	towns_state* state = (towns_state*)space->machine->driver_data;

	if(state->towns_mainmem_enable != 0)
	{
		messram_get_ptr(state->messram)[offset+0xc0000] = data;
		return;
	}
	offset = offset << 2;
	if(state->video.towns_vram_page_sel != 0)
		offset += 0x20000;
	if(state->video.towns_vram_wplane & 0x08)
	{
		state->towns_gfxvram[offset] &= ~0x88;
		state->towns_gfxvram[offset] |= ((data & 0x80) >> 4) | ((data & 0x40) << 1);
		state->towns_gfxvram[offset + 1] &= ~0x88;
		state->towns_gfxvram[offset + 1] |= ((data & 0x20) >> 2) | ((data & 0x10) << 3);
		state->towns_gfxvram[offset + 2] &= ~0x88;
		state->towns_gfxvram[offset + 2] |= ((data & 0x08)) | ((data & 0x04) << 5);
		state->towns_gfxvram[offset + 3] &= ~0x88;
		state->towns_gfxvram[offset + 3] |= ((data & 0x02) << 2) | ((data & 0x01) << 7);
	}
	if(state->video.towns_vram_wplane & 0x04)
	{
		state->towns_gfxvram[offset] &= ~0x44;
		state->towns_gfxvram[offset] |= ((data & 0x80) >> 5) | ((data & 0x40));
		state->towns_gfxvram[offset + 1] &= ~0x44;
		state->towns_gfxvram[offset + 1] |= ((data & 0x20) >> 3) | ((data & 0x10) << 2);
		state->towns_gfxvram[offset + 2] &= ~0x44;
		state->towns_gfxvram[offset + 2] |= ((data & 0x08) >> 1) | ((data & 0x04) << 4);
		state->towns_gfxvram[offset + 3] &= ~0x44;
		state->towns_gfxvram[offset + 3] |= ((data & 0x02) << 1) | ((data & 0x01) << 6);
	}
	if(state->video.towns_vram_wplane & 0x02)
	{
		state->towns_gfxvram[offset] &= ~0x22;
		state->towns_gfxvram[offset] |= ((data & 0x80) >> 6) | ((data & 0x40) >> 1);
		state->towns_gfxvram[offset + 1] &= ~0x22;
		state->towns_gfxvram[offset + 1] |= ((data & 0x20) >> 4) | ((data & 0x10) << 1);
		state->towns_gfxvram[offset + 2] &= ~0x22;
		state->towns_gfxvram[offset + 2] |= ((data & 0x08) >> 2) | ((data & 0x04) << 3);
		state->towns_gfxvram[offset + 3] &= ~0x22;
		state->towns_gfxvram[offset + 3] |= ((data & 0x02)) | ((data & 0x01) << 5);
	}
	if(state->video.towns_vram_wplane & 0x01)
	{
		state->towns_gfxvram[offset] &= ~0x11;
		state->towns_gfxvram[offset] |= ((data & 0x80) >> 7) | ((data & 0x40) >> 2);
		state->towns_gfxvram[offset + 1] &= ~0x11;
		state->towns_gfxvram[offset + 1] |= ((data & 0x20) >> 5) | ((data & 0x10));
		state->towns_gfxvram[offset + 2] &= ~0x11;
		state->towns_gfxvram[offset + 2] |= ((data & 0x08) >> 3) | ((data & 0x04) << 2);
		state->towns_gfxvram[offset + 3] &= ~0x11;
		state->towns_gfxvram[offset + 3] |= ((data & 0x02) >> 1) | ((data & 0x01) << 4);
	}
}

static void towns_update_kanji_offset(running_machine* machine)
{
	towns_state* state = (towns_state*)machine->driver_data;
	// this is a little over the top...
	if(state->video.towns_kanji_code_h < 0x30)
	{
		state->video.towns_kanji_offset = ((state->video.towns_kanji_code_l & 0x1f) << 4)
		                   | (((state->video.towns_kanji_code_l - 0x20) & 0x20) << 8)
		                   | (((state->video.towns_kanji_code_l - 0x20) & 0x40) << 6)
		                   | ((state->video.towns_kanji_code_h & 0x07) << 9);
	}
	else if(state->video.towns_kanji_code_h < 0x70)
	{
		state->video.towns_kanji_offset = ((state->video.towns_kanji_code_l & 0x1f) << 4)
		                   + (((state->video.towns_kanji_code_l - 0x20) & 0x60) << 8)
		                   + ((state->video.towns_kanji_code_h & 0x0f) << 9)
		                   + (((state->video.towns_kanji_code_h - 0x30) & 0x70) * 0xc00)
		                   + 0x8000;
	}
	else
	{
		state->video.towns_kanji_offset = ((state->video.towns_kanji_code_l & 0x1f) << 4)
		                   | (((state->video.towns_kanji_code_l - 0x20) & 0x20) << 8)
		                   | (((state->video.towns_kanji_code_l - 0x20) & 0x40) << 6)
		                   | ((state->video.towns_kanji_code_h & 0x07) << 9)
		                   | 0x38000;
	}
}

READ8_HANDLER( towns_video_cff80_r )
{
	towns_state* state = (towns_state*)space->machine->driver_data;
	UINT8* ROM = memory_region(space->machine,"user");

	switch(offset)
	{
		case 0x00:  // mix register
			return state->video.towns_crtc_mix;
		case 0x01:  // read/write plane select (bit 0-3 write, bit 6-7 read)
			return ((state->video.towns_vram_rplane << 6) & 0xc0) | state->video.towns_vram_wplane;
		case 0x02:  // display planes (bits 0-2,5), display page select (bit 4)
			return state->video.towns_display_plane | state->video.towns_display_page_sel;
		case 0x03:  // VRAM page select (bit 5)
			if(state->video.towns_vram_page_sel != 0)
				return 0x10;
			else
				return 0x00;
		case 0x16:  // Kanji character data
			return ROM[(state->video.towns_kanji_offset << 1) + 0x180000];
		case 0x17:  // Kanji character data
			return ROM[(state->video.towns_kanji_offset++ << 1) + 0x180001];
		case 0x19:  // ANK CG ROM
			if(state->towns_ankcg_enable != 0)
				return 0x01;
			else
				return 0x00;
		default:
			logerror("VGA: read from invalid or unimplemented memory-mapped port %05x\n",0xcff80+offset*4);
	}

	return 0;
}

WRITE8_HANDLER( towns_video_cff80_w )
{
	towns_state* state = (towns_state*)space->machine->driver_data;

	switch(offset)
	{
		case 0x00:  // mix register
			state->video.towns_crtc_mix = data;
			break;
		case 0x01:  // read/write plane select (bit 0-3 write, bit 6-7 read)
			state->video.towns_vram_wplane = data & 0x0f;
			state->video.towns_vram_rplane = (data & 0xc0) >> 6;
			towns_update_video_banks(space);
			//logerror("VGA: VRAM wplane select = 0x%02x\n",towns_vram_wplane);
			break;
		case 0x02:  // display plane (bits 0-2), display page select (bit 4)
			state->video.towns_display_plane = data & 0x27;
			state->video.towns_display_page_sel = data & 0x10;
			break;
		case 0x03:  // VRAM page select (bit 4)
			state->video.towns_vram_page_sel = data & 0x10;
			break;
		case 0x14:  // Kanji offset (high)
			state->video.towns_kanji_code_h = data & 0x7f;
			towns_update_kanji_offset(space->machine);
			//logerror("VID: Kanji code set (high) = %02x %02x\n",towns_kanji_code_h,towns_kanji_code_l);
			break;
		case 0x15:  // Kanji offset (low)
			state->video.towns_kanji_code_l = data & 0x7f;
			towns_update_kanji_offset(space->machine);
			//logerror("VID: Kanji code set (low) = %02x %02x\n",towns_kanji_code_h,towns_kanji_code_l);
			break;
		case 0x19:  // ANK CG ROM
			state->towns_ankcg_enable = data & 0x01;
			towns_update_video_banks(space);
			break;
		default:
			logerror("VGA: write %08x to invalid or unimplemented memory-mapped port %05x\n",data,0xcff80+offset);
	}
}

READ8_HANDLER( towns_video_cff80_mem_r )
{
	towns_state* state = (towns_state*)space->machine->driver_data;

	if(state->towns_mainmem_enable != 0)
		return messram_get_ptr(state->messram)[offset+0xcff80];

	return towns_video_cff80_r(space,offset);
}

WRITE8_HANDLER( towns_video_cff80_mem_w )
{
	towns_state* state = (towns_state*)space->machine->driver_data;

	if(state->towns_mainmem_enable != 0)
	{
		messram_get_ptr(state->messram)[offset+0xcff80] = data;
		return;
	}
	towns_video_cff80_w(space,offset,data);
}
/*
 *  port 0x440-0x443 - CRTC
 *      0x440 = register select
 *      0x442/3 = register data (16-bit)
 *      0x448 = shifter register select
 *      0x44a = shifter register data (8-bit)
 *
 */
READ8_HANDLER(towns_video_440_r)
{
	towns_state* state = (towns_state*)space->machine->driver_data;
	UINT8 ret = 0;
	UINT16 xpos,ypos;

	switch(offset)
	{
		case 0x00:
			return state->video.towns_crtc_sel;
		case 0x02:
//          logerror("CRTC: reading register %i (0x442) [%04x]\n",towns_crtc_sel,towns_crtc_reg[towns_crtc_sel]);
			if(state->video.towns_crtc_sel == 30)
					return 0x00;
			return state->video.towns_crtc_reg[state->video.towns_crtc_sel] & 0x00ff;
		case 0x03:
//          logerror("CRTC: reading register %i (0x443) [%04x]\n",towns_crtc_sel,towns_crtc_reg[towns_crtc_sel]);
			if(state->video.towns_crtc_sel == 30)
			{
				// check video position
				xpos = space->machine->primary_screen->hpos();
				ypos = space->machine->primary_screen->vpos();

				if(xpos < (state->video.towns_crtc_reg[0] & 0xfe))
					ret |= 0x02;
				if(ypos < (state->video.towns_crtc_reg[6] & 0x1f))
					ret |= 0x04;
				if(xpos < state->video.towns_crtc_layerscr[0].max_x && xpos > state->video.towns_crtc_layerscr[0].min_x)
					ret |= 0x10;
				if(xpos < state->video.towns_crtc_layerscr[1].max_x && xpos > state->video.towns_crtc_layerscr[1].min_x)
					ret |= 0x20;
				if(ypos < state->video.towns_crtc_layerscr[0].max_y && ypos > state->video.towns_crtc_layerscr[0].min_y)
					ret |= 0x40;
				if(ypos < state->video.towns_crtc_layerscr[1].max_y && ypos > state->video.towns_crtc_layerscr[1].min_y)
					ret |= 0x80;

				return ret;
			}
			return (state->video.towns_crtc_reg[state->video.towns_crtc_sel] & 0xff00) >> 8;
		case 0x08:
			return state->video.towns_video_sel;
		case 0x0a:
			logerror("Video: reading register %i (0x44a) [%02x]\n",state->video.towns_video_sel,state->video.towns_video_reg[state->video.towns_video_sel]);
			return state->video.towns_video_reg[state->video.towns_video_sel];
		case 0x0c:
			if(state->video.towns_dpmd_flag != 0)
			{
				state->video.towns_dpmd_flag = 0;
				ret |= 0x80;
			}
			ret |= (state->video.towns_vblank_flag ? 0x02 : 0x00);  // TODO: figure out just what this bit is...
			return ret;
		case 0x10:
			return state->video.towns_sprite_sel;
		case 0x12:
			logerror("SPR: reading register %i (0x452) [%02x]\n",state->video.towns_sprite_sel,state->video.towns_sprite_reg[state->video.towns_sprite_sel]);
			return state->video.towns_sprite_reg[state->video.towns_sprite_sel];
		default:
			logerror("VID: read port %04x\n",offset+0x440);
	}
	return 0x00;
}

WRITE8_HANDLER(towns_video_440_w)
{
	towns_state* state = (towns_state*)space->machine->driver_data;

	switch(offset)
	{
		case 0x00:
			state->video.towns_crtc_sel = data;
			break;
		case 0x02:
//          logerror("CRTC: writing register %i (0x442) [%02x]\n",towns_crtc_sel,data);
			state->video.towns_crtc_reg[state->video.towns_crtc_sel] =
				(state->video.towns_crtc_reg[state->video.towns_crtc_sel] & 0xff00) | data;
			towns_crtc_refresh_mode(space->machine);
			break;
		case 0x03:
//          logerror("CRTC: writing register %i (0x443) [%02x]\n",towns_crtc_sel,data);
			state->video.towns_crtc_reg[state->video.towns_crtc_sel] =
				(state->video.towns_crtc_reg[state->video.towns_crtc_sel] & 0x00ff) | (data << 8);
			towns_crtc_refresh_mode(space->machine);
			break;
		case 0x08:
			state->video.towns_video_sel = data & 0x01;
			break;
		case 0x0a:
			logerror("Video: writing register %i (0x44a) [%02x]\n",state->video.towns_video_sel,data);
			state->video.towns_video_reg[state->video.towns_video_sel] = data;
			break;
		case 0x10:
			state->video.towns_sprite_sel = data & 0x07;
			break;
		case 0x12:
			logerror("SPR: writing register %i (0x452) [%02x]\n",state->video.towns_sprite_sel,data);
			state->video.towns_sprite_reg[state->video.towns_sprite_sel] = data;
			break;
		default:
			logerror("VID: wrote 0x%02x to port %04x\n",data,offset+0x440);
	}
}

READ8_HANDLER(towns_video_5c8_r)
{
	towns_state* state = (towns_state*)space->machine->driver_data;

	logerror("VID: read port %04x\n",offset+0x5c8);
	switch(offset)
	{
		case 0x00:  // 0x5c8 - disable TVRAM?
		if(state->video.towns_tvram_enable != 0)
		{
			state->video.towns_tvram_enable = 0;
			return 0x80;
		}
		else
			return 0x00;
	}
	return 0x00;
}

WRITE8_HANDLER(towns_video_5c8_w)
{
	towns_state* state = (towns_state*)space->machine->driver_data;
	running_device* dev = state->pic_slave;

	switch(offset)
	{
		case 0x02:  // 0x5ca - VSync clear?
			pic8259_ir3_w(dev, 0);
			logerror("PIC: IRQ11 (VSync) set low\n");
			//towns_vblank_flag = 0;
			break;
	}
	logerror("VID: wrote 0x%02x to port %04x\n",data,offset+0x5c8);
}

/* Video/CRTC
 *
 * 0xfd90 - palette colour select
 * 0xfd92/4/6 - BRG value
 * 0xfd98-9f  - degipal(?)
 */
READ8_HANDLER(towns_video_fd90_r)
{
	towns_state* state = (towns_state*)space->machine->driver_data;
	UINT8 ret = 0;
	UINT16 xpos;
	switch(offset)
	{
		case 0x00:
			return state->video.towns_palette_select;
		case 0x02:
			return state->video.towns_palette_b[state->video.towns_palette_select];
		case 0x04:
			return state->video.towns_palette_r[state->video.towns_palette_select];
		case 0x06:
			return state->video.towns_palette_g[state->video.towns_palette_select];
		case 0x08:
		case 0x09:
		case 0x0a:
		case 0x0b:
		case 0x0c:
		case 0x0d:
		case 0x0e:
		case 0x0f:
			return state->video.towns_degipal[offset-0x08];
		case 0x10:  // "sub status register"
			// check video position
			xpos = space->machine->primary_screen->hpos();

			if(xpos < state->video.towns_crtc_layerscr[0].max_x && xpos > state->video.towns_crtc_layerscr[0].min_x)
				ret |= 0x02;
			if(state->video.towns_vblank_flag)
				ret |= 0x01;
			return ret;
	}
//  logerror("VID: read port %04x\n",offset+0xfd90);
	return 0x00;
}

WRITE8_HANDLER(towns_video_fd90_w)
{
	towns_state* state = (towns_state*)space->machine->driver_data;

	switch(offset)
	{
		case 0x00:
			state->video.towns_palette_select = data;
			break;
		case 0x02:
			state->video.towns_palette_b[state->video.towns_palette_select] = data;
			break;
		case 0x04:
			state->video.towns_palette_r[state->video.towns_palette_select] = data;
			break;
		case 0x06:
			state->video.towns_palette_g[state->video.towns_palette_select] = data;
			break;
		case 0x08:
		case 0x09:
		case 0x0a:
		case 0x0b:
		case 0x0c:
		case 0x0d:
		case 0x0e:
		case 0x0f:
			state->video.towns_degipal[offset-0x08] = data;
			state->video.towns_dpmd_flag = 1;
			break;
		case 0x10:
			state->video.towns_layer_ctrl = data;
			break;
	}
	//logerror("VID: wrote 0x%02x to port %04x\n",data,offset+0xfd90);
}

READ8_HANDLER(towns_video_ff81_r)
{
	towns_state* state = (towns_state*)space->machine->driver_data;

	return ((state->video.towns_vram_rplane << 6) & 0xc0) | state->video.towns_vram_wplane;
}

WRITE8_HANDLER(towns_video_ff81_w)
{
	towns_state* state = (towns_state*)space->machine->driver_data;

	state->video.towns_vram_wplane = data & 0x0f;
	state->video.towns_vram_rplane = (data & 0xc0) >> 6;
	towns_update_video_banks(space);
	logerror("VGA: VRAM wplane select (I/O) = 0x%02x\n",state->video.towns_vram_wplane);
}

/*
 *  Sprite RAM, low memory
 *  Writing to 0xc8xxx or 0xcaxxx activates TVRAM
 *  Writing to I/O port 0x5c8 disables TVRAM
 *     (bit 7 returns high if TVRAM was previously active)
 *
 *  In TVRAM mode:
 *    0xc8000-0xc8fff: ASCII text (2 bytes each: ISO646 code, then attribute)
 *    0xca000-0xcafff: JIS code
 */
READ8_HANDLER(towns_spriteram_low_r)
{
	towns_state* state = (towns_state*)space->machine->driver_data;
	UINT8* RAM = messram_get_ptr(state->messram);
	UINT8* ROM = memory_region(space->machine,"user");

	if(offset < 0x1000)
	{  // 0xc8000-0xc8fff
		if(state->towns_mainmem_enable == 0)
		{
//          if(towns_tvram_enable == 0)
//              return towns_sprram[offset];
//          else
				return state->towns_txtvram[offset];
		}
		else
			return RAM[offset + 0xc8000];
	}
	if(offset >= 0x1000 && offset < 0x2000)
	{  // 0xc9000-0xc9fff
		return RAM[offset + 0xc9000];
	}
	if(offset >= 0x2000 && offset < 0x3000)
	{  // 0xca000-0xcafff
		if(state->towns_mainmem_enable == 0)
		{
			if(state->towns_ankcg_enable != 0 && offset < 0x2800)
				return ROM[0x180000 + 0x3d000 + (offset-0x2000)];
//          if(towns_tvram_enable == 0)
//              return state->towns_sprram[offset];
//          else
				return state->towns_txtvram[offset];
		}
		else
			return RAM[offset + 0xca000];
	}
	return 0x00;
}

WRITE8_HANDLER(towns_spriteram_low_w)
{
	towns_state* state = (towns_state*)space->machine->driver_data;
	UINT8* RAM = messram_get_ptr(state->messram);

	if(offset < 0x1000)
	{  // 0xc8000-0xc8fff
		state->video.towns_tvram_enable = 1;
		if(state->towns_mainmem_enable == 0)
			state->towns_txtvram[offset] = data;
		else
			RAM[offset + 0xc8000] = data;
	}
	if(offset >= 0x1000 && offset < 0x2000)
	{
		RAM[offset + 0xc9000] = data;
	}
	if(offset >= 0x2000 && offset < 0x3000)
	{  // 0xca000-0xcafff
		state->video.towns_tvram_enable = 1;
		if(state->towns_mainmem_enable == 0)
			state->towns_txtvram[offset] = data;
		else
			RAM[offset + 0xca000] = data;
	}
}

READ8_HANDLER( towns_spriteram_r)
{
	towns_state* state = (towns_state*)space->machine->driver_data;

	return state->towns_txtvram[offset];
}

WRITE8_HANDLER( towns_spriteram_w)
{
	towns_state* state = (towns_state*)space->machine->driver_data;

	state->towns_txtvram[offset] = data;
}

/*
 *  Sprites
 *
 *  Max. 1024, 16x16, 16 colours per sprite
 *  128kB Sprite RAM (8kB attributes, 120kB pattern/colour data)
 *  Sprites are rendered directly to VRAM layer 1 (VRAM offset 0x40000 or 0x60000)
 *
 *  Sprite RAM format:
 *      4 words per sprite
 *      +0: X position (10-bit)
 *      +2: Y position (10-bit)
 *      +4: Sprite Attribute
 *          bit 15: enforce offsets (regs 2-5)
 *          bit 12,13: flip sprite
 *          bits 10-0: Sprite RAM offset containing sprite pattern
 *          TODO: other attributes (zoom?)
 *      +6: Sprite Colour
 *          bit 15: use colour data in located in sprite RAM offset in bits 11-0 (x32)
 */
void render_sprite_4(running_machine* machine, UINT32 poffset, UINT32 coffset, UINT16 x, UINT16 y, UINT16 xflip, UINT16 yflip, const rectangle* rect)
{
	towns_state* state = (towns_state*)machine->driver_data;
	UINT16 xpos,ypos;
	UINT16 col,pixel;
	UINT32 voffset;
	UINT16 xstart,xend,ystart,yend;
	int xdir,ydir;
	int width = (state->video.towns_crtc_reg[12] - state->video.towns_crtc_reg[11]) / (((state->video.towns_crtc_reg[27] & 0x0f00) >> 8)+1);
	int height = (state->video.towns_crtc_reg[16] - state->video.towns_crtc_reg[15]) / (((state->video.towns_crtc_reg[27] & 0xf000) >> 12)+1);
	UINT32 vram_start = (state->video.towns_sprite_reg[6] & 0x10) ? 0x60000 : 0x40000;

	if(xflip)
	{
		xstart = x+14;
		xend = x-2;
		xdir = -2;
	}
	else
	{
		xstart = x+1;
		xend = x+17;
		xdir = 2;
	}
	if(yflip)
	{
		ystart = y+15;
		yend = y-1;
		ydir = -1;
	}
	else
	{
		ystart = y;
		yend = y+16;
		ydir = 1;
	}
	xstart &= 0x1ff;
	xend &= 0x1ff;
	ystart &= 0x1ff;
	yend &= 0x1ff;
	poffset &= 0x1ffff;

	for(ypos=ystart;ypos!=yend;ypos+=ydir,ypos&=0x1ff)
	{
		for(xpos=xstart;xpos!=xend;xpos+=xdir,xpos&=0x1ff)
		{
			voffset = 0;
			pixel = (state->towns_txtvram[poffset] & 0xf0) >> 4;
			col = state->towns_txtvram[coffset+(pixel*2)] | (state->towns_txtvram[coffset+(pixel*2)+1] << 8);
			voffset += (state->video.towns_crtc_reg[24] * 4) * ypos;  // scanline size in bytes * y pos
			voffset += (xpos & 0x1ff) * 2;
			voffset &= 0x3ffff;
			//voffset += (towns_sprite_reg[6] & 0x10) ? 0x60000 : 0x40000;
			if(xpos < width && ypos < height && pixel != 0)
			{
				state->towns_gfxvram[voffset+vram_start+1] = (col & 0xff00) >> 8;
				state->towns_gfxvram[voffset+vram_start] = col & 0x00ff;
			}
			if(xflip)
				voffset+=2;
			else
				voffset-=2;
			pixel = state->towns_txtvram[poffset] & 0x0f;
			col = state->towns_txtvram[coffset+(pixel*2)] | (state->towns_txtvram[coffset+(pixel*2)+1] << 8);
			voffset &= 0x3ffff;
			if(xpos < width && ypos < height && pixel != 0)
			{
				state->towns_gfxvram[voffset+vram_start+1] = (col & 0xff00) >> 8;
				state->towns_gfxvram[voffset+vram_start] = col & 0x00ff;
			}
			poffset++;
			poffset &= 0x1ffff;
		}
	}
}

void render_sprite_16(running_machine* machine, UINT32 poffset, UINT16 x, UINT16 y, UINT16 xflip, UINT16 yflip, const rectangle* rect)
{
	towns_state* state = (towns_state*)machine->driver_data;
	UINT16 xpos,ypos;
	UINT16 col;
	UINT32 voffset;
	UINT16 xstart,ystart,xend,yend;
	int xdir,ydir;
	int width = (state->video.towns_crtc_reg[12] - state->video.towns_crtc_reg[11]) / (((state->video.towns_crtc_reg[27] & 0x0f00) >> 8)+1);
	int height = (state->video.towns_crtc_reg[16] - state->video.towns_crtc_reg[15]) / (((state->video.towns_crtc_reg[27] & 0xf000) >> 12)+1);

	if(xflip)
	{
		xstart = x+16;
		xend = x;
		xdir = -1;
	}
	else
	{
		xstart = x+1;
		xend = x+17;
		xdir = 1;
	}
	if(yflip)
	{
		ystart = y+15;
		yend = y-1;
		ydir = -1;
	}
	else
	{
		ystart = y;
		yend = y+16;
		ydir = 1;
	}
	xstart &= 0x1ff;
	xend &= 0x1ff;
	ystart &= 0x1ff;
	yend &= 0x1ff;
	poffset &= 0x1ffff;

	for(ypos=ystart;ypos!=yend;ypos+=ydir,ypos&=0x1ff)
	{
		for(xpos=xstart;xpos!=xend;xpos+=xdir,xpos&=0x1ff)
		{
			voffset = (state->video.towns_sprite_reg[6] & 0x10) ? 0x60000 : 0x40000;
			col = state->towns_txtvram[poffset] | (state->towns_txtvram[poffset+1] << 8);
			voffset += (state->video.towns_crtc_reg[24] * 4) * ypos;  // scanline size in bytes * y pos
			voffset += (xpos & 0x1ff) * 2;
			voffset &= 0x7ffff;
			if(xpos < width && ypos < height && col < 0x8000)
			{
				state->towns_gfxvram[voffset+1] = (col & 0xff00) >> 8;
				state->towns_gfxvram[voffset] = col & 0x00ff;
			}
			poffset+=2;
			poffset &= 0x1ffff;
		}
	}
}

void draw_sprites(running_machine* machine, const rectangle* rect)
{
	towns_state* state = (towns_state*)machine->driver_data;
	UINT16 sprite_limit = (state->video.towns_sprite_reg[0] | (state->video.towns_sprite_reg[1] << 8)) & 0x3ff;
	int n;
	UINT16 x,y,attr,colour;
	UINT16 xoff = (state->video.towns_sprite_reg[2] | (state->video.towns_sprite_reg[3] << 8)) & 0x1ff;
	UINT16 yoff = (state->video.towns_sprite_reg[4] | (state->video.towns_sprite_reg[5] << 8)) & 0x1ff;
	UINT32 poffset,coffset;

	if(!(state->video.towns_sprite_reg[1] & 0x80))
		return;

	// clears VRAM for each frame?
	if(state->video.towns_sprite_reg[6] & 0x10)
		memset(state->towns_gfxvram+0x60000,0x80,0x20000);
	else
		memset(state->towns_gfxvram+0x40000,0x80,0x20000);

	for(n=sprite_limit;n<1024;n++)
	{
		x = state->towns_txtvram[8*n] | (state->towns_txtvram[8*n+1] << 8);
		y = state->towns_txtvram[8*n+2] | (state->towns_txtvram[8*n+3] << 8);
		attr = state->towns_txtvram[8*n+4] | (state->towns_txtvram[8*n+5] << 8);
		colour = state->towns_txtvram[8*n+6] | (state->towns_txtvram[8*n+7] << 8);
		if(attr & 0x8000)
		{
			x += xoff;
			y += yoff;
		}
		x &= 0x1ff;
		y &= 0x1ff;

		if(colour & 0x8000)
		{
			poffset = (attr & 0x3ff) << 7;
			coffset = (colour & 0xfff) << 5;
#ifdef SPR_DEBUG
			printf("Sprite4 #%i, X %i Y %i Attr %04x Col %04x Poff %08x Coff %08x\n",
				n,x,y,attr,colour,poffset,coffset);
#endif
			if(!(colour & 0x2000))
				render_sprite_4(machine,(poffset)&0x1ffff,coffset,x,y,attr&0x2000,attr&0x1000,rect);
		}
		else
		{
			poffset = (attr & 0x3ff) << 7;
			coffset = (colour & 0xfff) << 5;
#ifdef SPR_DEBUG
			printf("Sprite16 #%i, X %i Y %i Attr %04x Col %04x Poff %08x Coff %08x\n",
				n,x,y,attr,colour,poffset,coffset);
#endif
			if(!(colour & 0x2000))
				render_sprite_16(machine,(poffset)&0x1ffff,x,y,attr&0x2000,attr&0x1000,rect);
		}
	}
}

void towns_crtc_draw_scan_layer_hicolour(running_machine* machine, bitmap_t* bitmap,const rectangle* rect,int layer,int line,int scanline)
{
	towns_state* state = (towns_state*)machine->driver_data;
	UINT32 off = 0;
	int x;
	UINT16 colour;
	int hzoom = 1;
	int linesize;
	UINT32 scroll;

	if(layer == 0)
		linesize = state->video.towns_crtc_reg[20] * 4;
	else
		linesize = state->video.towns_crtc_reg[24] * 4;

	if(state->video.towns_display_page_sel != 0)
		off = 0x20000;
	if(layer != 0)
	{
		if(!(state->video.towns_video_reg[0] & 0x10))
			return;
		if(!(state->video.towns_crtc_reg[28] & 0x10))
			off += (state->video.towns_crtc_reg[21]) << 2;  // initial offset
		else
		{
			scroll = ((state->video.towns_crtc_reg[21] & 0xfc00) << 2) | (((state->video.towns_crtc_reg[21] & 0x3ff) << 2));
			off += scroll;
		}
		off += (state->video.towns_crtc_reg[11] - state->video.towns_crtc_reg[22]);
		if(state->video.towns_crtc_reg[27] & 0x0100)
			hzoom = 2;
	}
	else
	{
		if(!(state->video.towns_crtc_reg[28] & 0x20))
			off += (state->video.towns_crtc_reg[17]) << 2;  // initial offset
		else
		{
			scroll = ((state->video.towns_crtc_reg[17] & 0xfc00) << 2) | (((state->video.towns_crtc_reg[17] & 0x3ff) << 2));
			off += scroll;
		}
		off += (state->video.towns_crtc_reg[9] - state->video.towns_crtc_reg[18]);
		if(state->video.towns_crtc_reg[27] & 0x0001)
			hzoom = 2;
	}

	off += line * linesize;

	if(hzoom == 1)
	{
		for(x=rect->min_x;x<rect->max_x;x++)
		{
			if(state->video.towns_video_reg[0] & 0x10)
				off &= 0x3ffff;  // 2 layers
			else
				off &= 0x7ffff;  // 1 layer

			colour = (state->towns_gfxvram[off+(layer*0x40000)+1] << 8) | state->towns_gfxvram[off+(layer*0x40000)];
			if(colour < 0x8000)
			{
				*BITMAP_ADDR32(bitmap,scanline,x) =
					((colour & 0x001f) << 3)
					| ((colour & 0x7c00) << 1)
					| ((colour & 0x03e0) << 14);
			}
			off+=2;
		}
	}
	else
	{  // x2 horizontal zoom
		for(x=rect->min_x;x<rect->max_x;x+=2)
		{
			if(state->video.towns_video_reg[0] & 0x10)
				off &= 0x3ffff;  // 2 layers
			else
				off &= 0x7ffff;  // 1 layer
			colour = (state->towns_gfxvram[off+(layer*0x40000)+1] << 8) | state->towns_gfxvram[off+(layer*0x40000)];
			if(colour < 0x8000)
			{
				*BITMAP_ADDR32(bitmap,scanline,x) =
					((colour & 0x001f) << 3)
					| ((colour & 0x7c00) << 1)
					| ((colour & 0x03e0) << 14);
				*BITMAP_ADDR32(bitmap,scanline,x+1) =
					((colour & 0x001f) << 3)
					| ((colour & 0x7c00) << 1)
					| ((colour & 0x03e0) << 14);
			}
			off+=2;
		}
	}
}

void towns_crtc_draw_scan_layer_256(running_machine* machine, bitmap_t* bitmap,const rectangle* rect,int layer,int line,int scanline)
{
	towns_state* state = (towns_state*)machine->driver_data;
	int off = 0;
	int x;
	UINT8 colour;
	int hzoom = 1;
	int linesize;
	UINT32 scroll;

	if(state->video.towns_display_page_sel != 0)
		off = 0x20000;
	if(layer == 0)
		linesize = state->video.towns_crtc_reg[20] * 8;
	else
		linesize = state->video.towns_crtc_reg[24] * 8;

	if(layer != 0)
	{
		if(!(state->video.towns_video_reg[0] & 0x10))
			return;
		if(!(state->video.towns_crtc_reg[28] & 0x10))
			off += state->video.towns_crtc_reg[21] << 3;  // initial offset
		else
		{
			scroll = ((state->video.towns_crtc_reg[21] & 0xfc00) << 3) | (((state->video.towns_crtc_reg[21] & 0x3ff) << 3));
			off += scroll;
		}
		off += (state->video.towns_crtc_reg[11] - state->video.towns_crtc_reg[22]);
		if(state->video.towns_crtc_reg[27] & 0x0100)
			hzoom = 2;
	}
	else
	{
		if(!(state->video.towns_crtc_reg[28] & 0x20))
			off += state->video.towns_crtc_reg[17] << 3;  // initial offset
		else
		{
			scroll = ((state->video.towns_crtc_reg[17] & 0xfc00) << 3) | (((state->video.towns_crtc_reg[17] & 0x3ff) << 3));
			off += scroll;
		}
		off += (state->video.towns_crtc_reg[9] - state->video.towns_crtc_reg[18]);
		if(state->video.towns_crtc_reg[27] & 0x0001)
			hzoom = 2;
	}

	off += line * linesize;

	if(hzoom == 1)
	{
		for(x=rect->min_x;x<rect->max_x;x++)
		{
			colour = state->towns_gfxvram[off+(layer*0x40000)];
			if(colour != 0)
			{
				*BITMAP_ADDR32(bitmap,scanline,x) =
					(state->video.towns_palette_r[colour] << 16)
					| (state->video.towns_palette_g[colour] << 8)
					| (state->video.towns_palette_b[colour]);
			}
			off++;
		}
	}
	else
	{  // x2 horizontal zoom
		for(x=rect->min_x;x<rect->max_x;x+=2)
		{
			colour = state->towns_gfxvram[off+(layer*0x40000)+1];
			if(colour != 0)
			{
				*BITMAP_ADDR32(bitmap,scanline,x) =
					(state->video.towns_palette_r[colour] << 16)
					| (state->video.towns_palette_g[colour] << 8)
					| (state->video.towns_palette_b[colour]);
				*BITMAP_ADDR32(bitmap,scanline,x+1) =
					(state->video.towns_palette_r[colour] << 16)
					| (state->video.towns_palette_g[colour] << 8)
					| (state->video.towns_palette_b[colour]);
			}
			off++;
		}
	}
}

void towns_crtc_draw_scan_layer_16(running_machine* machine, bitmap_t* bitmap,const rectangle* rect,int layer,int line,int scanline)
{
	towns_state* state = (towns_state*)machine->driver_data;
	int off = 0;
	int x;
	UINT8 colour;
	int hzoom = 1;
	int linesize;
	UINT32 scroll;

	if(state->video.towns_display_page_sel != 0)
		off = 0x20000;
	if(layer == 0)
		linesize = state->video.towns_crtc_reg[20] * 4;
	else
		linesize = state->video.towns_crtc_reg[24] * 4;

	if(layer != 0)
	{
		if(!(state->video.towns_video_reg[0] & 0x10))
			return;
		if(!(state->video.towns_crtc_reg[28] & 0x10))
			off += state->video.towns_crtc_reg[21];  // initial offset
		else
		{
			scroll = ((state->video.towns_crtc_reg[21] & 0xfc00)<<2) | (((state->video.towns_crtc_reg[21] & 0x3ff)<<2));
			off += scroll;
		}
		off += (state->video.towns_crtc_reg[11] - state->video.towns_crtc_reg[22]);
		if(state->video.towns_crtc_reg[27] & 0x0100)
			hzoom = 2;
	}
	else
	{
		if(!(state->video.towns_crtc_reg[28] & 0x20))
			off += state->video.towns_crtc_reg[17];  // initial offset
		else
		{
			scroll = ((state->video.towns_crtc_reg[17] & 0xfc00)<<2) | (((state->video.towns_crtc_reg[17] & 0x3ff)<<2));
			off += scroll;
		}
		off += (state->video.towns_crtc_reg[9] - state->video.towns_crtc_reg[18]);
		if(state->video.towns_crtc_reg[27] & 0x0001)
			hzoom = 2;
	}

	off += line * linesize;

	if(hzoom == 1)
	{
		for(x=rect->min_x;x<rect->max_x;x+=2)
		{
			colour = state->towns_gfxvram[off+(layer*0x40000)] >> 4;
			if(colour != 0)
			{
				*BITMAP_ADDR32(bitmap,scanline,x+1) =
					(state->video.towns_palette_r[colour] << 16)
					| (state->video.towns_palette_g[colour] << 8)
					| (state->video.towns_palette_b[colour]);
			}
			colour = state->towns_gfxvram[off+(layer*0x40000)] & 0x0f;
			if(colour != 0)
			{
				*BITMAP_ADDR32(bitmap,scanline,x) =
					(state->video.towns_palette_r[colour] << 16)
					| (state->video.towns_palette_g[colour] << 8)
					| (state->video.towns_palette_b[colour]);
			}
			off++;
		}
	}
	else
	{  // x2 horizontal zoom
		for(x=rect->min_x;x<rect->max_x;x+=4)
		{
			if(state->video.towns_video_reg[0] & 0x10)
				off &= 0x3ffff;  // 2 layers
			else
				off &= 0x7ffff;  // 1 layer
			colour = state->towns_gfxvram[off+(layer*0x40000)] >> 4;
			if(colour != 0)
			{
				*BITMAP_ADDR32(bitmap,scanline,x+2) =
					(state->video.towns_palette_r[colour] << 16)
					| (state->video.towns_palette_g[colour] << 8)
					| (state->video.towns_palette_b[colour]);
				*BITMAP_ADDR32(bitmap,scanline,x+3) =
					(state->video.towns_palette_r[colour] << 16)
					| (state->video.towns_palette_g[colour] << 8)
					| (state->video.towns_palette_b[colour]);
			}
			colour = state->towns_gfxvram[off+(layer*0x40000)] & 0x0f;
			if(colour != 0)
			{
				*BITMAP_ADDR32(bitmap,scanline,x) =
					(state->video.towns_palette_r[colour] << 16)
					| (state->video.towns_palette_g[colour] << 8)
					| (state->video.towns_palette_b[colour]);
				*BITMAP_ADDR32(bitmap,scanline,x+1) =
					(state->video.towns_palette_r[colour] << 16)
					| (state->video.towns_palette_g[colour] << 8)
					| (state->video.towns_palette_b[colour]);
			}
			off++;
		}
	}
}

void towns_crtc_draw_layer(running_machine* machine,bitmap_t* bitmap,const rectangle* rect,int layer)
{
	towns_state* state = (towns_state*)machine->driver_data;
	int line;
	int scanline;
	int height;

	if(layer == 0)
	{
		scanline = rect->min_y;
		height = (rect->max_y - rect->min_y);
		if(state->video.towns_crtc_reg[27] & 0x0010)
			height /= 2;
		switch(state->video.towns_video_reg[0] & 0x03)
		{
			case 0x01:
				for(line=0;line<height;line++)
				{
					towns_crtc_draw_scan_layer_16(machine,bitmap,rect,layer,line,scanline);
					scanline++;
					if(state->video.towns_crtc_reg[27] & 0x0010)  // vertical zoom
					{
						towns_crtc_draw_scan_layer_16(machine,bitmap,rect,layer,line,scanline);
						scanline++;
					}
				}
				break;
			case 0x02:
				for(line=0;line<height;line++)
				{
					towns_crtc_draw_scan_layer_256(machine,bitmap,rect,layer,line,scanline);
					scanline++;
					if(state->video.towns_crtc_reg[27] & 0x0010)  // vertical zoom
					{
						towns_crtc_draw_scan_layer_256(machine,bitmap,rect,layer,line,scanline);
						scanline++;
					}
				}
				break;
			case 0x03:
				for(line=0;line<height;line++)
				{
					towns_crtc_draw_scan_layer_hicolour(machine,bitmap,rect,layer,line,scanline);
					scanline++;
					if(state->video.towns_crtc_reg[27] & 0x0010)  // vertical zoom
					{
						towns_crtc_draw_scan_layer_hicolour(machine,bitmap,rect,layer,line,scanline);
						scanline++;
					}
				}
				break;
		}
	}
	else
	{
		scanline = rect->min_y;
		height = (rect->max_y - rect->min_y);
		if(state->video.towns_crtc_reg[27] & 0x1000)
			height /= 2;
		switch(state->video.towns_video_reg[0] & 0x0c)
		{
			case 0x04:
				for(line=0;line<height;line++)
				{
					towns_crtc_draw_scan_layer_16(machine,bitmap,rect,layer,line,scanline);
					scanline++;
					if(state->video.towns_crtc_reg[27] & 0x1000)  // vertical zoom
					{
						towns_crtc_draw_scan_layer_16(machine,bitmap,rect,layer,line,scanline);
						scanline++;
					}
				}
				break;
			case 0x08:
				for(line=0;line<height;line++)
				{
					towns_crtc_draw_scan_layer_256(machine,bitmap,rect,layer,line,scanline);
					scanline++;
					if(state->video.towns_crtc_reg[27] & 0x1000)  // vertical zoom
					{
						towns_crtc_draw_scan_layer_256(machine,bitmap,rect,layer,line,scanline);
						scanline++;
					}
				}
				break;
			case 0x0c:
				for(line=0;line<height;line++)
				{
					towns_crtc_draw_scan_layer_hicolour(machine,bitmap,rect,layer,line,scanline);
					scanline++;
					if(state->video.towns_crtc_reg[27] & 0x1000)  // vertical zoom
					{
						towns_crtc_draw_scan_layer_hicolour(machine,bitmap,rect,layer,line,scanline);
						scanline++;
					}
				}
				break;
		}
	}
}

void render_text_char(running_machine* machine, UINT8 x, UINT8 y, UINT8 ascii, UINT16 jis, UINT8 attr)
{
	towns_state* state = (towns_state*)machine->driver_data;
	UINT32 rom_addr;
	UINT32 vram_addr;
	UINT16 linesize = state->video.towns_crtc_reg[24] * 4;
	UINT8 code_h,code_l;
	UINT8 colour;
	UINT8 data;
	UINT8 temp;
	UINT8* font_rom = memory_region(machine,"user");
	int a,b;

	// all characters are 16 pixels high
	vram_addr = (x * 16) * linesize;

	if((attr & 0xc0) == 0)
		rom_addr = 0x3d800 + (ascii * 128);
	else
	{
		code_h = (jis & 0xff00) >> 8;
		code_l = jis & 0x00ff;
		if(code_h < 0x30)
		{
			rom_addr = ((code_l & 0x1f) << 4)
			                   | (((code_l - 0x20) & 0x20) << 8)
			                   | (((code_l - 0x20) & 0x40) << 6)
			                   | ((code_h & 0x07) << 9);
		}
		else if(code_h < 0x70)
		{
			rom_addr = ((code_l & 0x1f) << 4)
			                   + (((code_l - 0x20) & 0x60) << 8)
			                   + ((code_h & 0x0f) << 9)
			                   + (((code_h - 0x30) & 0x70) * 0xc00)
			                   + 0x8000;
		}
		else
		{
			rom_addr = ((code_l & 0x1f) << 4)
			                   | (((code_l - 0x20) & 0x20) << 8)
			                   | (((code_l - 0x20) & 0x40) << 6)
			                   | ((code_h & 0x07) << 9)
			                   | 0x38000;
		}
	}
	colour = attr & 0x07;
	if(attr & 0x20)
		colour |= 0x08;

	for(a=0;a<16;a++)  // for each scanline
	{
		if((attr & 0xc0) == 0)
			data = font_rom[0x180000 + rom_addr + a];
		else
		{
			if((attr & 0xc0) == 0x80)
				data = font_rom[0x180000 + rom_addr + (a*2)];
			else
				data = font_rom[0x180000 + rom_addr + (a*2) + 1];
		}

		if(attr & 0x08)
			data = ~data;  // inverse

		// and finally, put the data in VRAM
		for(b=0;b<8;b+=2)
		{
			temp = 0;
			if(data & (1<<b))
				temp |= ((colour & 0x0f) << 4);
			if(data & (1<<(b+1)))
				temp |= (colour & 0x0f);
		}

		vram_addr += linesize;
	}
}

void draw_text_layer(running_machine* machine)
{
/*
 *  Text format
 *  2 bytes per character at both 0xc8000 and 0xca000
 *  0xc8xxx: Byte 1: ASCII character
 *           Byte 2: Attributes
 *             bits 2-0: GRB (or is it BRG?)
 *             bit 3: Inverse
 *             bit 4: Blink
 *             bit 5: high brightness
 *             bits 7-6: Kanji high/low
 *
 *  If either bits 6 or 7 are high, then a fullwidth Kanji character is displayed
 *  at this location.  The character displayed is represented by a 2-byte
 *  JIS code at the same offset at 0xca000.
 *
 *  The video hardware renders text to VRAM layer 1, there is no separate text layer
 */
	towns_state* state = (towns_state*)machine->driver_data;
	int x,y,c = 0;

	for(y=0;y<40;y++)
	{
		for(x=0;x<80;x++)
		{
			render_text_char(machine,x,y,state->towns_txtvram[c],((state->towns_txtvram[c+0x2000] << 8)|(state->towns_txtvram[c+0x2001])),state->towns_txtvram[c+1]);
			c+=2;
		}
	}
}

static TIMER_CALLBACK( towns_vblank_end )
{
	// here we'll clear the vsync signal, I presume it goes low on it's own eventually
	towns_state* state = (towns_state*)machine->driver_data;
	running_device* dev = (running_device*)ptr;
	pic8259_ir3_w(dev, 0);  // IRQ11 = VSync
	logerror("PIC: IRQ11 (VSync) set low\n");
	state->video.towns_vblank_flag = 0;
}

INTERRUPT_GEN( towns_vsync_irq )
{
	towns_state* state = (towns_state*)device->machine->driver_data;
	running_device* dev = state->pic_slave;
	pic8259_ir3_w(dev, 1);  // IRQ11 = VSync
	logerror("PIC: IRQ11 (VSync) set high\n");
	state->video.towns_vblank_flag = 1;
	timer_set(device->machine,device->machine->primary_screen->time_until_vblank_end(),(void*)dev,0,towns_vblank_end);
	if(state->video.towns_tvram_enable)
		draw_text_layer(dev->machine);
	if(state->video.towns_sprite_reg[1] & 0x80)  // if sprites are enabled, then sprites are drawn on this layer.
		draw_sprites(dev->machine,&state->video.towns_crtc_layerscr[1]);
}

VIDEO_START( towns )
{
	towns_state* state = (towns_state*)machine->driver_data;

	state->video.towns_vram_wplane = 0x00;
}

VIDEO_UPDATE( towns )
{
	towns_state* state = (towns_state*)screen->machine->driver_data;

	bitmap_fill(bitmap,cliprect,0x00000000);

	if(!(state->video.towns_video_reg[1] & 0x01))
	{
		if(!input_code_pressed(screen->machine,KEYCODE_Q))
		{
			if((state->video.towns_layer_ctrl & 0x03) != 0)
				towns_crtc_draw_layer(screen->machine,bitmap,&state->video.towns_crtc_layerscr[1],1);
		}
		if(!input_code_pressed(screen->machine,KEYCODE_W))
		{
			if((state->video.towns_layer_ctrl & 0x0c) != 0)
				towns_crtc_draw_layer(screen->machine,bitmap,&state->video.towns_crtc_layerscr[0],0);
		}
	}
	else
	{
		if(!input_code_pressed(screen->machine,KEYCODE_Q))
		{
			if((state->video.towns_layer_ctrl & 0x0c) != 0)
				towns_crtc_draw_layer(screen->machine,bitmap,&state->video.towns_crtc_layerscr[0],0);
		}
		if(!input_code_pressed(screen->machine,KEYCODE_W))
		{
			if((state->video.towns_layer_ctrl & 0x03) != 0)
				towns_crtc_draw_layer(screen->machine,bitmap,&state->video.towns_crtc_layerscr[1],1);
		}
	}

/*#ifdef SPR_DEBUG
    if(input_code_pressed(screen->machine,KEYCODE_O))
        pshift+=0x80;
    if(input_code_pressed(screen->machine,KEYCODE_I))
        pshift-=0x80;
    popmessage("Pixel shift = %08x",pshift);
#endif*/

#ifdef CRTC_REG_DISP
	popmessage("CRTC: %i %i %i %i %i %i %i %i %i\n%i %i %i %i | %i %i %i %i\n%04x %i %i %i | %04x %i %i %i\nZOOM: %04x\nVideo: %02x %02x\nText=%i Spr=%02x\nReg28=%04x",
		towns_crtc_reg[0],towns_crtc_reg[1],towns_crtc_reg[2],towns_crtc_reg[3],
		towns_crtc_reg[4],towns_crtc_reg[5],towns_crtc_reg[6],towns_crtc_reg[7],
		towns_crtc_reg[8],
		towns_crtc_reg[9],towns_crtc_reg[10],towns_crtc_reg[11],towns_crtc_reg[12],
		towns_crtc_reg[13],towns_crtc_reg[14],towns_crtc_reg[15],towns_crtc_reg[16],
		towns_crtc_reg[17],towns_crtc_reg[18],towns_crtc_reg[19],towns_crtc_reg[20],
		towns_crtc_reg[21],towns_crtc_reg[22],towns_crtc_reg[23],towns_crtc_reg[24],
		towns_crtc_reg[27],towns_video_reg[0],towns_video_reg[1],towns_tvram_enable,towns_sprite_reg[1] & 0x80,
		towns_crtc_reg[28]);
#endif

    return 0;
}
