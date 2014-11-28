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

#if 0
SPI_WriteCommm(0x01);
	mdelay(10);

SPI_WriteCommm(0xC8);       //Set EXTC
  SPI_WriteDataa(0xFF);
  SPI_WriteDataa(0x93);
  SPI_WriteDataa(0x42);
  
  SPI_WriteCommm(0x36);       //Memory Access Control
  SPI_WriteDataa(0xc8); //MY,MX,MV,ML,BGR,MH

  
  SPI_WriteCommm(0x3A);       //Pixel Format Set
  SPI_WriteDataa(0x05); //DPI [2:0],DBI [2:0]
  
  SPI_WriteCommm(0xC0);       //Power Control 1
  SPI_WriteDataa(0x15); //VRH[5:0]
  SPI_WriteDataa(0x15); //VC[3:0]
  
  SPI_WriteCommm(0xC1);       //Power Control 2
  SPI_WriteDataa(0x01); //SAP[2:0],BT[3:0]
  
  SPI_WriteCommm(0xC5);       //VCOM
  SPI_WriteDataa(0xEC);//0xFA
 //test=test+2;
  
  SPI_WriteCommm(0xB1);      
  SPI_WriteDataa(0x00);     
  SPI_WriteDataa(0x1B);
  SPI_WriteCommm(0xB4);      
  SPI_WriteDataa(0x02);
  
  SPI_WriteCommm(0xE0);
  SPI_WriteDataa(0x0F);//P01-VP63   
  SPI_WriteDataa(0x13);//P02-VP62   
  SPI_WriteDataa(0x17);//P03-VP61   
  SPI_WriteDataa(0x04);//P04-VP59   
  SPI_WriteDataa(0x13);//P05-VP57   
  SPI_WriteDataa(0x07);//P06-VP50   
  SPI_WriteDataa(0x40);//P07-VP43   
  SPI_WriteDataa(0x39);//P08-VP27,36
  SPI_WriteDataa(0x4F);//P09-VP20   
  SPI_WriteDataa(0x06);//P10-VP13   
  SPI_WriteDataa(0x0D);//P11-VP6    
  SPI_WriteDataa(0x0A);//P12-VP4    
  SPI_WriteDataa(0x1F);//P13-VP2    
  SPI_WriteDataa(0x22);//P14-VP1    
  SPI_WriteDataa(0x00);//P15-VP0    
  
  SPI_WriteCommm(0xE1);
  SPI_WriteDataa(0x00);//P01
  SPI_WriteDataa(0x21);//P02
  SPI_WriteDataa(0x24);//P03
  SPI_WriteDataa(0x03);//P04
  SPI_WriteDataa(0x0F);//P05
  SPI_WriteDataa(0x05);//P06
  SPI_WriteDataa(0x38);//P07
  SPI_WriteDataa(0x32);//P08
  SPI_WriteDataa(0x49);//P09
  SPI_WriteDataa(0x00);//P10
  SPI_WriteDataa(0x09);//P11
  SPI_WriteDataa(0x08);//P12
  SPI_WriteDataa(0x32);//P13
  SPI_WriteDataa(0x35);//P14
  SPI_WriteDataa(0x0F);//P15
  
  SPI_WriteCommm(0x11);//Exit Sleep
  mdelay(120);
  SPI_WriteCommm(0x29);//Display On
	SPI_WriteCommm(0x2C);   //write data
#endif


#if 1
	SPI_WriteCommm(0x01);
	mdelay(10);

	SPI_WriteCommm(0xC8);SPI_WriteDataa(0xFF);SPI_WriteDataa(0x93);SPI_WriteDataa(0x42);
	SPI_WriteCommm(0xB4);SPI_WriteDataa(0x02);
	SPI_WriteCommm(0xB0);SPI_WriteDataa(0xC0);    //RGB
	SPI_WriteCommm(0xF6);SPI_WriteDataa(0x00);SPI_WriteDataa(0x00);SPI_WriteDataa(0x06); //RGB
	SPI_WriteCommm(0x36);SPI_WriteDataa(0xC8);
	SPI_WriteCommm(0x3A);SPI_WriteDataa(0x66);
	SPI_WriteCommm(0xC0);SPI_WriteDataa(0x12);SPI_WriteDataa(0x12);   // c0 0f 0f
	SPI_WriteCommm(0xC1);SPI_WriteDataa(0x01);
	SPI_WriteCommm(0xC5);SPI_WriteDataa(0xcc);   //c8
	
	//SPI_WriteCommm(0xE0);SPI_WriteDataa(0x0F);SPI_WriteDataa(0x00);SPI_WriteDataa(0x08);SPI_WriteDataa(0x05);SPI_WriteDataa(0x08);SPI_WriteDataa(0x1A);SPI_WriteDataa(0x0C);SPI_WriteDataa(0x42);SPI_WriteDataa(0x7A);SPI_WriteDataa(0x54);SPI_WriteDataa(0x08);SPI_WriteDataa(0x08);SPI_WriteDataa(0x08);SPI_WriteDataa(0x23);SPI_WriteDataa(0x25);SPI_WriteDataa(0x0F);
	//SPI_WriteCommm(0xE1);SPI_WriteDataa(0x0F);SPI_WriteDataa(0x00);SPI_WriteDataa(0x2f);SPI_WriteDataa(0x29);SPI_WriteDataa(0x08);SPI_WriteDataa(0x0F);SPI_WriteDataa(0x05);SPI_WriteDataa(0x42);SPI_WriteDataa(0x55);SPI_WriteDataa(0x53);SPI_WriteDataa(0x06);SPI_WriteDataa(0x08);SPI_WriteDataa(0x08);SPI_WriteDataa(0x38);SPI_WriteDataa(0x3A);SPI_WriteDataa(0x0f);

//	SPI_WriteCommm(0x20);
 /*   SPI_WriteCommm(0xE0);  // Positive Gamma Correction

    SPI_WriteDataa(0x08);
    SPI_WriteDataa(0x1C);
    SPI_WriteDataa(0x1B);
    SPI_WriteDataa(0x09);
    SPI_WriteDataa(0x0D);
    SPI_WriteDataa(0x08);

    SPI_WriteDataa(0x4B);
    SPI_WriteDataa(0xB8);
    SPI_WriteDataa(0x3B);
    SPI_WriteDataa(0x08);
    SPI_WriteDataa(0x10);

    SPI_WriteDataa(0x08);
    SPI_WriteDataa(0x20);
    SPI_WriteDataa(0x20);
    SPI_WriteDataa(0x08);

    SPI_WriteCommm(0xE1);  // Negative Gamma Correction

    SPI_WriteDataa(0x07);
    SPI_WriteDataa(0x23);
    SPI_WriteDataa(0x24);
    SPI_WriteDataa(0x06);
    SPI_WriteDataa(0x12);

    SPI_WriteDataa(0x07);
    SPI_WriteDataa(0x34);
    SPI_WriteDataa(0x47);
    SPI_WriteDataa(0x44);
    SPI_WriteDataa(0x07);

    SPI_WriteDataa(0x0F);
    SPI_WriteDataa(0x07);
    SPI_WriteDataa(0x1F);
    SPI_WriteDataa(0x1F);
    SPI_WriteDataa(0x07);*/

	SPI_WriteCommm(0xE0);
  SPI_WriteDataa(0x0F);//P01-VP63   
  SPI_WriteDataa(0x13);//P02-VP62   
  SPI_WriteDataa(0x17);//P03-VP61   
  SPI_WriteDataa(0x04);//P04-VP59   
  SPI_WriteDataa(0x13);//P05-VP57   
  SPI_WriteDataa(0x07);//P06-VP50   
  SPI_WriteDataa(0x40);//P07-VP43   
  SPI_WriteDataa(0x39);//P08-VP27,36
  SPI_WriteDataa(0x4F);//P09-VP20   
  SPI_WriteDataa(0x06);//P10-VP13   
  SPI_WriteDataa(0x0D);//P11-VP6    
  SPI_WriteDataa(0x0A);//P12-VP4    
  SPI_WriteDataa(0x1F);//P13-VP2    
  SPI_WriteDataa(0x22);//P14-VP1    
  SPI_WriteDataa(0x00);//P15-VP0    
  
  SPI_WriteCommm(0xE1);
  SPI_WriteDataa(0x00);//P01
  SPI_WriteDataa(0x21);//P02
  SPI_WriteDataa(0x24);//P03
  SPI_WriteDataa(0x03);//P04
  SPI_WriteDataa(0x0F);//P05
  SPI_WriteDataa(0x05);//P06
  SPI_WriteDataa(0x38);//P07
  SPI_WriteDataa(0x32);//P08
  SPI_WriteDataa(0x49);//P09
  SPI_WriteDataa(0x00);//P10
  SPI_WriteDataa(0x09);//P11
  SPI_WriteDataa(0x08);//P12
  SPI_WriteDataa(0x32);//P13
  SPI_WriteDataa(0x35);//P14
  SPI_WriteDataa(0x0F);//P15

	SPI_WriteCommm(0x11);
	mdelay(20);
	SPI_WriteCommm(0x29);
//	SPI_WriteCommm(0x2C);
#endif
#if 0
SPI_WriteCommm(0x01);
	mdelay(10);

   SPI_WriteCommm(0xB9);  // Set EXTC
    SPI_WriteDataa(0xFF);
    SPI_WriteDataa(0x93);
    SPI_WriteDataa(0x42);

    SPI_WriteCommm(0x36);  // Memory Access Control

    //SPI_WriteDataa(0x08);// MY,MX,MV,ML,BGR,MH bit
    SPI_WriteDataa(0x00);// MY,MX,MV,ML,BGR,MH bit

    SPI_WriteCommm(0x3A);  // Pixel Format Set
    SPI_WriteDataa(0x66);// DPI[2:0]&DBI[2:0]=16 bits / pixel

    SPI_WriteCommm(0xB1);  // Display Waveform Cycle
    SPI_WriteDataa(0x00);// DIVA[1:0],division ratio，0x00=Fosc
    SPI_WriteDataa(0x10);// RTNA[4:0]，Clock per Line，0x10=16 clocks

    SPI_WriteCommm(0xB6);  // Display Function Control
    SPI_WriteDataa(0x0A);// PTG[1:0],PT[1:0] bit
    SPI_WriteDataa(0xE2);// REV,GS,SS,SM,ISC[3:0] bit
    SPI_WriteDataa(0x1D);// NL[5:0],0x1D=240 lines

    SPI_WriteCommm(0xC0);  // Power Control 1
    SPI_WriteDataa(0x28);// Set the VREG1OUT level,VRH[5:0],0x28=4.85V
    SPI_WriteDataa(0x0A);// Sets VCI1 level,VC[3:0],0x2A=2.80V

    SPI_WriteCommm(0xC1);  // Power Control 2
    SPI_WriteDataa(0x02);// SAP[2:0], BT[3:0] VGH/VGL=6/-3

    SPI_WriteCommm(0xC5);  // VCOM Control 1

    SPI_WriteDataa(0x31);// VMH[6:0],0x31=3.925V

    SPI_WriteDataa(0x3C);// VML[6:0],0x3C=-1.000V

    SPI_WriteCommm(0xB8);  // Oscillator Control
    SPI_WriteDataa(0x0A);// FOSC[3:0],0x0A=93Hz

    SPI_WriteCommm(0x26);  // Gamma Set
    SPI_WriteDataa(0x01);// GC[7:0],Gamma curve 2.2

    SPI_WriteCommm(0xC7);  // VCOM Control 2
    SPI_WriteDataa(0xBF);// VMF[6:0],VCOM offset voltage,0xBF=VMH+31,VML+31

    SPI_WriteCommm(0xE0);  // Positive Gamma Correction
    SPI_WriteDataa(0x08);
    SPI_WriteDataa(0x1C);
    SPI_WriteDataa(0x1B);
    SPI_WriteDataa(0x09);
    SPI_WriteDataa(0x0D);
    SPI_WriteDataa(0x08);
    SPI_WriteDataa(0x4B);
    SPI_WriteDataa(0xB8);
    SPI_WriteDataa(0x3B);
    SPI_WriteDataa(0x08);
    SPI_WriteDataa(0x10);
    SPI_WriteDataa(0x08);
    SPI_WriteDataa(0x20);
    SPI_WriteDataa(0x20);
    SPI_WriteDataa(0x08);

    SPI_WriteCommm(0xE1);  // Negative Gamma Correction
    SPI_WriteDataa(0x07);
    SPI_WriteDataa(0x23);
    SPI_WriteDataa(0x24);
    SPI_WriteDataa(0x06);
    SPI_WriteDataa(0x12);
    SPI_WriteDataa(0x07);
    SPI_WriteDataa(0x34);
    SPI_WriteDataa(0x47);
    SPI_WriteDataa(0x44);
    SPI_WriteDataa(0x07);
    SPI_WriteDataa(0x0F);
    SPI_WriteDataa(0x07);
    SPI_WriteDataa(0x1F);
    SPI_WriteDataa(0x1F);
    SPI_WriteDataa(0x07);

    SPI_WriteCommm(0x11);  // Exit Sleep
    mdelay(80);
    SPI_WriteCommm(0x11);  // Exit Sleep
    mdelay(80);
    SPI_WriteCommm(0x29);  // Display On

    SPI_WriteCommm(0x2C);
#endif
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
