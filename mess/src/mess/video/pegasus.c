/***************************************************************************

    The Aamber Pegasus uses a MCM66710P, which is functionally
    equivalent to the MCM6571. Some code here is copied from poly88.

    Note that datasheet is incorrect for the number "9", line 57 below.
    The first byte is really 0x3E rather than 0x3F, confirmed on real
    hardware.


****************************************************************************/

#include "emu.h"
#include "includes/pegasus.h"

UINT8* pegasus_video_ram;

static UINT8 mcm6571a[] =
{
	0x00,0x00,0x00,0x00,0x31,0x4A,0x44,0x4A,0x31,// 0
	0x3C,0x22,0x3C,0x22,0x22,0x3C,0x20,0x20,0x40,// 1
	0x00,0x61,0x12,0x14,0x18,0x10,0x30,0x30,0x30,// 2
	0x30,0x48,0x40,0x40,0x20,0x30,0x48,0x48,0x30,// 3
	0x00,0x00,0x18,0x20,0x40,0x78,0x40,0x20,0x18,// 4
	0x16,0x0E,0x10,0x20,0x40,0x40,0x38,0x04,0x18,// 5
	0x00,0x2C,0x52,0x12,0x12,0x12,0x02,0x02,0x02,// 6
	0x18,0x24,0x42,0x42,0x7E,0x42,0x42,0x24,0x18,// 7
	0x00,0x00,0x00,0x40,0x40,0x40,0x40,0x48,0x30,// 8
	0x00,0x00,0x40,0x48,0x50,0x60,0x50,0x4A,0x44,// 9
	0x40,0x20,0x10,0x10,0x10,0x10,0x18,0x24,0x42,// 10
	0x00,0x48,0x48,0x48,0x48,0x74,0x40,0x40,0x40,// 11
	0x00,0x00,0x00,0x62,0x22,0x24,0x28,0x30,0x20,// 12
	0x08,0x1C,0x20,0x18,0x20,0x40,0x3C,0x02,0x0C,// 13
	0x00,0x00,0x00,0x18,0x24,0x42,0x42,0x24,0x18,// 14
	0x00,0x00,0x00,0x3F,0x54,0x14,0x14,0x14,0x14,// 15
	0x18,0x24,0x42,0x42,0x64,0x58,0x40,0x40,0x40,// 16
	0x00,0x00,0x00,0x1F,0x24,0x42,0x42,0x24,0x18,// 17
	0x00,0x00,0x00,0x3F,0x48,0x08,0x08,0x08,0x08,// 18
	0x00,0x00,0x00,0x62,0x24,0x24,0x24,0x24,0x18,// 19
	0x10,0x10,0x38,0x54,0x54,0x54,0x38,0x10,0x10,// 20
	0x00,0x00,0x00,0x00,0x62,0x14,0x08,0x14,0x23,// 21
	0x08,0x49,0x2A,0x2A,0x2A,0x1C,0x08,0x08,0x08,// 22
	0x00,0x00,0x00,0x22,0x41,0x49,0x49,0x49,0x36,// 23
	0x00,0x1C,0x22,0x41,0x41,0x41,0x22,0x22,0x63,// 24
	0x1F,0x10,0x10,0x10,0x10,0x10,0x50,0x30,0x10,// 25
	0x00,0x00,0x04,0x02,0x7F,0x02,0x04,0x00,0x00,// 26
	0x00,0x00,0x10,0x20,0x7F,0x20,0x10,0x00,0x00,// 27
	0x08,0x1C,0x2A,0x08,0x08,0x08,0x08,0x08,0x08,// 28
	0x00,0x00,0x08,0x00,0x7F,0x00,0x08,0x00,0x00,// 29
	0x7F,0x20,0x10,0x08,0x06,0x08,0x10,0x20,0x7F,// 30
	0x00,0x30,0x49,0x06,0x30,0x49,0x06,0x00,0x00,// 31
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,// 32
	0x08,0x08,0x08,0x08,0x08,0x08,0x00,0x00,0x08,// 33
	0x12,0x12,0x12,0x00,0x00,0x00,0x00,0x00,0x00,// 34
	0x14,0x14,0x14,0x7F,0x14,0x7F,0x14,0x14,0x14,// 35
	0x08,0x3F,0x48,0x48,0x3E,0x09,0x09,0x7E,0x08,// 36
	0x20,0x51,0x22,0x04,0x08,0x10,0x22,0x45,0x02,// 37
	0x38,0x44,0x44,0x28,0x10,0x29,0x46,0x46,0x39,// 38
	0x20,0x20,0x40,0x00,0x00,0x00,0x00,0x00,0x00,// 39
	0x08,0x10,0x20,0x20,0x20,0x20,0x20,0x10,0x08,// 40
	0x08,0x04,0x02,0x02,0x02,0x02,0x02,0x04,0x08,// 41
	0x08,0x49,0x2A,0x1C,0x7F,0x1C,0x2A,0x49,0x08,// 42
	0x00,0x08,0x08,0x08,0x7F,0x08,0x08,0x08,0x00,// 43
	0x00,0x00,0x00,0x00,0x00,0x30,0x10,0x10,0x20,// 44
	0x00,0x00,0x00,0x00,0x7F,0x00,0x00,0x00,0x00,// 45
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x10,// 46
	0x00,0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x00,// 47
	0x3E,0x41,0x43,0x45,0x49,0x51,0x61,0x41,0x3E,// 48
	0x08,0x18,0x08,0x08,0x08,0x08,0x08,0x08,0x3E,// 49
	0x3E,0x41,0x01,0x02,0x0C,0x10,0x20,0x40,0x7F,// 50
	0x3E,0x41,0x01,0x01,0x1E,0x01,0x01,0x41,0x3E,// 51
	0x02,0x06,0x0A,0x12,0x22,0x7F,0x02,0x02,0x02,// 52
	0x7F,0x40,0x40,0x40,0x7E,0x01,0x01,0x41,0x3E,// 53
	0x3E,0x41,0x40,0x40,0x7E,0x41,0x41,0x41,0x3E,// 54
	0x7F,0x41,0x02,0x04,0x08,0x10,0x10,0x10,0x10,// 55
	0x3E,0x41,0x41,0x41,0x3E,0x41,0x41,0x41,0x3E,// 56
	0x3E,0x41,0x41,0x41,0x3F,0x01,0x01,0x41,0x3E,// 57
	0x00,0x00,0x20,0x00,0x00,0x00,0x20,0x00,0x00,// 58
	0x20,0x00,0x00,0x00,0x00,0x30,0x10,0x10,0x20,// 59
	0x04,0x08,0x10,0x20,0x40,0x20,0x10,0x08,0x04,// 60
	0x00,0x00,0x00,0x7F,0x00,0x7F,0x00,0x00,0x00,// 61
	0x10,0x08,0x04,0x02,0x01,0x02,0x04,0x08,0x10,// 62
	0x3E,0x41,0x41,0x02,0x04,0x08,0x08,0x00,0x08,// 63
	0x3E,0x41,0x41,0x4D,0x55,0x5E,0x40,0x40,0x3F,// 64
	0x1C,0x22,0x41,0x41,0x7F,0x41,0x41,0x41,0x41,// 65
	0x7E,0x21,0x21,0x21,0x3E,0x21,0x21,0x21,0x7E,// 66
	0x1E,0x21,0x40,0x40,0x40,0x40,0x40,0x21,0x1E,// 67
	0x7E,0x21,0x21,0x21,0x21,0x21,0x21,0x21,0x7E,// 68
	0x7F,0x40,0x40,0x40,0x78,0x40,0x40,0x40,0x7F,// 69
	0x7F,0x40,0x40,0x40,0x78,0x40,0x40,0x40,0x40,// 70
	0x1E,0x21,0x40,0x40,0x40,0x47,0x41,0x21,0x1E,// 71
	0x41,0x41,0x41,0x41,0x7F,0x41,0x41,0x41,0x41,// 72
	0x3E,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x3E,// 73
	0x1F,0x04,0x04,0x04,0x04,0x04,0x04,0x44,0x38,// 74
	0x41,0x42,0x44,0x48,0x70,0x48,0x44,0x42,0x41,// 75
	0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x7F,// 76
	0x41,0x63,0x55,0x49,0x49,0x41,0x41,0x41,0x41,// 77
	0x41,0x61,0x51,0x49,0x45,0x43,0x41,0x41,0x41,// 78
	0x1C,0x22,0x41,0x41,0x41,0x41,0x41,0x22,0x1C,// 79
	0x7E,0x41,0x41,0x41,0x7E,0x40,0x40,0x40,0x40,// 80
	0x1C,0x22,0x41,0x41,0x41,0x41,0x45,0x22,0x1D,// 81
	0x7E,0x41,0x41,0x41,0x7E,0x48,0x44,0x42,0x41,// 82
	0x3E,0x41,0x40,0x40,0x3E,0x01,0x01,0x41,0x3E,// 83
	0x7F,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,// 84
	0x41,0x41,0x41,0x41,0x41,0x41,0x41,0x41,0x3E,// 85
	0x41,0x41,0x41,0x41,0x41,0x41,0x22,0x14,0x08,// 86
	0x41,0x41,0x41,0x49,0x49,0x49,0x55,0x63,0x41,// 87
	0x41,0x41,0x22,0x14,0x08,0x14,0x22,0x41,0x41,// 88
	0x41,0x41,0x22,0x14,0x08,0x08,0x08,0x08,0x08,// 89
	0x7F,0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x7F,// 90
	0x1E,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x1E,// 91
	0x00,0x40,0x20,0x10,0x08,0x04,0x02,0x01,0x00,// 92
	0x3C,0x04,0x04,0x04,0x04,0x04,0x04,0x04,0x3C,// 93
	0x3E,0x41,0x00,0x00,0x00,0x00,0x00,0x00,0x00,// 94
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x7F,// 95
	0x02,0x02,0x01,0x00,0x00,0x00,0x00,0x00,0x00,// 96
	0x00,0x00,0x00,0x1E,0x22,0x42,0x42,0x46,0x3D,// 97
	0x40,0x40,0x40,0x5C,0x62,0x42,0x42,0x62,0x5C,// 98
	0x00,0x00,0x00,0x3C,0x42,0x40,0x40,0x42,0x3C,// 99
	0x02,0x02,0x02,0x3A,0x46,0x42,0x42,0x46,0x3A,// 100
	0x00,0x00,0x00,0x3C,0x42,0x42,0x7E,0x40,0x3E,// 101
	0x0C,0x12,0x10,0x10,0x7C,0x10,0x10,0x10,0x10,// 102
	0x3A,0x46,0x42,0x42,0x46,0x3A,0x02,0x42,0x3C,// 103
	0x40,0x40,0x40,0x58,0x64,0x42,0x42,0x42,0x42,// 104
	0x00,0x08,0x00,0x18,0x08,0x08,0x08,0x08,0x08,// 105
	0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x42,0x3C,// 106
	0x40,0x40,0x40,0x44,0x48,0x70,0x48,0x44,0x42,// 107
	0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,// 108
	0x00,0x00,0x00,0x76,0x49,0x49,0x49,0x49,0x49,// 109
	0x00,0x00,0x00,0x5C,0x62,0x42,0x42,0x42,0x42,// 110
	0x00,0x00,0x00,0x3C,0x42,0x42,0x42,0x42,0x3C,// 111
	0x5C,0x62,0x42,0x42,0x62,0x5C,0x40,0x40,0x40,// 112
	0x3A,0x46,0x42,0x42,0x46,0x3A,0x02,0x02,0x02,// 113
	0x00,0x00,0x00,0x5C,0x62,0x40,0x40,0x40,0x40,// 114
	0x00,0x00,0x00,0x3C,0x42,0x30,0x0C,0x42,0x3C,// 115
	0x00,0x10,0x10,0x7C,0x10,0x10,0x10,0x12,0x0C,// 116
	0x00,0x00,0x00,0x42,0x42,0x42,0x42,0x42,0x3C,// 117
	0x00,0x00,0x00,0x44,0x44,0x44,0x44,0x28,0x10,// 118
	0x00,0x00,0x00,0x41,0x49,0x49,0x49,0x49,0x36,// 119
	0x00,0x00,0x00,0x42,0x24,0x18,0x18,0x24,0x42,// 120
	0x42,0x42,0x42,0x42,0x46,0x3A,0x02,0x42,0x3C,// 121
	0x00,0x00,0x00,0x7E,0x04,0x08,0x10,0x20,0x7E,// 122
	0x0E,0x10,0x10,0x10,0x20,0x10,0x10,0x10,0x0E,// 123
	0x08,0x08,0x08,0x00,0x00,0x08,0x08,0x08,0x00,// 124
	0x38,0x04,0x04,0x04,0x02,0x04,0x04,0x04,0x38,// 125
	0x30,0x49,0x06,0x00,0x00,0x00,0x00,0x00,0x00,// 126
	0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F // 127
};

static UINT8 mcm6571a_shift[] =
{
	0,1,1,0,0,0,1,0,0,0,0,1,0,0,0,0,
	1,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,1,0,0,1,0,0,0,0,0,
	1,1,0,0,0,0,0,0,0,1,0,0,0,0,0,0
};

static UINT8 get_mcm6571a_line(UINT8 code, UINT8 line, UINT8 pcg_mode, UINT8 *FNT)
{
	if (pcg_mode)
	{
		return FNT[(code << 4) | line];
	}
	else
	if (!mcm6571a_shift[code])
	{
		if (line < 1)
			return 0;
		else
		if (line < 10)
			return mcm6571a[code*9 + line - 1];
		else
			return 0;
	}
	else
	{
		if (line < 4)
			return 0;
		else
		if (line <13)
			return mcm6571a[code*9 + line - 4];
		else
			return 0;
	}
	return 0;

}

VIDEO_UPDATE( pegasus )
{
	UINT16 addr,xpos,x,y;
	UINT8 b,j,l,code,inv;
	UINT8 *FNT = memory_region(screen->machine, "pcg");
	UINT8 pcg_mode = pegasus_control_bits & 2;

	for(y = 0; y < 16; y++ )
	{
		addr = y<<5;
		xpos = 0;
		for(x = 0; x < 32; x++ )
		{
			code = pegasus_video_ram[addr|x];
			inv = (code >> 7) ^ 1;
			for(j = 0; j < 16; j++ )
			{
				l = get_mcm6571a_line(code &0x7f, j, pcg_mode, FNT);
				for(b = 0; b < 8; b++ )
					*BITMAP_ADDR16(bitmap, (y<<4)|j, xpos+b ) =  inv ^ ((l >> (7-b)) & 1);
			}
			xpos +=8;
		}
	}
	return 0;
}
