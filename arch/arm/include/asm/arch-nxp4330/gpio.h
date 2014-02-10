/*
 *  Copyright (C) 2013 NEXELL SOC Lab.
 *  BongKwan Kook <kook@nexell.co.kr>
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

#ifndef __ASM_ARCH_GPIO_H
#define __ASM_ARCH_GPIO_H

#ifndef __ASSEMBLY__

/* functions */
void	nxp_gpio_set_alt(int gpio, int mode);
int		nxp_gpio_get_alt(int gpio);
void	nxp_gpio_direction_output(int gpio, int val);
void	nxp_gpio_direction_input(int gpio);
void	nxp_gpio_set_value(int gpio, int val);
int		nxp_gpio_get_value(int gpio);
void	nxp_gpio_set_pull(int gpio, int mode);
void	nxp_gpio_set_drv(int gpio, int mode);
void	nxp_gpio_set_int_mode(int gpio, int d_mode);
void	nxp_gpio_set_int_en(int gpio, int enable);
int		nxp_gpio_get_int_en(int gpio);
int		nxp_gpio_get_int_pend(int gpio);
void	nxp_gpio_set_int_clear(int gpio);

/* GPIO pins per group */
#define GPIO_PER_GROUP  32

#if 0
static inline unsigned int nxp_gpio_base(int gpio)
{
	if (gpio < PAD_GPIO_ALV)
    {
        return (PHY_BASEADDR_GPIOA + (0x1000 * PAD_GET_GROUP(gpio)));
	}
	else
	{   
		return PHY_BASEADDR_ALIVE;
	}
}

static inline unsigned int nxp_gpio_part_max(int nr)
{
	return 0;
}
#endif
#endif

/* Pin configurations */
#define GPIO_INPUT		0x0
#define GPIO_OUTPUT		0x1
#define GPIO_IRQ		0xf
#define GPIO_FUNC(x)	(x)

/* Pull mode */
#define GPIO_PULL_NONE	0x0
#define GPIO_PULL_DOWN	0x1
#define GPIO_PULL_UP	0x2

/* Drive Strength level */
#define GPIO_DRV_1X		0x0
#define GPIO_DRV_2X		0x1
#define GPIO_DRV_3X		0x2
#define GPIO_DRV_4X		0x3
#define GPIO_DRV_FAST	0x0
#define GPIO_DRV_SLOW	0x1

#endif
