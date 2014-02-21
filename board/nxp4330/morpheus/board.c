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
#include <asm/io.h>
#include <asm/gpio.h>

#include <platform.h>
#include <mach-api.h>
#include <nxp_dwmmc.h>
#include <nxp_rtc.h>

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
			NX_GPIO_SetOutputValue(index, bit, (lv  ? CTRUE : CFALSE));
			NX_GPIO_SetInterruptMode(index, bit, (lv));
			NX_GPIO_SetPullUpEnable(index, bit, (plup ? CTRUE : CFALSE));
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
	PAD_GPIOALV0, PAD_GPIOALV1,  PAD_GPIOALV2, PAD_GPIOALV3,
	PAD_GPIOALV4, PAD_GPIOALV5,  PAD_GPIOALV6, PAD_GPIOALV7
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
		NX_ALIVE_SetPullUpEnable(bit, (plup ? CTRUE : CFALSE));
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
	nxe_power_config.i2c_bus	= 0;

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
	bd_pmic_init();
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

int power_init_board(void)
{
	int ret;

	ret = pmic_init(I2C_0);
	ret |= pmic_init_nxe2000();
	ret |= power_fg_init(I2C_0);
	ret |= power_bat_init(I2C_0);
	if (ret)
		return ret;

	ret = power_muic_init(I2C_0);

	return 0;
}
#endif  /* CONFIG_BAT_CHECK */

int board_mmc_init(bd_t *bis)
{
	int err = 0;
#ifdef CONFIG_MMC0_NEXELL
	writel(readl(0xC0012004) | (1<<7), 0xC0012004);
	err = nxp_dwmmc_init(0, 4);
#endif
#ifdef CONFIG_MMC1_NEXELL
	writel(readl(0xC0012004) | (1<<8), 0xC0012004);
	err = nxp_dwmmc_init(1, 4);
#endif
#ifdef CONFIG_MMC2_NEXELL
	writel(readl(0xC0012004) | (1<<9), 0xC0012004);
	err = nxp_dwmmc_init(2, 4);
#endif
	return err;
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

#if defined(CONFIG_BAT_CHECK)
int board_late_init(void)
{
	int chrg;
    int shutdown_ilim_uA = NXE2000_DEF_LOWBAT1_VOL;
    u32 chg_state;
	struct power_battery *pb;
	struct pmic *p_fg, *p_chrg, *p_muic, *p_bat;
	int show_bat_state = 0;
	int power_key_depth = 0;
    u32 time_key_pev = 0;

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
        chrg = p_muic->chrg->chrg_type(p_muic);
    else
        chrg = p_chrg->chrg->chrg_type(p_chrg);

    if (!p_chrg->chrg->chrg_bat_present(p_chrg)) {
        puts("No battery detected\n");
        return -1;
    }

    if (pb->bat->state == CHARGE && chrg == CHARGER_USB)
        puts("CHARGE Battery !\n");

    /* Check to Power-Key status */
#ifndef CONFIG_FAST_BOOTUP
    if (gpio_get_value(GPIO_PMIC_VUSB_DET) || power_key_depth)
    {
        show_bat_state = 1;
    }
    else
    {
        goto enter_shutdown;
    }
#endif

//  show_bat_state = 0;
//  show_bat_state = 1;

    /* Access for image file. */
    p_fg->fg->fg_battery_check(p_fg, p_bat);
//shutdown_ilim_uA    = 3000000;

    if (pb->bat->voltage_uV < shutdown_ilim_uA)
    {
        pmic_reg_read(p_chrg, NXE2000_REG_CHGSTATE, &chg_state);
        if ( !(chg_state & NXE2000_POS_CHGSTATE_PWRSRC_MASK) )
        {
            goto enter_shutdown;
        }
    }

/*===========================================================*/

#ifdef CONFIG_FAST_BOOTUP
    if (gpio_get_value(GPIO_PMIC_VUSB_DET))
    {
        show_bat_state = 1;
    }
    else
    {
        power_key_depth = 2;
    }
    nxp_gpio_set_int_clear(CFG_KEY_POWER);
#else

    if (nxp_gpio_get_int_pend(CFG_KEY_POWER))
    {
        power_key_depth++;
    }
    else
    {
        power_key_depth = 0;
    }
#endif

    if (power_key_depth > 1)
    {
        goto skip_bat_animation;
    }

#ifdef CONFIG_FAST_BOOTUP
    power_key_depth = 1;
#endif

/*===========================================================*/

	// draw charing image
	if (show_bat_state) {
        u32 time_pwr_prev;
        u8  is_pwr_in, power_state = 0;
        u8  power_src = CHARGER_NO;
        u8  power_depth = 3;

        time_pwr_prev = nxp_rtc_get();

		while(show_bat_state)
        {
            if (nxp_gpio_get_int_pend(CFG_KEY_POWER))
            {
                power_key_depth++;
            }
            else
            {
                power_key_depth = 0;
            }
            nxp_gpio_set_int_clear(CFG_KEY_POWER);

            pmic_reg_read(p_chrg, NXE2000_REG_CHGSTATE, &chg_state);
            if (chg_state & NXE2000_POS_CHGSTATE_PWRSRC_MASK)
            {
                is_pwr_in           = 1;
                shutdown_ilim_uA    = NXE2000_DEF_LOWBAT2_VOL;
            }
            else
            {
                is_pwr_in           = 0;
                shutdown_ilim_uA    = NXE2000_DEF_LOWBAT1_VOL;
            }
//shutdown_ilim_uA    = 3000000;

            if (!power_state && is_pwr_in)
            {
                if (p_muic)
                    chrg = p_muic->chrg->chrg_type(p_muic);
                else
                    chrg = p_chrg->chrg->chrg_type(p_chrg);

                if (power_src != chrg)
                {
                    power_src = chrg;
                }
            }

            power_state = is_pwr_in;

            p_fg->fg->fg_battery_check(p_fg, p_bat);

            if (nxp_rtc_get() > (time_pwr_prev + 5))
            {
                time_pwr_prev = nxp_rtc_get();

                power_depth--;
            }

            if ((!power_state) || (!power_depth))
            {
                if ((pb->bat->voltage_uV < shutdown_ilim_uA) || (!power_depth))
                {
                    goto enter_shutdown;
                }
            }
            if (power_key_depth > 1)
            {
                if (pb->bat->voltage_uV > shutdown_ilim_uA)
                {
                    break;
                }
            }

            mdelay(1000);
        }
	}

skip_bat_animation:
	/* Temp check gpio to update */
    if (chrg == CHARGER_USB)
    	auto_update(UPDATE_KEY, UPDATE_CHECK_TIME);

	return 0;

enter_shutdown:
	pmic_reg_write(p_chrg, NXE2000_REG_SLPCNT, 0x01);
	while(1);

	return -1;
}
#else	/* CONFIG_BAT_CHECK */

int board_late_init(void)
{
	return 0;
}
#endif	/* CONFIG_BAT_CHECK */

