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
#include <mmc.h>
//#include <pwm.h>
#include <asm/io.h>
#include <asm/gpio.h>

#include <platform.h>
#include <mach-api.h>
#include <nxp_rtc.h>

#include <draw_lcd.h>

#include "fastboot.h"
#if defined(CONFIG_PMIC)
#include <power/pmic.h>
#include <power/battery.h>
#include <asm/arch/nxe2000-private.h>
#if defined(CONFIG_PMIC_NXE2000)
#include <nxe2000_power.h>
#endif
#include <i2c.h>
#include <errno.h>
#endif

DECLARE_GLOBAL_DATA_PTR;

#include "eth.c"

#if (0)
#define DBGOUT(msg...)		{ printf("BD: " msg); }
#else
#define DBGOUT(msg...)		do {} while (0)
#endif



/*------------------------------------------------------------------------------
 * intialize nexell soc and board status.
 */

static void set_gpio_strenth(U32 Group, U32 BitNumber, U32 mA)
{
	U32 drv1=0, drv0=0;
	U32 drv1_value, drv0_value;

	switch( mA )
	{
		case 0 : drv0 = 0; drv1 = 0; break;
		case 1 : drv0 = 0; drv1 = 1; break;
		case 2 : drv0 = 1; drv1 = 0; break;
		case 3 : drv0 = 1; drv1 = 1; break;
		default: drv0 = 0; drv1 = 0; break;
	}
	DBGOUT("DRV Strength : GRP : i %x Bit: %x  ma :%d  \n",
				Group, BitNumber, mA);

	drv1_value = NX_GPIO_GetDRV1(Group) & ~(1 << BitNumber);
	drv0_value = NX_GPIO_GetDRV0(Group) & ~(1 << BitNumber);

	if (drv1) drv1_value |= (drv1 << BitNumber);
	if (drv0) drv0_value |= (drv0 << BitNumber);

	DBGOUT(" Value : drv1 :%8x  drv0 %8x \n ",drv1_value, drv0_value);

	NX_GPIO_SetDRV0 ( Group, drv0_value );
	NX_GPIO_SetDRV1 ( Group, drv1_value );
}

static void bd_gpio_init(void)
{
	int index, bit;
	int mode, func, out, lv, plup, stren;
	U32 gpio;

	const U32 pads[NUMBER_OF_GPIO_MODULE][32] = {
	{	/* GPIO_A */
	PAD_GPIOA0 , PAD_GPIOA1 , PAD_GPIOA2 , PAD_GPIOA3 , PAD_GPIOA4 , PAD_GPIOA5 , PAD_GPIOA6 , PAD_GPIOA7 , PAD_GPIOA8 , PAD_GPIOA9 ,
	PAD_GPIOA10, PAD_GPIOA11, PAD_GPIOA12, PAD_GPIOA13, PAD_GPIOA14, PAD_GPIOA15, PAD_GPIOA16, PAD_GPIOA17, PAD_GPIOA18, PAD_GPIOA19,
	PAD_GPIOA20, PAD_GPIOA21, PAD_GPIOA22, PAD_GPIOA23, PAD_GPIOA24, PAD_GPIOA25, PAD_GPIOA26, PAD_GPIOA27, PAD_GPIOA28, PAD_GPIOA29,
	PAD_GPIOA30, PAD_GPIOA31
	}, { /* GPIO_B */
	PAD_GPIOB0 , PAD_GPIOB1 , PAD_GPIOB2 , PAD_GPIOB3 , PAD_GPIOB4 , PAD_GPIOB5 , PAD_GPIOB6 , PAD_GPIOB7 , PAD_GPIOB8 , PAD_GPIOB9 ,
	PAD_GPIOB10, PAD_GPIOB11, PAD_GPIOB12, PAD_GPIOB13, PAD_GPIOB14, PAD_GPIOB15, PAD_GPIOB16, PAD_GPIOB17, PAD_GPIOB18, PAD_GPIOB19,
	PAD_GPIOB20, PAD_GPIOB21, PAD_GPIOB22, PAD_GPIOB23, PAD_GPIOB24, PAD_GPIOB25, PAD_GPIOB26, PAD_GPIOB27, PAD_GPIOB28, PAD_GPIOB29,
	PAD_GPIOB30, PAD_GPIOB31
	}, { /* GPIO_C */
	PAD_GPIOC0 , PAD_GPIOC1 , PAD_GPIOC2 , PAD_GPIOC3 , PAD_GPIOC4 , PAD_GPIOC5 , PAD_GPIOC6 , PAD_GPIOC7 , PAD_GPIOC8 , PAD_GPIOC9 ,
	PAD_GPIOC10, PAD_GPIOC11, PAD_GPIOC12, PAD_GPIOC13, PAD_GPIOC14, PAD_GPIOC15, PAD_GPIOC16, PAD_GPIOC17, PAD_GPIOC18, PAD_GPIOC19,
	PAD_GPIOC20, PAD_GPIOC21, PAD_GPIOC22, PAD_GPIOC23, PAD_GPIOC24, PAD_GPIOC25, PAD_GPIOC26, PAD_GPIOC27, PAD_GPIOC28, PAD_GPIOC29,
	PAD_GPIOC30, PAD_GPIOC31
	}, { /* GPIO_D */
	PAD_GPIOD0 , PAD_GPIOD1 , PAD_GPIOD2 , PAD_GPIOD3 , PAD_GPIOD4 , PAD_GPIOD5 , PAD_GPIOD6 , PAD_GPIOD7 , PAD_GPIOD8 , PAD_GPIOD9 ,
	PAD_GPIOD10, PAD_GPIOD11, PAD_GPIOD12, PAD_GPIOD13, PAD_GPIOD14, PAD_GPIOD15, PAD_GPIOD16, PAD_GPIOD17, PAD_GPIOD18, PAD_GPIOD19,
	PAD_GPIOD20, PAD_GPIOD21, PAD_GPIOD22, PAD_GPIOD23, PAD_GPIOD24, PAD_GPIOD25, PAD_GPIOD26, PAD_GPIOD27, PAD_GPIOD28, PAD_GPIOD29,
	PAD_GPIOD30, PAD_GPIOD31
	}, { /* GPIO_E */
	PAD_GPIOE0 , PAD_GPIOE1 , PAD_GPIOE2 , PAD_GPIOE3 , PAD_GPIOE4 , PAD_GPIOE5 , PAD_GPIOE6 , PAD_GPIOE7 , PAD_GPIOE8 , PAD_GPIOE9 ,
	PAD_GPIOE10, PAD_GPIOE11, PAD_GPIOE12, PAD_GPIOE13, PAD_GPIOE14, PAD_GPIOE15, PAD_GPIOE16, PAD_GPIOE17, PAD_GPIOE18, PAD_GPIOE19,
	PAD_GPIOE20, PAD_GPIOE21, PAD_GPIOE22, PAD_GPIOE23, PAD_GPIOE24, PAD_GPIOE25, PAD_GPIOE26, PAD_GPIOE27, PAD_GPIOE28, PAD_GPIOE29,
	PAD_GPIOE30, PAD_GPIOE31
	},
	};

	/* GPIO pad function */
	for (index = 0; NUMBER_OF_GPIO_MODULE > index; index++) {

		NX_GPIO_ClearInterruptPendingAll(index);

		for (bit = 0; 32 > bit; bit++) {
			gpio  = pads[index][bit];
			func  = PAD_GET_FUNC(gpio);
			mode  = PAD_GET_MODE(gpio);
			lv    = PAD_GET_LEVEL(gpio);
			stren = PAD_GET_STRENGTH(gpio);
			plup  = PAD_GET_PULLUP(gpio);

			/* get pad alternate function (0,1,2,4) */
			switch (func) {
			case PAD_GET_FUNC(PAD_FUNC_ALT0): func = NX_GPIO_PADFUNC_0;	break;
			case PAD_GET_FUNC(PAD_FUNC_ALT1): func = NX_GPIO_PADFUNC_1;	break;
			case PAD_GET_FUNC(PAD_FUNC_ALT2): func = NX_GPIO_PADFUNC_2;	break;
			case PAD_GET_FUNC(PAD_FUNC_ALT3): func = NX_GPIO_PADFUNC_3;	break;
			default: printf("ERROR, unknown alt func (%d.%02d=%d)\n", index, bit, func);
				continue;
			}

			switch (mode) {
			case PAD_GET_MODE(PAD_MODE_ALT): out = 0;
			case PAD_GET_MODE(PAD_MODE_IN ): out = 0;
			case PAD_GET_MODE(PAD_MODE_INT): out = 0; break;
			case PAD_GET_MODE(PAD_MODE_OUT): out = 1; break;
			default: printf("ERROR, unknown io mode (%d.%02d=%d)\n", index, bit, mode);
				continue;
			}

			NX_GPIO_SetPadFunction(index, bit, func);
			NX_GPIO_SetOutputEnable(index, bit, (out ? CTRUE : CFALSE));
			NX_GPIO_SetOutputValue(index, bit,  (lv  ? CTRUE : CFALSE));
			NX_GPIO_SetInterruptMode(index, bit, (lv));

			NX_GPIO_SetPullMode(index, bit, plup);
			set_gpio_strenth(index, bit, stren); /* pad strength */
		}

		NX_GPIO_SetDRV0_DISABLE_DEFAULT(index, 0xFFFFFFFF);
		NX_GPIO_SetDRV1_DISABLE_DEFAULT(index, 0xFFFFFFFF);
	}
}

static void bd_alive_init(void)
{
	int index, bit;
	int mode, out, lv, plup, detect;
	U32 gpio;

	const U32 pads[] = {
	PAD_GPIOALV0, PAD_GPIOALV1, PAD_GPIOALV2,
	PAD_GPIOALV3, PAD_GPIOALV4, PAD_GPIOALV5
	};

	index = sizeof(pads)/sizeof(pads[0]);

	/* Alive pad function */
	for (bit = 0; index > bit; bit++) {
		NX_ALIVE_ClearInterruptPending(bit);
		gpio = pads[bit];
		mode = PAD_GET_MODE(gpio);
		lv   = PAD_GET_LEVEL(gpio);
		plup = PAD_GET_PULLUP(gpio);

		switch (mode) {
		case PAD_GET_MODE(PAD_MODE_IN ):
		case PAD_GET_MODE(PAD_MODE_INT): out = 0; break;
		case PAD_GET_MODE(PAD_MODE_OUT): out = 1; break;
		case PAD_GET_MODE(PAD_MODE_ALT):
			printf("ERROR, alive.%d not support alt function\n", bit);
			continue;
		default :
			printf("ERROR, unknown alive mode (%d=%d)\n", bit, mode);
			continue;
		}

		NX_ALIVE_SetOutputEnable(bit, (out ? CTRUE : CFALSE));
		NX_ALIVE_SetOutputValue (bit, (lv));
		NX_ALIVE_SetPullUpEnable(bit, (plup & 1 ? CTRUE : CFALSE));
		/* set interrupt mode */
		for (detect = 0; 6 > detect; detect++) {
			if (mode == PAD_GET_MODE(PAD_MODE_INT))
				NX_ALIVE_SetDetectMode(detect, bit, (lv == detect ? CTRUE : CFALSE));
			else
				NX_ALIVE_SetDetectMode(detect, bit, CFALSE);
		}
		NX_ALIVE_SetDetectEnable(bit, (mode == PAD_MODE_INT ? CTRUE : CFALSE));
	}
}


/* call from u-boot */
int board_early_init_f(void)
{
	bd_gpio_init();
	bd_alive_init();

#if defined(CONFIG_NXP_RTC_USE)
	nxp_rtc_init();
#endif
	return 0;
}

int board_init(void)
{
	DBGOUT("%s : done board init ...\n", CFG_SYS_BOARD_NAME);
	return 0;
}


extern void	bd_display(void);

static void auto_update(int io, int wait)
{
	unsigned int grp = PAD_GET_GROUP(io);
	unsigned int bit = PAD_GET_BITNO(io);
	int level = 1, i = 0;
	char *cmd = "fastboot";

	for (i = 0; wait > i; i++) {
		switch (io & ~(32-1)) {
		case PAD_GPIO_A:
		case PAD_GPIO_B:
		case PAD_GPIO_C:
		case PAD_GPIO_D:
		case PAD_GPIO_E:
			level = NX_GPIO_GetInputValue(grp, bit);	break;
		case PAD_GPIO_ALV:
			level = NX_ALIVE_GetInputValue(bit);	break;
		};
		if (level)
			break;
		mdelay(1);
	}

	if (i == wait)
		run_command (cmd, 0);
}

#define	UPDATE_KEY			(PAD_GPIO_ALV + 0)
#define	UPDATE_CHECK_TIME	(3000)	/* ms */



int board_late_init(void)
{
#if defined(CONFIG_SYS_MMC_BOOT_DEV)
	char boot[16];
	sprintf(boot, "mmc dev %d", CONFIG_SYS_MMC_BOOT_DEV);
	run_command(boot, 0);
#endif

#if defined(CONFIG_DISPLAY_OUT)
	run_command(CONFIG_CMD_LOGO_WALLPAPERS, 0);

	lcd_draw_boot_logo(CONFIG_FB_ADDR,
			CFG_DISP_PRI_RESOL_WIDTH,
			CFG_DISP_PRI_RESOL_HEIGHT,
			CFG_DISP_PRI_SCREEN_PIXEL_BYTE);

	bd_display();

	/* backlight */
	pwm_init(CFG_LCD_PRI_PWM_CH, 0, 0);
	pwm_config(CFG_LCD_PRI_PWM_CH,
		TO_DUTY_NS(CFG_LCD_PRI_PWM_DUTYCYCLE, CFG_LCD_PRI_PWM_FREQ),
		TO_PERIOD_NS(CFG_LCD_PRI_PWM_FREQ));
#endif

	/* Temp check gpio to update */
	auto_update(UPDATE_KEY, UPDATE_CHECK_TIME);

	return 0;
}


#ifdef CONFIG_FASTBOOT

#define	LOGO_BGCOLOR	(0xffffff)
static int _logo_left   = CFG_DISP_PRI_RESOL_WIDTH /2 +  50;
static int _logo_top    = CFG_DISP_PRI_RESOL_HEIGHT/2 + 180;
static int _logo_width  = 8*24;
static int _logo_height = 16;

void fboot_lcd_start(void)
{
	lcd_info lcd = {
		.fb_base		= CONFIG_FB_ADDR,
		.bit_per_pixel	= CFG_DISP_PRI_SCREEN_PIXEL_BYTE * 8,
		.lcd_width		= CFG_DISP_PRI_RESOL_WIDTH,
		.lcd_height		= CFG_DISP_PRI_RESOL_HEIGHT,
		.back_color		= LOGO_BGCOLOR,
		.text_color		= 0xFF,
		.alphablend		= 0,
	};
	lcd_debug_init(&lcd);

	// clear FB
	memset((void*)CONFIG_FB_ADDR, 0xFF,
		CFG_DISP_PRI_RESOL_WIDTH * CFG_DISP_PRI_RESOL_HEIGHT *
		CFG_DISP_PRI_SCREEN_PIXEL_BYTE);

	run_command(CONFIG_CMD_LOGO_UPDATE, 0);
	lcd_draw_text("wait for update", _logo_left, _logo_top, 2, 2, 0);
}

void fboot_lcd_stop(void)
{
	run_command(CONFIG_CMD_LOGO_WALLPAPERS, 0);
}

void fboot_lcd_part(char *part, char *stat)
{
	int s = 2;
	int l = _logo_left, t = _logo_top;
	int w = (_logo_width*s), h = (_logo_height*s);
	unsigned bg = LOGO_BGCOLOR;

	lcd_fill_rectangle(l, t, w, h, bg, 0);
	lcd_draw_string(l, t, s, s, 0, "%s: %s", part, stat);
}

void fboot_lcd_down(int percent)
{
	int s = 2;
	int l = _logo_left, t = _logo_top;
	int w = (_logo_width*s), h = (_logo_height*s);
	unsigned bg = LOGO_BGCOLOR;

	lcd_fill_rectangle(l, t, w, h, bg, 0);
	lcd_draw_string(l, t, s, s, 0, "down %d%%", percent);
}

void fboot_lcd_flash(char *part, char *stat)
{
	int s = 2;
	int l = _logo_left, t = _logo_top;
	int w = (_logo_width*s), h = (_logo_height*s);
	unsigned bg = LOGO_BGCOLOR;

	lcd_fill_rectangle(l, t, w, h, bg, 0);
	lcd_draw_string(l, t, s, s, 0, "%s: %s", part, stat);
}

void fboot_lcd_status(char *stat)
{
	int s = 2;
	int l = _logo_left, t = _logo_top;
	int w = (_logo_width*s), h = (_logo_height*s);
	unsigned bg = LOGO_BGCOLOR;

	lcd_fill_rectangle(l, t, w, h, bg, 0);
	lcd_draw_string(l, t, s, s, 0, "%s", stat);
}

#endif

