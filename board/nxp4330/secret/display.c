/*
 * (C) Copyright 2009
 * jung hyun kim, Nexell Co, <jhkim@nexell.co.kr>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */
#include <config.h>
#include <common.h>
#include <asm/arch/mach-api.h>
#include <asm/arch/display.h>

#if defined(CONFIG_DISPLAY_OUT_LVDS)
extern void display_lvds(int module, unsigned int fbbase,
					struct disp_vsync_info *pvsync, struct disp_syncgen_param *psgen,
					struct disp_multily_param *pmly, struct disp_lvds_param *plvds);
#endif

#if defined(CONFIG_DISPLAY_OUT_MIPI)
extern void display_mipi(int module, unsigned int fbbase,
				struct disp_vsync_info *pvsync, struct disp_syncgen_param *psgen,
				struct disp_multily_param *pmly, struct disp_mipi_param *pmipi);

#if defined(CONFIG_SECRET_2P1ND_BOARD)||defined(CONFIG_SECRET_3RD_BOARD)
#define	MIPI_BITRATE_480M
#else
#define	MIPI_BITRATE_750M
#endif

#ifdef MIPI_BITRATE_1G
#define	PLLPMS		0x33E8
#define	BANDCTL		0xF
#elif defined(MIPI_BITRATE_900M)
#define	PLLPMS		0x2258
#define	BANDCTL		0xE
#elif defined(MIPI_BITRATE_840M)
#define	PLLPMS		0x2230
#define	BANDCTL		0xD
#elif defined(MIPI_BITRATE_750M)
#define	PLLPMS		0x43E8
#define	BANDCTL		0xC
#elif defined(MIPI_BITRATE_660M)
#define	PLLPMS		0x21B8
#define	BANDCTL		0xB
#elif defined(MIPI_BITRATE_600M)
#define	PLLPMS		0x2190
#define	BANDCTL		0xA
#elif defined(MIPI_BITRATE_540M)
#define	PLLPMS		0x2168
#define	BANDCTL		0x9
#elif defined(MIPI_BITRATE_512M)
#define	PLLPMS		0x03200
#define	BANDCTL		0x9
#elif defined(MIPI_BITRATE_480M)
#define	PLLPMS		0x2281
#define	BANDCTL		0x8
#elif defined(MIPI_BITRATE_420M)
#define	PLLPMS		0x2231
#define	BANDCTL		0x7
#elif defined(MIPI_BITRATE_402M)
#define	PLLPMS		0x2219
#define	BANDCTL		0x7
#elif defined(MIPI_BITRATE_210M)
#define	PLLPMS		0x2232
#define	BANDCTL		0x4
#endif
#define	PLLCTL		0
#define	DPHYCTL		0

#define MIPI_DELAY 0xFF
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

struct data_val{
	u8 data[48];
};

struct mipi_reg_val{
	u32 cmd;
	u32 addr;
	u32 cnt;
	struct data_val data;
};

#if defined(CONFIG_SECRET_2P1ND_BOARD)||defined(CONFIG_SECRET_3RD_BOARD)

static struct mipi_reg_val mipi_init_data_tiny[]= // BOE init code tiny
{
 {MIPI_DELAY,  5, 0, {0}},
 {0x39, 0xF0,  2, {0x5A, 0x5A}},
 {0x39, 0xD0,  2, {0x00, 0x10}},

 {MIPI_DELAY,  1, 0, {0}},
 {0x39, 0xC3,  3, {0x40, 0x00, 0x28}},

 {MIPI_DELAY,  5, 0, {0}},
 {0x05, 0x00,  1, {0x11}}, 

 {MIPI_DELAY,120, 0, {0}},
 {0x39, 0xF0,  2, {0x5A, 0x5A}},
 {0x15, 0x35,  1, {0x00}},
 {0x05, 0x00,  1, {0x29}},
};

static struct mipi_reg_val mipi_init_data[]= // BOE init code
{
 {0x39, 0xF0,  2, {0x5A, 0x5A}},
 {0x39, 0xF1,  2, {0x5A, 0x5A}},
 {0x39, 0xFC,  2, {0xA5, 0xA5}},
 {0x15, 0xB1,  1, {0x10}},
 {0x39, 0xB2,  4, {0x14, 0x22, 0x2F, 0x04}},
 {0x39, 0xD0,  2, {0x00, 0x10}},
 {0x39, 0xF2,  5, {0x02, 0x08, 0x08, 0x40, 0x10}},
 {0x15, 0xB0,  1, {0x03}},
 {0x39, 0xFD,  2, {0x23, 0x09}},
 {0x39, 0xF3, 10, {0x01, 0xD7, 0xE2, 0x62, 0xF4, 0xF7, 0x77, 0x3C, 0x26, 0x00}},
 {0x39, 0xF4, 45, {0x00, 0x02, 0x03, 0x26, 0x03, 0x02, 0x09, 0x00, 0x07, 0x16, 0x16, 0x03, 0x00, 0x08, 0x08, 0x03, 0x0E, 0x0F, 0x12, 0x1C, 0x1D, 0x1E, 0x0C, 0x09, 0x01, 0x04, 0x02, 0x61, 0x74, 0x75, 0x72, 0x83, 0x80, 0x80, 0xB0, 0x00, 0x01, 0x01, 0x28, 0x04, 0x03, 0x28, 0x01, 0xD1, 0x32}},
 {0x39, 0xF5, 26, {0xA2, 0x2F, 0x2F, 0x3A, 0xAB, 0x98, 0x52, 0x0F, 0x33, 0x43, 0x04, 0x59, 0x54, 0x52, 0x05, 0x40, 0x60, 0x4E, 0x60, 0x40, 0x27, 0x26, 0x52, 0x25, 0x6D, 0x18}},
 {0x39, 0xEE,  8, {0x22, 0x00, 0x22, 0x00, 0x22, 0x00, 0x22, 0x00}},
 {0x39, 0xEF,  8, {0x12, 0x12, 0x43, 0x43, 0xA0, 0x04, 0x24, 0x81}},
 {0x39, 0xF7, 32, {0x0A, 0x0A, 0x08, 0x08, 0x0B, 0x0B, 0x09, 0x09, 0x04, 0x05, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x0A, 0x0A, 0x08, 0x08, 0x0B, 0x0B, 0x09, 0x09, 0x04, 0x05, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01}},
 {0x39, 0xBC,  3, {0x01, 0x4E, 0x0A}},
 {0x39, 0xE1,  5, {0x03, 0x10, 0x1C, 0xA0, 0x10}},
 {0x39, 0xF6,  6, {0x60, 0x21, 0xA6, 0x00, 0x00, 0x00}},
 {0x39, 0xFE,  6, {0x00, 0x0D, 0x03, 0x21, 0x00, 0x78}},
 {0x15, 0xB0,  1, {0x22}},
 {0x39, 0xFA, 17, {0x00, 0x35, 0x08, 0x10, 0x08, 0x0F, 0x14, 0x14, 0x17, 0x1F, 0x23, 0x24, 0x25, 0x24, 0x23, 0x23, 0x29}},
 {0x15, 0xB0,  1, {0x22}},
 {0x39, 0xFB, 17, {0x00, 0x35, 0x08, 0x10, 0x08, 0x0F, 0x14, 0x14, 0x17, 0x1F, 0x23, 0x24, 0x25, 0x24, 0x23, 0x23, 0x29}},
 {0x15, 0xB0,  1, {0x11}},
 {0x39, 0xFA, 17, {0x0B, 0x35, 0x11, 0x18, 0x0F, 0x14, 0x1A, 0x19, 0x1A, 0x22, 0x24, 0x25, 0x26, 0x25, 0x24, 0x24, 0x2A}},
 {0x15, 0xB0,  1, {0x11}},
 {0x39, 0xFB, 17, {0x0B, 0x35, 0x11, 0x18, 0x0F, 0x14, 0x1A, 0x19, 0x1A, 0x22, 0x24, 0x25, 0x26, 0x25, 0x24, 0x24, 0x2A}},
 {0x39, 0xFA, 17, {0x1F, 0x35, 0x22, 0x24, 0x18, 0x1B, 0x1D, 0x1A, 0x1B, 0x20, 0x26, 0x25, 0x28, 0x27, 0x25, 0x25, 0x2B}},
 {0x39, 0xFB, 17, {0x1F, 0x35, 0x22, 0x24, 0x18, 0x1B, 0x1D, 0x1A, 0x1B, 0x20, 0x26, 0x25, 0x28, 0x27, 0x25, 0x25, 0x2B}},
 {MIPI_DELAY, 10, 0, {0}},
 {0x39, 0xC3,  3, {0x40, 0x00, 0x28}},
 {MIPI_DELAY,120, 0, {0}},
 {0x15, 0x35,  1, {0x00}},
 {0x05, 0x00,  1, {0x11}}, 
 {0x05, 0x00,  1, {0x29}},
};

#elif defined(CONFIG_SECRET_2ND_BOARD)

static struct mipi_reg_val mipi_init_data[]= // INNOLUX init code
{
 {0x39, 0xFF, 4, {0xAA,0x55,0xA5,0x80}},
 {0x39, 0x6F, 2, {0x11,0x00}},
 {0x39, 0xF7, 2, {0x20,0x00}},
 {0x15, 0x6F, 1, {0x06}},
 {0x15, 0xF7, 1, {0xA0}},
 {0x15, 0x6F, 1, {0x19}},
 {0x15, 0xF7, 1, {0x12}},
 {0x39, 0xF0, 5, {0x55,0xAA,0x52,0x08,0x00}},
 {0x39, 0xEF, 2, {0x07,0xFF}},
 {0x39, 0xEE, 4, {0x87,0x78,0x08,0x40}},
 {0x15, 0xC8, 1, {0x80}},
 {0x39, 0xB1, 2, {0x6C,0x01}},
 {0x15, 0xB6, 1, {0x08}},
 {0x15, 0x6F, 1, {0x02}},
 {0x15, 0xB8, 1, {0x08}},
 {0x39, 0xBB, 2, {0x54,0x54}},
 {0x39, 0xBC, 2, {0x05,0x05}},
 {0x15, 0xC7, 1, {0x01}},
 {0x39, 0xBD, 5, {0x02,0xB0,0x0C,0x0A,0x00}},
 {0x39, 0xF0, 5, {0x55,0xAA,0x52,0x08,0x01}},
 {0x39, 0xB0, 2, {0x05,0x05}},
 {0x39, 0xB1, 2, {0x05,0x05}},
 {0x39, 0xBC, 2, {0x8E,0x00}},
 {0x39, 0xBD, 2, {0x92,0x00}},
 {0x15, 0xCA, 1, {0x00}},
 {0x15, 0xC0, 1, {0x04}},
 {0x39, 0xB3, 2, {0x19,0x19}},
 {0x39, 0xB4, 2, {0x12,0x12}},
 {0x39, 0xB9, 2, {0x24,0x24}},
 {0x39, 0xBA, 2, {0x14,0x14}},
 {0x39, 0xF0, 5, {0x55,0xAA,0x52,0x08,0x02}},
 {0x15, 0xEE, 1, {0x02}},
 {0x39, 0xEF, 4, {0x09,0x06,0x15,0x18}},
 {0x39, 0xB0, 6, {0x00,0x00,0x00,0x11,0x00,0x27}},
 {0x15, 0x6F, 1, {0x06}},
 {0x39, 0xB0, 6, {0x00,0x36,0x00,0x45,0x00,0x5F}},
 {0x15, 0x6F, 1, {0x0C}},
 {0x39, 0xB0, 4, {0x00,0x74,0x00,0xA5}},
 {0x39, 0xB1, 6, {0x00,0xCF,0x01,0x13,0x01,0x47}},
 {0x15, 0x6F, 1, {0x06}},
 {0x39, 0xB1, 6, {0x01,0x9B,0x01,0xDF,0x01,0xE1}},
 {0x15, 0x6F, 1, {0x0C}},
 {0x39, 0xB1, 4, {0x02,0x23,0x02,0x6C}},
 {0x39, 0xB2, 6, {0x02,0x9A,0x02,0xD7,0x03,0x05}},
 {0x15, 0x6F, 1, {0x06}},
 {0x39, 0xB2, 6, {0x03,0x42,0x03,0x68,0x03,0x91}},
 {0x15, 0x6F, 1, {0x0C}},
 {0x39, 0xB2, 4, {0x03,0xA5,0x03,0xBD}},
 {0x39, 0xB3, 4, {0x03,0xD7,0x03,0xFF}},
 {0x39, 0xBC, 6, {0x00,0x00,0x00,0x11,0x00,0x27}},
 {0x15, 0x6F, 1, {0x06}},
 {0x39, 0xBC, 6, {0x00,0x38,0x00,0x47,0x00,0x61}},
 {0x15, 0x6F, 1, {0x0C}},
 {0x39, 0xBC, 4, {0x00,0x78,0x00,0xAB}},
 {0x39, 0xBD, 6, {0x00,0xD7,0x01,0x1B,0x01,0x4F}},
 {0x15, 0x6F, 1, {0x06}},
 {0x39, 0xBD, 6, {0x01,0xA1,0x01,0xE5,0x01,0xE7}},
 {0x15, 0x6F, 1, {0x0C}},
 {0x39, 0xBD, 4, {0x02,0x27,0x02,0x70}},
 {0x39, 0xBE, 6, {0x02,0x9E,0x02,0xDB,0x03,0x07}},
 {0x15, 0x6F, 1, {0x06}},
 {0x39, 0xBE, 6, {0x03,0x44,0x03,0x6A,0x03,0x93}},
 {0x15, 0x6F, 1, {0x0C}},
 {0x39, 0xBE, 4, {0x03,0xA5,0x03,0xBD}},
 {0x39, 0xBF, 4, {0x03,0xD7,0x03,0xFF}},
 {0x39, 0xF0, 5, {0x55,0xAA,0x52,0x08,0x06}},
 {0x39, 0xB0, 2, {0x00,0x17}},
 {0x39, 0xB1, 2, {0x16,0x15}},
 {0x39, 0xB2, 2, {0x14,0x13}},
 {0x39, 0xB3, 2, {0x12,0x11}},
 {0x39, 0xB4, 2, {0x10,0x2D}},
 {0x39, 0xB5, 2, {0x01,0x08}},
 {0x39, 0xB6, 2, {0x09,0x31}},
 {0x39, 0xB7, 2, {0x31,0x31}},
 {0x39, 0xB8, 2, {0x31,0x31}},
 {0x39, 0xB9, 2, {0x31,0x31}},
 {0x39, 0xBA, 2, {0x31,0x31}},
 {0x39, 0xBB, 2, {0x31,0x31}},
 {0x39, 0xBC, 2, {0x31,0x31}},
 {0x39, 0xBD, 2, {0x31,0x09}},
 {0x39, 0xBE, 2, {0x08,0x01}},
 {0x39, 0xBF, 2, {0x2D,0x10}},
 {0x39, 0xC0, 2, {0x11,0x12}},
 {0x39, 0xC1, 2, {0x13,0x14}},
 {0x39, 0xC2, 2, {0x15,0x16}},
 {0x39, 0xC3, 2, {0x17,0x00}},
 {0x39, 0xE5, 2, {0x31,0x31}},
 {0x39, 0xC4, 2, {0x00,0x17}},
 {0x39, 0xC5, 2, {0x16,0x15}},
 {0x39, 0xC6, 2, {0x14,0x13}},
 {0x39, 0xC7, 2, {0x12,0x11}},
 {0x39, 0xC8, 2, {0x10,0x2D}},
 {0x39, 0xC9, 2, {0x01,0x08}},
 {0x39, 0xCA, 2, {0x09,0x31}},
 {0x39, 0xCB, 2, {0x31,0x31}},
 {0x39, 0xCC, 2, {0x31,0x31}},
 {0x39, 0xCD, 2, {0x31,0x31}},
 {0x39, 0xCE, 2, {0x31,0x31}},
 {0x39, 0xCF, 2, {0x31,0x31}},
 {0x39, 0xD0, 2, {0x31,0x31}},
 {0x39, 0xD1, 2, {0x31,0x09}},
 {0x39, 0xD2, 2, {0x08,0x01}},
 {0x39, 0xD3, 2, {0x2D,0x10}},
 {0x39, 0xD4, 2, {0x11,0x12}},
 {0x39, 0xD5, 2, {0x13,0x14}},
 {0x39, 0xD6, 2, {0x15,0x16}},
 {0x39, 0xD7, 2, {0x17,0x00}},
 {0x39, 0xE6, 2, {0x31,0x31}},
 {0x39, 0xD8, 5, {0x00,0x00,0x00,0x00,0x00}},
 {0x39, 0xD9, 5, {0x00,0x00,0x00,0x00,0x00}},
 {0x15, 0xE7, 1, {0x00}},
 {0x39, 0xF0, 5, {0x55,0xAA,0x52,0x08,0x03}},
 {0x39, 0xB0, 2, {0x20,0x00}},
 {0x39, 0xB1, 2, {0x20,0x00}},
 {0x39, 0xB2, 5, {0x05,0x00,0x42,0x00,0x00}},
 {0x39, 0xB6, 5, {0x05,0x00,0x42,0x00,0x00}},
 {0x39, 0xBA, 5, {0x53,0x00,0x42,0x00,0x00}},
 {0x39, 0xBB, 5, {0x53,0x00,0x42,0x00,0x00}},
 {0x15, 0xC4, 1, {0x40}},
 {0x39, 0xF0, 5, {0x55,0xAA,0x52,0x08,0x05}},
 {0x39, 0xB0, 2, {0x17,0x06}},
 {0x15, 0xB8, 1, {0x00}},
 {0x39, 0xBD, 5, {0x03,0x01,0x01,0x00,0x01}},
 {0x39, 0xB1, 2, {0x17,0x06}},
 {0x39, 0xB9, 2, {0x00,0x01}},
 {0x39, 0xB2, 2, {0x17,0x06}},
 {0x39, 0xBA, 2, {0x00,0x01}},
 {0x39, 0xB3, 2, {0x17,0x06}},
 {0x39, 0xBB, 2, {0x0A,0x00}},
 {0x39, 0xB4, 2, {0x17,0x06}},
 {0x39, 0xB5, 2, {0x17,0x06}},
 {0x39, 0xB6, 2, {0x14,0x03}},
 {0x39, 0xB7, 2, {0x00,0x00}},
 {0x39, 0xBC, 2, {0x02,0x01}},
 {0x15, 0xC0, 1, {0x05}},
 {0x15, 0xC4, 1, {0xA5}},
 {0x39, 0xC8, 2, {0x03,0x30}},
 {0x39, 0xC9, 2, {0x03,0x51}},
 {0x39, 0xD1, 5, {0x00,0x05,0x03,0x00,0x00}},
 {0x39, 0xD2, 5, {0x00,0x05,0x09,0x00,0x00}},
 {0x15, 0xE5, 1, {0x02}},
 {0x15, 0xE6, 1, {0x02}},
 {0x15, 0xE7, 1, {0x02}},
 {0x15, 0xE9, 1, {0x02}},
 {0x15, 0xED, 1, {0x33}},
 {0x05, 0x00, 1, {0x11}},
 {0x05, 0x00, 1, {0x29}},
};

#else

static struct mipi_reg_val mipi_init_data[]= 
{
 {0x15, 0xB2,  1, {0x7D}},
 {0x15, 0xAE,  1, {0x0B}},
 {0x15, 0xB6,  1, {0x18}},
 {0x15, 0xD2,  1, {0x64}},
};

#endif


static void  mipilcd_dcs_long_write(U32 cmd, U32 ByteCount, U8* pByteData )
{
	U32 DataCount32 = (ByteCount+3)/4;
	int i = 0;
	U32 index = 0;
	volatile NX_MIPI_RegisterSet* pmipi = (volatile NX_MIPI_RegisterSet*)IO_ADDRESS(NX_MIPI_GetPhysicalAddress(index));

	NX_ASSERT( 512 >= DataCount32 );

#if 0
	printf("0x%02x %2d: ", cmd, ByteCount);
	for(i=0; i< ByteCount; i++)
		printf("%02x ", pByteData[i]);
	printf("\n");
#endif

	for( i=0; i<DataCount32; i++ )
	{
		pmipi->DSIM_PAYLOAD = (pByteData[3]<<24)|(pByteData[2]<<16)|(pByteData[1]<<8)|pByteData[0];
		pByteData += 4;
	}
	pmipi->DSIM_PKTHDR  = (cmd & 0xff) | (ByteCount<<8);
}

static void mipilcd_dcs_write( unsigned int id, unsigned int data0, unsigned int data1 )
{
	U32 index = 0;
	volatile NX_MIPI_RegisterSet* pmipi = (volatile NX_MIPI_RegisterSet*)IO_ADDRESS(NX_MIPI_GetPhysicalAddress(index));

#if 0
	int i = 0;
	switch(id)
	{
		case 0x05:
			printf("0x05  1: %02x \n", data0);
			break;

		case 0x15:
			printf("0x15  2: %02x %02x \n", data0, data1);
			break;
	}
#endif

	pmipi->DSIM_PKTHDR = id | (data0<<8) | (data1<<16);
}


static int MIPI_LCD_INIT(int width, int height, void *data)
{
	int i=0;
	int size=ARRAY_SIZE(mipi_init_data);
	u32 index = 0;
	u32 value = 0;
	u8 pByteData[48];
	u8 bitrate=BANDCTL;
	

	volatile NX_MIPI_RegisterSet* pmipi = (volatile NX_MIPI_RegisterSet*)IO_ADDRESS(NX_MIPI_GetPhysicalAddress(index));
	value = pmipi->DSIM_ESCMODE;
	pmipi->DSIM_ESCMODE = value|(3 << 6);
	value = pmipi->DSIM_ESCMODE;
	printf("DSIM_ESCMODE : 0x%x\n", value);
	switch(bitrate)
	{
		case 0xF:	printf("MIPI clk: 1000MHz \n");	break;
		case 0xE:	printf("MIPI clk:  900MHz \n");	break;
		case 0xD:	printf("MIPI clk:  840MHz \n");	break;
		case 0xC:	printf("MIPI clk:  760MHz \n");	break;
		case 0xB:	printf("MIPI clk:  660MHz \n");	break;
		case 0xA:	printf("MIPI clk:  600MHz \n");	break;
		case 0x9:	printf("MIPI clk:  540MHz \n");	break;
		case 0x8:	printf("MIPI clk:  480MHz \n");	break;
		case 0x7:	printf("MIPI clk:  420MHz \n");	break;
		case 0x6:	printf("MIPI clk:  330MHz \n");	break;
		case 0x5:	printf("MIPI clk:  300MHz \n");	break;
		case 0x4:	printf("MIPI clk:  210MHz \n");	break;
		case 0x3:	printf("MIPI clk:  180MHz \n");	break;
		case 0x2:	printf("MIPI clk:  150MHz \n");	break;
		case 0x1:	printf("MIPI clk:  100MHz \n");	break;
		case 0x0:	printf("MIPI clk:   80MHz \n");	break;
		default :	printf("MIPI clk:  unknown \n");	break;
	}

#if defined(CONFIG_SECRET_2P1ND_BOARD)||defined(CONFIG_SECRET_3RD_BOARD)
	mdelay(1);

	NX_GPIO_SetPadFunction(PAD_GET_GROUP(CFG_IO_PANEL_RESET), PAD_GET_BITNO(CFG_IO_PANEL_RESET), PAD_GET_FUNC(CFG_IO_PANEL_RESET));
	NX_GPIO_SetOutputEnable(PAD_GET_GROUP(CFG_IO_PANEL_RESET), PAD_GET_BITNO(CFG_IO_PANEL_RESET), CTRUE);
	NX_GPIO_SetOutputValue(PAD_GET_GROUP(CFG_IO_PANEL_RESET), PAD_GET_BITNO(CFG_IO_PANEL_RESET), CTRUE);
#endif

	mdelay(10);

	for(i=0; i<size; i++)
	{
		switch(mipi_init_data[i].cmd)
		{
#if 0 // all long packet
			case 0x05:
				//pByteData[0] = mipi_init_data[i].addr;
				//memcpy(&pByteData[1], &mipi_init_data[i].data.data[0], 7);
				mipilcd_dcs_long_write(0x39, mipi_init_data[i].cnt, &mipi_init_data[i].data.data[0]);
				break;
			case 0x15:
				pByteData[0] = mipi_init_data[i].addr;
				memcpy(&pByteData[1], &mipi_init_data[i].data.data[0], 7);
				mipilcd_dcs_long_write(0x39, mipi_init_data[i].cnt+1, &pByteData);
				break;
#else
			case 0x05:
				mipilcd_dcs_write(mipi_init_data[i].cmd, mipi_init_data[i].data.data[0], 0x00);
				break;
			case 0x15:
				mipilcd_dcs_write(mipi_init_data[i].cmd, mipi_init_data[i].addr, mipi_init_data[i].data.data[0]);
				break;
#endif
 			case 0x39:
				pByteData[0] = mipi_init_data[i].addr;
				memcpy(&pByteData[1], &mipi_init_data[i].data.data[0], 48);
				mipilcd_dcs_long_write(mipi_init_data[i].cmd, mipi_init_data[i].cnt+1, &pByteData[0]);
				break;
			case MIPI_DELAY:
				//printf("delay %d\n", mipi_init_data[i].addr);
				mdelay(mipi_init_data[i].addr);
				break;
		}
		mdelay(1);
	}

	value = pmipi->DSIM_ESCMODE;
	pmipi->DSIM_ESCMODE = value&(~(3 << 6));
	value = pmipi->DSIM_ESCMODE;
	printf("DSIM_ESCMODE : 0x%x\n", value);

	mdelay(10);
	return 0;
}
#endif /* CONFIG_DISPLAY_OUT_MIPI */

#define	INIT_VIDEO_SYNC(name)								\
	struct disp_vsync_info name = {							\
		.h_active_len	= CFG_DISP_PRI_RESOL_WIDTH,         \
		.h_sync_width	= CFG_DISP_PRI_HSYNC_SYNC_WIDTH,    \
		.h_back_porch	= CFG_DISP_PRI_HSYNC_BACK_PORCH,    \
		.h_front_porch	= CFG_DISP_PRI_HSYNC_FRONT_PORCH,   \
		.h_sync_invert	= CFG_DISP_PRI_HSYNC_ACTIVE_HIGH,   \
		.v_active_len	= CFG_DISP_PRI_RESOL_HEIGHT,        \
		.v_sync_width	= CFG_DISP_PRI_VSYNC_SYNC_WIDTH,    \
		.v_back_porch	= CFG_DISP_PRI_VSYNC_BACK_PORCH,    \
		.v_front_porch	= CFG_DISP_PRI_VSYNC_FRONT_PORCH,   \
		.v_sync_invert	= CFG_DISP_PRI_VSYNC_ACTIVE_HIGH,   \
		.pixel_clock_hz	= CFG_DISP_PRI_PIXEL_CLOCK,   		\
		.clk_src_lv0	= CFG_DISP_PRI_CLKGEN0_SOURCE,      \
		.clk_div_lv0	= CFG_DISP_PRI_CLKGEN0_DIV,         \
		.clk_src_lv1	= CFG_DISP_PRI_CLKGEN1_SOURCE,      \
		.clk_div_lv1	= CFG_DISP_PRI_CLKGEN1_DIV,         \
	};

#define	INIT_PARAM_SYNCGEN(name)						\
	struct disp_syncgen_param name = {						\
		.interlace 		= CFG_DISP_PRI_MLC_INTERLACE,       \
		.out_format		= CFG_DISP_PRI_OUT_FORMAT,          \
		.lcd_mpu_type 	= 0,                                \
		.invert_field 	= CFG_DISP_PRI_OUT_INVERT_FIELD,    \
		.swap_RB		= CFG_DISP_PRI_OUT_SWAPRB,          \
		.yc_order		= CFG_DISP_PRI_OUT_YCORDER,         \
		.delay_mask		= 0,                                \
		.vclk_select	= CFG_DISP_PRI_PADCLKSEL,           \
		.clk_delay_lv0	= CFG_DISP_PRI_CLKGEN0_DELAY,       \
		.clk_inv_lv0	= CFG_DISP_PRI_CLKGEN0_INVERT,      \
		.clk_delay_lv1	= CFG_DISP_PRI_CLKGEN1_DELAY,       \
		.clk_inv_lv1	= CFG_DISP_PRI_CLKGEN1_INVERT,      \
		.clk_sel_div1	= CFG_DISP_PRI_CLKSEL1_SELECT,		\
	};

#define	INIT_PARAM_MULTILY(name)					\
	struct disp_multily_param name = {						\
		.x_resol		= CFG_DISP_PRI_RESOL_WIDTH,			\
		.y_resol		= CFG_DISP_PRI_RESOL_HEIGHT,		\
		.pixel_byte		= CFG_DISP_PRI_SCREEN_PIXEL_BYTE,	\
		.fb_layer		= CFG_DISP_PRI_SCREEN_LAYER,		\
		.video_prior	= CFG_DISP_PRI_VIDEO_PRIORITY,		\
		.mem_lock_size	= 16,								\
		.rgb_format		= CFG_DISP_PRI_SCREEN_RGB_FORMAT,	\
		.bg_color		= CFG_DISP_PRI_BACK_GROUND_COLOR,	\
		.interlace		= CFG_DISP_PRI_MLC_INTERLACE,		\
	};

#define	INIT_PARAM_LVDS(name)							\
	struct disp_lvds_param name = {							\
		.lcd_format 	= CFG_DISP_LVDS_LCD_FORMAT,         \
	};


#define	INIT_PARAM_MIPI(name)	\
	struct disp_mipi_param name = {	\
		.pllpms 	= PLLPMS,       \
		.bandctl	= BANDCTL,      \
		.pllctl		= PLLCTL,    	\
		.phyctl		= DPHYCTL,      \
		.lcd_init	= MIPI_LCD_INIT	\
	};

int bd_display(void)
{
	INIT_VIDEO_SYNC(vsync);
	INIT_PARAM_SYNCGEN(syncgen);
	INIT_PARAM_MULTILY(multily);

#if defined(CONFIG_DISPLAY_OUT_LVDS)
	INIT_PARAM_LVDS(lvds);

	display_lvds(CFG_DISP_OUTPUT_MODOLE, CONFIG_FB_ADDR,
		&vsync, &syncgen, &multily, &lvds);
#endif

#if defined(CONFIG_DISPLAY_OUT_MIPI)
	INIT_PARAM_MIPI(mipi);

	/*
	 * set multilayer parameters
	 */
	multily.x_resol =  800;
	multily.y_resol = 1280;

	/*
	 * set vsync parameters
	 */
#if defined(CONFIG_SECRET_2P1ND_BOARD)||defined(CONFIG_SECRET_3RD_BOARD)
	vsync.h_active_len =  800;
	vsync.v_active_len = 1280;

	vsync.h_sync_width = 16;
	vsync.h_back_porch = 48;
	vsync.h_front_porch = 16;

	vsync.v_sync_width = 4;
	vsync.v_back_porch = 4;
	vsync.v_front_porch = 8;
#elif defined(CONFIG_SECRET_2ND_BOARD)
	vsync.h_active_len =  800;
	vsync.v_active_len = 1280;
	vsync.h_sync_width = 4;
	vsync.h_back_porch = 138;
	vsync.h_front_porch = 138;
	vsync.v_sync_width = 4;
	vsync.v_back_porch = 12;
	vsync.v_front_porch = 10;
#else
	vsync.h_active_len =  800;
	vsync.v_active_len = 1280;
	vsync.h_sync_width = 8;
	vsync.h_back_porch = 40;
	vsync.h_front_porch = 16;
	vsync.v_sync_width = 1;
	vsync.v_back_porch = 2;
	vsync.v_front_porch = 4;
#endif

	/*
	 * set syncgen parameters
	 */
	syncgen.delay_mask = DISP_SYNCGEN_DELAY_RGB_PVD | DISP_SYNCGEN_DELAY_HSYNC_CP1 |
						  	DISP_SYNCGEN_DELAY_VSYNC_FRAM | DISP_SYNCGEN_DELAY_DE_CP;

	syncgen.d_rgb_pvd = 0;
	syncgen.d_hsync_cp1	= 0;
	syncgen.d_vsync_fram = 0;
	syncgen.d_de_cp2 = 7;
	syncgen.vs_start_offset = (vsync.h_front_porch + vsync.h_sync_width +
								vsync.h_back_porch + vsync.h_active_len - 1);
	syncgen.ev_start_offset = (vsync.h_front_porch + vsync.h_sync_width +
								vsync.h_back_porch + vsync.h_active_len - 1);
	syncgen.vs_end_offset = 0;
	syncgen.ev_end_offset = 0;

	lcd_draw_boot_logo(CONFIG_FB_ADDR, multily.x_resol, multily.y_resol, multily.pixel_byte);

	display_mipi(CFG_DISP_OUTPUT_MODOLE, CONFIG_FB_ADDR,
		&vsync, &syncgen, &multily, &mipi);
#endif
	return 0;
}
