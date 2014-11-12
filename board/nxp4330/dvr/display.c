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

extern void display_rgb(int module, unsigned int fbbase,
					struct disp_vsync_info *pvsync, struct disp_syncgen_param *psgen,
					struct disp_multily_param *pmly, struct disp_rgb_param *prgb);

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

#define	INIT_PARAM_RGB(name)							\
	struct disp_rgb_param name = {							\
		.lcd_mpu_type 	= 0,                                \
	};

#define SPI_CS 			PAD_GPIO_D + 21//GPIOD21
#define SPI_CLK 		PAD_GPIO_D + 1//GPIOD1
#define SPI_DI 			PAD_GPIO_D + 16//GPIOD16


#define IO_OUTPUT(_io)		NX_GPIO_SetOutputEnable	(PAD_GET_GROUP(_io), PAD_GET_BITNO(_io), CTRUE )
#define IO_HIGH(_io)		NX_GPIO_SetOutputValue 	(PAD_GET_GROUP(_io), PAD_GET_BITNO(_io), CTRUE )
#define IO_LOW(_io)			NX_GPIO_SetOutputValue 	(PAD_GET_GROUP(_io), PAD_GET_BITNO(_io), CFALSE)

void SPI_SendData(unsigned char i)
{
   unsigned char n;
   
   for(n=0; n<8; n++)
   {
		if(i&0x80)
			IO_HIGH(SPI_DI);
		else
			IO_LOW(SPI_DI);

		i<<= 1;

		IO_LOW(SPI_CLK);
		udelay(1);
		IO_HIGH(SPI_CLK);
		udelay(1);
   }
}

void SPI_WriteCommm(unsigned char i)
{
    IO_LOW(SPI_CS);

    IO_LOW(SPI_DI);

	IO_LOW(SPI_CLK);
	udelay(1);
	IO_HIGH(SPI_CLK);
	udelay(1);

	SPI_SendData(i);

    IO_HIGH(SPI_CS);
}

void SPI_WriteDataa(unsigned char i)
{
    IO_LOW(SPI_CS);

    IO_HIGH(SPI_DI);

	IO_LOW(SPI_CLK);
	IO_HIGH(SPI_CLK);

	SPI_SendData(i);

    IO_HIGH(SPI_CS);
}

void init_lcd(void)
{
	IO_OUTPUT(SPI_CS);
	IO_OUTPUT(SPI_CLK);
	IO_OUTPUT(SPI_DI);

	SPI_WriteCommm(0x01);
	mdelay(10);

	SPI_WriteCommm(0xC8);SPI_WriteDataa(0xFF);SPI_WriteDataa(0x93);SPI_WriteDataa(0x42);
	SPI_WriteCommm(0xB4);SPI_WriteDataa(0x02);
	SPI_WriteCommm(0xB0);SPI_WriteDataa(0xC0);    //RGB
	SPI_WriteCommm(0xF6);SPI_WriteDataa(0x00);SPI_WriteDataa(0x00);SPI_WriteDataa(0x06); //RGB
	SPI_WriteCommm(0x36);SPI_WriteDataa(0xC8);
	SPI_WriteCommm(0x3A);SPI_WriteDataa(0x66);
	SPI_WriteCommm(0xC0);SPI_WriteDataa(0x0F);SPI_WriteDataa(0x0F);
	SPI_WriteCommm(0xC1);SPI_WriteDataa(0x01);
	SPI_WriteCommm(0xC5);SPI_WriteDataa(0xC8);
	SPI_WriteCommm(0xE0);SPI_WriteDataa(0x0F);SPI_WriteDataa(0x00);SPI_WriteDataa(0x08);SPI_WriteDataa(0x05);SPI_WriteDataa(0x08);SPI_WriteDataa(0x1A);SPI_WriteDataa(0x0C);SPI_WriteDataa(0x42);SPI_WriteDataa(0x7A);SPI_WriteDataa(0x54);SPI_WriteDataa(0x08);SPI_WriteDataa(0x08);SPI_WriteDataa(0x08);SPI_WriteDataa(0x23);SPI_WriteDataa(0x25);SPI_WriteDataa(0x0F);
	SPI_WriteCommm(0xE1);SPI_WriteDataa(0x0F);SPI_WriteDataa(0x00);SPI_WriteDataa(0x2f);SPI_WriteDataa(0x29);SPI_WriteDataa(0x08);SPI_WriteDataa(0x0F);SPI_WriteDataa(0x05);SPI_WriteDataa(0x42);SPI_WriteDataa(0x55);SPI_WriteDataa(0x53);SPI_WriteDataa(0x06);SPI_WriteDataa(0x08);SPI_WriteDataa(0x08);SPI_WriteDataa(0x38);SPI_WriteDataa(0x3A);SPI_WriteDataa(0x0f);

//	SPI_WriteCommm(0x20);
	SPI_WriteCommm(0x11);
	mdelay(20);
	SPI_WriteCommm(0x29);
//	SPI_WriteCommm(0x2C);
}

int bd_display(void)
{
#if defined(CONFIG_DISPLAY_OUT_RGB)
	INIT_VIDEO_SYNC(vsync);
	INIT_PARAM_SYNCGEN(syncgen);
	INIT_PARAM_MULTILY(multily);
	INIT_PARAM_RGB(rgb);

	display_rgb(CFG_DISP_OUTPUT_MODOLE, CONFIG_FB_ADDR,
		&vsync, &syncgen, &multily, &rgb);
#endif
	return 0;
}
