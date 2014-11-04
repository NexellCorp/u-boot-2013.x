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
#include <pwm.h>
#include <asm/io.h>
#include <asm/gpio.h>

#include <platform.h>
#include <mach-api.h>
#include <nxp_rtc.h>
#include <pm.h>

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

#if defined(CONFIG_BAT_CHECK)
#define CFG_KEY_POWER       (PAD_GPIO_ALV + 0)
#endif


/*------------------------------------------------------------------------------
 * intialize nexell soc and board status.
 */
#if defined(CONFIG_PMIC_NXE2000)
struct nxe2000_power	nxe_power_config;
#endif
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

int bd_pmic_init(void)
{
#if defined(CONFIG_PMIC_NXE2000)
	nxe_power_config.i2c_addr	= (0x64>>1);
	nxe_power_config.i2c_bus	= CONFIG_NXE2000_I2C_BUS;

	nxe_power_config.policy.ldo.ldo_1_out_vol = NXE2000_DEF_LDO1_VOL;
	nxe_power_config.policy.ldo.ldo_2_out_vol = NXE2000_DEF_LDO2_VOL;
	nxe_power_config.policy.ldo.ldo_3_out_vol = NXE2000_DEF_LDO3_VOL;
	nxe_power_config.policy.ldo.ldo_4_out_vol = NXE2000_DEF_LDO4_VOL;
	nxe_power_config.policy.ldo.ldo_5_out_vol = NXE2000_DEF_LDO5_VOL;
	nxe_power_config.policy.ldo.ldo_6_out_vol = NXE2000_DEF_LDO6_VOL;
	nxe_power_config.policy.ldo.ldo_7_out_vol = NXE2000_DEF_LDO7_VOL;
	nxe_power_config.policy.ldo.ldo_8_out_vol = NXE2000_DEF_LDO8_VOL;
	nxe_power_config.policy.ldo.ldo_9_out_vol = NXE2000_DEF_LDO9_VOL;
	nxe_power_config.policy.ldo.ldo_10_out_vol = NXE2000_DEF_LDO10_VOL;
	nxe_power_config.policy.ldo.ldo_rtc1_out_vol = NXE2000_DEF_LDORTC1_VOL;
	nxe_power_config.policy.ldo.ldo_rtc2_out_vol = NXE2000_DEF_LDORTC2_VOL;

	nxe_power_config.policy.ldo.ldo_1_out_enb = 1;
	nxe_power_config.policy.ldo.ldo_2_out_enb = 1;
	nxe_power_config.policy.ldo.ldo_3_out_enb = 1;
	nxe_power_config.policy.ldo.ldo_4_out_enb = 1;
	nxe_power_config.policy.ldo.ldo_5_out_enb = 1;
	nxe_power_config.policy.ldo.ldo_6_out_enb = 1;
	nxe_power_config.policy.ldo.ldo_7_out_enb = 1;
	nxe_power_config.policy.ldo.ldo_8_out_enb = 1;
	nxe_power_config.policy.ldo.ldo_9_out_enb = 1;
	nxe_power_config.policy.ldo.ldo_10_out_enb = 1;
	nxe_power_config.policy.ldo.ldo_rtc1_out_enb = 1;
	nxe_power_config.policy.ldo.ldo_rtc2_out_enb = 1;

	nxe_power_config.policy.dcdc.ddc_1_out_vol = NXE2000_DEF_DDC1_VOL;
	nxe_power_config.policy.dcdc.ddc_2_out_vol = NXE2000_DEF_DDC2_VOL;
	nxe_power_config.policy.dcdc.ddc_3_out_vol = NXE2000_DEF_DDC3_VOL;
	nxe_power_config.policy.dcdc.ddc_4_out_vol = NXE2000_DEF_DDC4_VOL;
	nxe_power_config.policy.dcdc.ddc_5_out_vol = NXE2000_DEF_DDC5_VOL;

	nxe_power_config.policy.dcdc.ddc_1_out_enb = 1;
	nxe_power_config.policy.dcdc.ddc_2_out_enb = 1;
	nxe_power_config.policy.dcdc.ddc_3_out_enb = 1;
	nxe_power_config.policy.dcdc.ddc_4_out_enb = 1;
	nxe_power_config.policy.dcdc.ddc_5_out_enb = 1;

	nxe2000_device_setup(&nxe_power_config);
#endif  // #if defined(CONFIG_PMIC_NXE2000)

	return 0;
}

/* call from u-boot */
int board_early_init_f(void)
{
	bd_gpio_init();
	bd_alive_init();
#ifndef CONFIG_NXE2000_REG_DUMP
	bd_pmic_init();
#endif
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

#if defined(CONFIG_BAT_CHECK)
static int pmic_init_nxe2000(void)
{
    struct pmic *p = pmic_get("PMIC_NXE2000");
    u32 val;
    int ret = 0;

    if (pmic_probe(p))
        return -1;

    if (GPIO_OTG_USBID_DET > -1)
    {
        gpio_direction_input(GPIO_OTG_USBID_DET);
    }

    if (GPIO_OTG_VBUS_DET > -1)
    {
        gpio_direction_output(GPIO_OTG_VBUS_DET, 0);
    }

    if (GPIO_PMIC_VUSB_DET > -1)
    {
        gpio_direction_input(GPIO_PMIC_VUSB_DET);
    }

    if (GPIO_PMIC_LOWBAT_DET > -1)
    {
        gpio_direction_input(GPIO_PMIC_LOWBAT_DET);
    }

    val = (NXE2000_POS_ADCCNT3_ADRQ_SINGLE | NXE2000_POS_ADCCNT3_ADSEL_VBAT);
    pmic_reg_write(p, NXE2000_REG_ADCCNT3, val);

    val = (1 << NXE2000_POS_ADCCNT1_VBATSEL);
    pmic_reg_write(p, NXE2000_REG_ADCCNT1, val);

    if (ret) {
        puts("NXE2000 PMIC setting error!\n");
        return -1;
    }
    return 0;
}
#endif  /* CONFIG_BAT_CHECK */

int power_init_board(void)
{
	int ret;

#ifdef CONFIG_NXE2000_REG_DUMP
	bd_pmic_init();
#endif

	ret = pmic_init(CONFIG_NXE2000_I2C_BUS);
#if defined(CONFIG_BAT_CHECK)
	ret |= pmic_init_nxe2000();
	ret |= power_bat_init(CONFIG_NXE2000_I2C_BUS);
#endif
	ret |= power_fg_init(CONFIG_NXE2000_I2C_BUS);
	if (ret)
		return ret;

	ret = power_muic_init(CONFIG_NXE2000_I2C_BUS);

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

static void bd_display_run(char *cmd, int bl_duty, int bl_on)
{
	static int display_init = 0;

	if (cmd) {
		run_command(cmd, 0);
		lcd_draw_boot_logo(CONFIG_FB_ADDR, CFG_DISP_PRI_RESOL_WIDTH,
			CFG_DISP_PRI_RESOL_HEIGHT, CFG_DISP_PRI_SCREEN_PIXEL_BYTE);
	}

	if (!display_init) {
		bd_display();
		pwm_init(CFG_LCD_PRI_PWM_CH, 0, 0);
		display_init = 1;
	}

	pwm_config(CFG_LCD_PRI_PWM_CH,
		TO_DUTY_NS(bl_duty, CFG_LCD_PRI_PWM_FREQ),
		TO_PERIOD_NS(CFG_LCD_PRI_PWM_FREQ));

	if (bl_on)
		pwm_enable(CFG_LCD_PRI_PWM_CH);
}

#define	UPDATE_KEY			(PAD_GPIO_ALV + 0)
#define	UPDATE_CHECK_TIME	(3000)	/* ms */

#if defined(CONFIG_BAT_CHECK)
#if !defined (CONFIG_PMIC_VOLTAGE_CHECK_WITH_CHARGE)
extern u32 chgctl_reg_val;
#endif

int board_late_init(void)
{
#if defined(CONFIG_DISPLAY_OUT)
	lcd_info lcd = {
		.fb_base		= CONFIG_FB_ADDR,
		.bit_per_pixel	= CFG_DISP_PRI_SCREEN_PIXEL_BYTE * 8,
		.lcd_width		= CFG_DISP_PRI_RESOL_WIDTH,
		.lcd_height		= CFG_DISP_PRI_RESOL_HEIGHT,
		.back_color		= 0x000000,
		.text_color		= (0<<16) + (255 << 8) + (255),
		.dbg_win_left	= 10,
		.dbg_win_width	= 1260,
		.dbg_win_top	= 10,
		.dbg_win_height	= 780,
		.alphablend		= 0,
	};

	int chrg;
	int shutdown_ilim_uV = NXE2000_DEF_LOWBAT_BATTERY_VOL;
	int bl_duty = CFG_LCD_PRI_PWM_DUTYCYCLE;
	u32 chg_state=0, poweron_his, poweroff_his;
	//u32 chg_led_mode = 0;
	struct power_battery *pb;
	struct pmic *p_fg, *p_chrg, *p_muic, *p_bat;
	int show_bat_state = 0;
	int power_key_depth = 0;
	u32 time_key_pev = 0;
	unsigned int sum_voltage=0, avg_voltage=0;
	int i=0;
	//u8 is_pwr_in;
	u8 power_state = 0;
	u8 power_depth = 3;


#if defined(CONFIG_SYS_MMC_BOOT_DEV)
	char boot[16];
	sprintf(boot, "mmc dev %d", CONFIG_SYS_MMC_BOOT_DEV);
	run_command(boot, 0);
#endif

	power_key_depth = nxp_gpio_get_int_pend(CFG_KEY_POWER);
	nxp_gpio_set_int_clear(CFG_KEY_POWER);
	if (power_key_depth)
		time_key_pev = nxp_rtc_get();

	p_fg = pmic_get("FG_NXE2000");
	if (!p_fg) {
		puts("FG_NXE2000: Not found\n");
		return -ENODEV;
	}

	p_chrg = pmic_get("PMIC_NXE2000");
	if (!p_chrg) {
		puts("PMIC_NXE2000: Not found\n");
		return -ENODEV;
	}

	p_muic = pmic_get("MUIC_NXE2000");
	if (!p_muic) {
		puts("MUIC_NXE2000: Not found\n");
	}

	p_bat = pmic_get("BAT_NXE2000");
	if (!p_bat) {
		puts("BAT_NXE2000: Not found\n");
		return -ENODEV;
	}

	p_fg->parent    = p_bat;
	p_chrg->parent  = p_bat;

	if(p_muic)
		p_muic->parent  = p_bat;

	//    p_bat->low_power_mode = nxe2000_low_power_mode;
	p_bat->low_power_mode = NULL;
	p_bat->pbat->battery_init(p_bat, p_fg, p_chrg, p_muic);
	pb = p_bat->pbat;

	if(p_muic)
		chrg = p_muic->chrg->chrg_type(p_muic, 1);
	else
		chrg = p_chrg->chrg->chrg_type(p_chrg, 1);

	if (!p_chrg->chrg->chrg_bat_present(p_chrg))
	{
		puts("No battery detected\n");
		return -1;
	}

	/*===========================================================*/
#if !defined (CONFIG_PMIC_VOLTAGE_CHECK_WITH_CHARGE)
	//if(CHARGER_NO < chrg || chrg < CHARGER_UNKNOWN)
	//{
	//	pmic_reg_read(p_chrg, NXE2000_REG_GPLED_FUNC, &chg_led_mode);
	//	chg_led_mode |= 0x07;
	//	pmic_reg_write(p_chrg, NXE2000_REG_GPLED_FUNC, chg_led_mode);
	//}
	pmic_reg_read(p_chrg, NXE2000_REG_CHGCTL1, &chgctl_reg_val);
	pmic_reg_write(p_chrg, NXE2000_REG_CHGCTL1, (chgctl_reg_val & ~0x0B));
#endif

	for(i=0; i<3; i++)
	{
		mdelay(10);
		p_fg->fg->fg_battery_check(p_fg, p_bat);
		sum_voltage += pb->bat->voltage_uV;
	}
	avg_voltage = sum_voltage/3;

	pmic_reg_read(p_chrg, NXE2000_REG_CHGSTATE, &chg_state);
	pmic_reg_read(p_chrg, NXE2000_REG_PONHIS, &poweron_his);
	pmic_reg_read(p_chrg, NXE2000_REG_POFFHIS, &poweroff_his);

	if(chrg == CHARGER_USB)
	{
		shutdown_ilim_uV = NXE2000_DEF_LOWBAT_USB_PC_VOL;
	}
	else if(chrg == CHARGER_TA)
	{
		shutdown_ilim_uV = NXE2000_DEF_LOWBAT_ADP_VOL;
	}
	else
	{
		shutdown_ilim_uV = NXE2000_DEF_LOWBAT_BATTERY_VOL;
	}

	if (GPIO_PMIC_VUSB_DET > -1)
	{
		printf("VUSB_DET     : %d\n", gpio_get_value(GPIO_PMIC_VUSB_DET));
	}

	if (GPIO_PMIC_LOWBAT_DET > -1)
	{
		printf("BAT_DET      : %d\n", gpio_get_value(GPIO_PMIC_LOWBAT_DET));
	}

	printf("poweroff_his : 0x%02x \n", poweroff_his);
	printf("poweron_his  : 0x%02x \n", poweron_his);
	printf("chg_state    : 0x%02x,    chrg_type        : %d\n", chg_state, chrg);
	printf("avg_voltage  : %d, shutdown_ilim_uV : %d\n", avg_voltage, shutdown_ilim_uV);

	if(avg_voltage < NXE2000_DEF_CUTOFF_VOL)
	{
		printf("Power Cut Off Vol : %dmV\n", avg_voltage/1000);
		goto enter_shutdown;
	}
	else if(avg_voltage < shutdown_ilim_uV)
	{
		bl_duty = (CFG_LCD_PRI_PWM_DUTYCYCLE / 2);
		show_bat_state = 2;
		power_key_depth = 0;
	}
	else if(chrg == CHARGER_NO || chrg == CHARGER_UNKNOWN)
	{
		show_bat_state = 0;
		power_key_depth = 2;
	}
	else
	{
		bl_duty = (CFG_LCD_PRI_PWM_DUTYCYCLE / 2);
		show_bat_state = 1;
		power_key_depth = 0;
	}

	/*===========================================================*/
#if defined CONFIG_RECOVERY_BOOT
    if (RECOVERY_SIGNATURE == readl(SCR_RESET_SIG_READ)) {
        writel((-1UL), SCR_RESET_SIG_RESET); /* clear */

        printf("RECOVERY BOOT\n");
        bd_display_run(CONFIG_CMD_LOGO_WALLPAPERS, CFG_LCD_PRI_PWM_DUTYCYCLE, 1);
        run_command(CONFIG_CMD_RECOVERY_BOOT, 0);	/* recovery boot */
    }
    writel((-1UL), SCR_RESET_SIG_RESET);
#endif

	/*===========================================================*/
    if (power_key_depth > 1)
    {
        bd_display_run(CONFIG_CMD_LOGO_WALLPAPERS, bl_duty, 1);
        goto skip_bat_animation;
    }
    else if (show_bat_state)
    {
        memset((void*)lcd.fb_base, 0, lcd.lcd_width * lcd.lcd_height * (lcd.bit_per_pixel/8));
        bd_display_run(CONFIG_CMD_LOGO_BATTERY, bl_duty, 1);
    }

	/*===========================================================*/
	// draw charing image
	if (show_bat_state)
	{
		int lcdw = lcd.lcd_width, lcdh = lcd.lcd_height;
		int bmpw = 240, bmph = 320;
		int bw = 82, bh = 55;
		int bx = 79, by = 60+4;
		int sx, sy, dy, str_dy, clr_str_size;
		unsigned int color = (54<<16) + (221 << 8) + (19);
		int i = 0;
		u32 time_pwr_prev;
		char *str_charging		= " Charging...   ";
		char *str_discharging	= " Discharging...";
		char *str_lowbatt  		= " Low Battery...";

		clr_str_size = max(strlen(str_charging), strlen(str_lowbatt));

		sx = (lcdw - bmpw)/2 + bx;
		sy = (lcdh - bmph)/2 + by;
		dy = sy + (bh+4)*3;

		lcd_debug_init(&lcd);
		lcd_draw_text(str_charging, (lcdw - strlen(str_charging)*8*3)/2 + 30, dy+100, 3, 3, 0);
		str_dy = dy;

		time_pwr_prev = nxp_rtc_get();

		while(!ctrlc())
		{
			if (nxp_gpio_get_int_pend(CFG_KEY_POWER))
				power_key_depth++;
			else
				power_key_depth = 0;

			nxp_gpio_set_int_clear(CFG_KEY_POWER);

			p_fg->fg->fg_battery_check(p_fg, p_bat);

			if(p_muic)
				chrg = p_muic->chrg->chrg_type(p_muic, 0);
			else
				chrg = p_chrg->chrg->chrg_type(p_chrg, 0);

			if(chrg == CHARGER_USB)
			{
				shutdown_ilim_uV = NXE2000_DEF_LOWBAT_USB_PC_VOL;
			}
			else if(chrg == CHARGER_TA)
			{
				shutdown_ilim_uV = NXE2000_DEF_LOWBAT_ADP_VOL;
			}
			else
			{
				shutdown_ilim_uV = NXE2000_DEF_LOWBAT_BATTERY_VOL;
			}

			if (nxp_rtc_get() > (time_pwr_prev + 4))
			{
				time_pwr_prev = nxp_rtc_get();
				bl_duty = (bl_duty >> 1);
				if(power_depth > 0)
				{
					bd_display_run(NULL, bl_duty, 1);
					power_depth--;
				}
				else
				{
					bd_display_run(NULL, 0, 0);
					memset((void*)lcd.fb_base, 0, lcd.lcd_width * lcd.lcd_height * (lcd.bit_per_pixel/8));
				}
			}

			if (power_key_depth > 1)
			{
				if (pb->bat->voltage_uV >= shutdown_ilim_uV)
				{
					break;
				}
			}

			if(!power_depth)
			{
				goto enter_shutdown;
			}

			/* Draw battery status */
			if (power_depth > 0)
			{
				i++;
				if(i<5)
				{
					lcd_fill_rectangle(sx, dy, bw, bh, color, 0);
					dy -= (bh+4);
				}
				else
				{
					dy = sy + (bh+4)*3;
					lcd_fill_rectangle(sx, sy, bw, (bh+4)*4, 0x0, 0);
					i = 0;
				}

				if(show_bat_state == 2)
					lcd_draw_text(str_lowbatt, (lcdw - strlen(str_lowbatt)*8*3)/2 + 30, str_dy+100, 3, 3, 0);
				else
				{
					if(chrg == CHARGER_NO || chrg == CHARGER_UNKNOWN)
						lcd_draw_text(str_discharging, (lcdw - strlen(str_discharging)*8*3)/2 + 30, str_dy+100, 3, 3, 0);
					else
						lcd_draw_text(str_charging, (lcdw - strlen(str_charging)*8*3)/2 + 30, str_dy+100, 3, 3, 0);
				}

			}

			mdelay(1000);
		}

		bd_display_run(CONFIG_CMD_LOGO_WALLPAPERS, CFG_LCD_PRI_PWM_DUTYCYCLE, 1);
	}

skip_bat_animation:

#if !defined (CONFIG_PMIC_VOLTAGE_CHECK_WITH_CHARGE)
	pmic_reg_write(p_chrg, NXE2000_REG_CHGCTL1, chgctl_reg_val);
	//pmic_reg_read(p_chrg, NXE2000_REG_GPLED_FUNC, &chg_led_mode);
	//chg_led_mode &= ~0x04;
	//pmic_reg_write(p_chrg, NXE2000_REG_GPLED_FUNC, chg_led_mode);
#endif	/* CONFIG_PMIC_VOLTAGE_CHECK_WITH_CHARGE */
#endif  /* CONFIG_DISPLAY_OUT */

	if(p_muic)
		chrg = p_muic->chrg->chrg_type(p_muic, 1);
	else
		chrg = p_chrg->chrg->chrg_type(p_chrg, 1);

	/* Temp check gpio to update */
	if (chrg == CHARGER_USB)
		auto_update(UPDATE_KEY, UPDATE_CHECK_TIME);

#if defined(CONFIG_NXE2000_REG_DUMP)
	nxe2000_register_dump(&nxe_power_config);
#endif
	printf("## chrg:%d, power_state:%d, power_depth:%d, power_key_depth:%d\n", chrg, power_state, power_depth, power_key_depth);
	printf("## voltage_uV:%d, shutdown_ilim_uV:%d \n", pb->bat->voltage_uV, shutdown_ilim_uV);

	return 0;


enter_shutdown:

#if !defined (CONFIG_PMIC_VOLTAGE_CHECK_WITH_CHARGE)
	pmic_reg_write(p_chrg, NXE2000_REG_CHGCTL1, chgctl_reg_val);
	//pmic_reg_read(p_chrg, NXE2000_REG_GPLED_FUNC, &chg_led_mode);
	//chg_led_mode &= ~0x04;
	//pmic_reg_write(p_chrg, NXE2000_REG_GPLED_FUNC, chg_led_mode);
#endif	/* CONFIG_PMIC_VOLTAGE_CHECK_WITH_CHARGE */

	if(p_muic)
		chrg = p_muic->chrg->chrg_type(p_muic, 1);
	else
		chrg = p_chrg->chrg->chrg_type(p_chrg, 1);

#if defined(CONFIG_NXE2000_REG_DUMP)
	nxe2000_register_dump(&nxe_power_config);
#endif

	printf("## chrg:%d, power_state:%d, power_depth:%d, power_key_depth:%d\n", chrg, power_state, power_depth, power_key_depth);
	printf("## voltage_uV:%d, shutdown_ilim_uV:%d \n", pb->bat->voltage_uV, shutdown_ilim_uV);
	printf("Power Off\n");

	mdelay(500);

	pmic_reg_write(p_chrg, NXE2000_REG_SLPCNT, 0x01);

	while(1);

	return -1;
}
#else	/* CONFIG_BAT_CHECK */

int board_late_init(void)
{
#if defined(CONFIG_SYS_MMC_BOOT_DEV)
	char boot[16];
	sprintf(boot, "mmc dev %d", CONFIG_SYS_MMC_BOOT_DEV);
	run_command(boot, 0);
#endif

#if defined(CONFIG_DISPLAY_OUT)
	bd_display_run(CONFIG_CMD_LOGO_WALLPAPERS, CFG_LCD_PRI_PWM_DUTYCYCLE, 1);
#endif

#ifdef CONFIG_CMD_NET
	bd_eth_init();
#endif

	/* Temp check gpio to update */
	auto_update(UPDATE_KEY, UPDATE_CHECK_TIME);

	return 0;
}
#endif	/* CONFIG_BAT_CHECK */

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

#if defined (CONFIG_CMD_LOGO_UPDATE)
	run_command(CONFIG_CMD_LOGO_UPDATE, 0);
#endif

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

int bd_eth_init(void)
{
#if defined(CONFIG_DESIGNWARE_ETH)
	u32 addr;

#if 0	// 20140515
	// 100 & 10Base-T
	nxp_gpio_set_drv( (PAD_GPIO_E +  7), GPIO_DRV_1X );     // PAD_GPIOE7,     GMAC0_PHY_TXD[0]
	nxp_gpio_set_drv( (PAD_GPIO_E +  8), GPIO_DRV_1X );     // PAD_GPIOE8,     GMAC0_PHY_TXD[1]
	nxp_gpio_set_drv( (PAD_GPIO_E +  9), GPIO_DRV_1X );     // PAD_GPIOE9,     GMAC0_PHY_TXD[2]
	nxp_gpio_set_drv( (PAD_GPIO_E + 10), GPIO_DRV_1X );     // PAD_GPIOE10,    GMAC0_PHY_TXD[3]
	nxp_gpio_set_drv( (PAD_GPIO_E + 11), GPIO_DRV_1X );     // PAD_GPIOE11,    GMAC0_PHY_TXEN
//	nxp_gpio_set_drv( (PAD_GPIO_E + 12), GPIO_DRV_1X );     // PAD_GPIOE12,    GMAC0_PHY_TXER
//	nxp_gpio_set_drv( (PAD_GPIO_E + 13), GPIO_DRV_1X );     // PAD_GPIOE13,    GMAC0_PHY_COL
	nxp_gpio_set_drv( (PAD_GPIO_E + 14), GPIO_DRV_3X );     // PAD_GPIOE14,    GMAC0_PHY_RXD[0]
	nxp_gpio_set_drv( (PAD_GPIO_E + 15), GPIO_DRV_3X );     // PAD_GPIOE15,    GMAC0_PHY_RXD[1]
	nxp_gpio_set_drv( (PAD_GPIO_E + 16), GPIO_DRV_3X );     // PAD_GPIOE16,    GMAC0_PHY_RXD[2]
	nxp_gpio_set_drv( (PAD_GPIO_E + 17), GPIO_DRV_3X );     // PAD_GPIOE17,    GMAC0_PHY_RXD[3]
	nxp_gpio_set_drv( (PAD_GPIO_E + 18), GPIO_DRV_4X );     // PAD_GPIOE18,    GMAC0_RX_CLK
	nxp_gpio_set_drv( (PAD_GPIO_E + 19), GPIO_DRV_4X );     // PAD_GPIOE19,    GMAC0_PHY_RX_DV
	nxp_gpio_set_drv( (PAD_GPIO_E + 20), GPIO_DRV_4X );     // PAD_GPIOE20,    GMAC0_GMII_MDC
	nxp_gpio_set_drv( (PAD_GPIO_E + 21), GPIO_DRV_4X );     // PAD_GPIOE21,    GMAC0_GMII_MDI
//	nxp_gpio_set_drv( (PAD_GPIO_E + 22), GPIO_DRV_1X );     // PAD_GPIOE22,    GMAC0_PHY_RXER
//	nxp_gpio_set_drv( (PAD_GPIO_E + 23), GPIO_DRV_1X );     // PAD_GPIOE23,    GMAC0_PHY_CRS
	nxp_gpio_set_drv( (PAD_GPIO_E + 24), GPIO_DRV_1X );     // PAD_GPIOE24,    GMAC0_GTX_CLK

	NX_CLKGEN_SetClockOutInv( CLOCKINDEX_OF_DWC_GMAC_MODULE, 0, CFALSE);    // TX Clk invert off : 100 & 10Base-T
#endif

#if 1
	// 1000Base-T
	nxp_gpio_set_drv( (PAD_GPIO_E +  7), GPIO_DRV_4X );     // PAD_GPIOE7,     GMAC0_PHY_TXD[0]
	nxp_gpio_set_drv( (PAD_GPIO_E +  8), GPIO_DRV_4X );     // PAD_GPIOE8,     GMAC0_PHY_TXD[1]
	nxp_gpio_set_drv( (PAD_GPIO_E +  9), GPIO_DRV_4X );     // PAD_GPIOE9,     GMAC0_PHY_TXD[2]
	nxp_gpio_set_drv( (PAD_GPIO_E + 10), GPIO_DRV_4X );     // PAD_GPIOE10,    GMAC0_PHY_TXD[3]
	nxp_gpio_set_drv( (PAD_GPIO_E + 11), GPIO_DRV_4X );     // PAD_GPIOE11,    GMAC0_PHY_TXEN
//	nxp_gpio_set_drv( (PAD_GPIO_E + 12), GPIO_DRV_1X );     // PAD_GPIOE12,    GMAC0_PHY_TXER
//	nxp_gpio_set_drv( (PAD_GPIO_E + 13), GPIO_DRV_1X );     // PAD_GPIOE13,    GMAC0_PHY_COL
	nxp_gpio_set_drv( (PAD_GPIO_E + 14), GPIO_DRV_4X );     // PAD_GPIOE14,    GMAC0_PHY_RXD[0]
	nxp_gpio_set_drv( (PAD_GPIO_E + 15), GPIO_DRV_4X );     // PAD_GPIOE15,    GMAC0_PHY_RXD[1]
	nxp_gpio_set_drv( (PAD_GPIO_E + 16), GPIO_DRV_4X );     // PAD_GPIOE16,    GMAC0_PHY_RXD[2]
	nxp_gpio_set_drv( (PAD_GPIO_E + 17), GPIO_DRV_4X );     // PAD_GPIOE17,    GMAC0_PHY_RXD[3]
	nxp_gpio_set_drv( (PAD_GPIO_E + 18), GPIO_DRV_4X );     // PAD_GPIOE18,    GMAC0_RX_CLK
	nxp_gpio_set_drv( (PAD_GPIO_E + 19), GPIO_DRV_4X );     // PAD_GPIOE19,    GMAC0_PHY_RX_DV
	nxp_gpio_set_drv( (PAD_GPIO_E + 20), GPIO_DRV_4X );     // PAD_GPIOE20,    GMAC0_GMII_MDC
	nxp_gpio_set_drv( (PAD_GPIO_E + 21), GPIO_DRV_4X );     // PAD_GPIOE21,    GMAC0_GMII_MDI
//	nxp_gpio_set_drv( (PAD_GPIO_E + 22), GPIO_DRV_1X );     // PAD_GPIOE22,    GMAC0_PHY_RXER
//	nxp_gpio_set_drv( (PAD_GPIO_E + 23), GPIO_DRV_1X );     // PAD_GPIOE23,    GMAC0_PHY_CRS
	nxp_gpio_set_drv( (PAD_GPIO_E + 24), GPIO_DRV_4X );     // PAD_GPIOE24,    GMAC0_GTX_CLK

	//NX_CLKGEN_SetClockOutInv( CLOCKINDEX_OF_DWC_GMAC_MODULE, 0, CFALSE);    // TX Clk invert off
#endif

	// Clock control
	NX_CLKGEN_Initialize();
	addr = NX_CLKGEN_GetPhysicalAddress(CLOCKINDEX_OF_DWC_GMAC_MODULE);
	NX_CLKGEN_SetBaseAddress( CLOCKINDEX_OF_DWC_GMAC_MODULE, (u32)IO_ADDRESS(addr) );

	NX_CLKGEN_SetClockSource( CLOCKINDEX_OF_DWC_GMAC_MODULE, 0, 4);     // Sync mode for 100 & 10Base-T : External RX_clk
	NX_CLKGEN_SetClockDivisor( CLOCKINDEX_OF_DWC_GMAC_MODULE, 0, 1);    // Sync mode for 100 & 10Base-T

	NX_CLKGEN_SetClockOutInv( CLOCKINDEX_OF_DWC_GMAC_MODULE, 0, CFALSE);    // TX Clk invert off : 100 & 10Base-T
//	NX_CLKGEN_SetClockOutInv( CLOCKINDEX_OF_DWC_GMAC_MODULE, 0, CTRUE);     // TX clk invert on : 100 & 10Base-T

	NX_CLKGEN_SetClockDivisorEnable( CLOCKINDEX_OF_DWC_GMAC_MODULE, CTRUE);

	// Reset control
	NX_RSTCON_Initialize();
	addr = NX_RSTCON_GetPhysicalAddress();
	NX_RSTCON_SetBaseAddress( (u32)IO_ADDRESS(addr) );
	NX_RSTCON_SetnRST(RESETINDEX_OF_DWC_GMAC_MODULE_aresetn_i, RSTCON_ENABLE);
	udelay(100);
	NX_RSTCON_SetnRST(RESETINDEX_OF_DWC_GMAC_MODULE_aresetn_i, RSTCON_DISABLE);
	udelay(100);
	NX_RSTCON_SetnRST(RESETINDEX_OF_DWC_GMAC_MODULE_aresetn_i, RSTCON_ENABLE);
	udelay(100);

	// Set interrupt config.
	nxp_gpio_set_pull_sel(CFG_ETHER_GMAC_PHY_IRQ_NUM, CTRUE);
	nxp_gpio_set_pull_enb(CFG_ETHER_GMAC_PHY_IRQ_NUM, CTRUE);
	gpio_direction_input(CFG_ETHER_GMAC_PHY_IRQ_NUM);

	// Set GPIO nReset
	nxp_gpio_set_pull_enb(CFG_ETHER_GMAC_PHY_RST_NUM, CFALSE);
	gpio_direction_output(CFG_ETHER_GMAC_PHY_RST_NUM, 1 );
	udelay( 100 );
	gpio_set_value(CFG_ETHER_GMAC_PHY_RST_NUM, 0 );
	udelay( 100 );
	gpio_set_value(CFG_ETHER_GMAC_PHY_RST_NUM, 1 );
#endif	// #if defined(CONFIG_DESIGNWARE_ETH)

	return 0;
}

#endif

