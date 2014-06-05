/*
 * (C) Copyright 2012 SAMSUNG Electronics
 * Jaehoon Chung <jh80.chung@samsung.com>
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,  MA 02111-1307 USA
 *
 */

#include <common.h>
#include <malloc.h>
#include <dwmmc.h>
#include <mach-api.h>

static char *NXP_NAME = "NXP DWMMC";

#define DWMCI_CLKSEL			0x09C
#define DWMCI_SHIFT_0			0x0
#define DWMCI_SHIFT_1			0x1
#define DWMCI_SHIFT_2			0x2
#define DWMCI_SHIFT_3			0x3
#define DWMCI_SET_SAMPLE_CLK(x)	(x)
#define DWMCI_SET_DRV_CLK(x)	((x) << 16)
#define DWMCI_SET_DIV_RATIO(x)	((x) << 24)

static unsigned int get_mmc_clk(int dev_index)
{
	struct clk *clk;
	char name[50];

	sprintf(name, "%s.%d", DEV_NAME_SDHC, dev_index);
	clk = clk_get(NULL, name);
	if (!clk)
		return 0;

	return clk_get_rate(clk)/2;
}

static unsigned long set_mmc_clk(int dev_index, unsigned  rate)
{
	struct clk *clk;
	char name[50];

	sprintf(name, "%s.%d", DEV_NAME_SDHC, dev_index);
	clk = clk_get(NULL, name);
	if (!clk)
		return 0;

	rate = clk_set_rate(clk, rate);
	clk_enable(clk);

	return rate;
}

static void nxp_dwmci_clksel(struct dwmci_host *host)
{
	u32 val;
	val = DWMCI_SET_SAMPLE_CLK(DWMCI_SHIFT_0) |
		DWMCI_SET_DRV_CLK(DWMCI_SHIFT_0) | DWMCI_SET_DIV_RATIO(3);

	dwmci_writel(host, DWMCI_CLKSEL, val);
}

//int nxp_dwmci_init(u32 regbase, int bus_width, int index)
int nxp_dwmci_init(u32 regbase, int bus_width, int index, int max_clock)
{
	struct dwmci_host *host = NULL;

	host = malloc(sizeof(struct dwmci_host));
	if (!host) {
		printf("dwmci_host malloc fail!\n");
		return 1;
	}
	if(max_clock >= 100000000L)
		set_mmc_clk(index, 200000000L);
	else
		set_mmc_clk(index, 100000000L);
	host->name = NXP_NAME;
	host->ioaddr = (void *)regbase;
	host->buswidth = bus_width;
	host->clksel = nxp_dwmci_clksel;
	host->dev_index = index;
	host->mmc_clk = get_mmc_clk;

	if(max_clock == 100000000)
		add_dwmci(host, 100000000, 400000);
	else
		add_dwmci(host, 52000000, 400000);
	return 0;
}

int board_mmc_init(bd_t *bis)
{
	int err = 0;
#if(CONFIG_MMC0_ATTACH == TRUE)
	writel(readl(0xC0012004) | (1<<7), 0xC0012004);
#endif
#if(CONFIG_MMC0_CLOCK)
	err = nxp_dwmci_init(0xC0062000, 4, 0, CONFIG_MMC0_CLOCK);
#else 
	err = nxp_dwmci_init(0xC0062000, 4, 0, 52000000);
#endif
#if(CONFIG_MMC1_ATTACH == TRUE)
	writel(readl(0xC0012004) | (1<<8), 0xC0012004);
#endif
#if(CONFIG_MMC1_CLOCK)
	err = nxp_dwmci_init(0xC0068000, 4, 1, CONFIG_MMC1_CLOCK);
#else
	err = nxp_dwmci_init(0xC0068000, 4, 1, 52000000);
#endif

#if(CONFIG_MMC2_ATTACH == TRUE)	
	writel(readl(0xC0012004) | (1<<9), 0xC0012004);
#endif
#if(CONFIG_MMC2_CLOCK)
	err = nxp_dwmci_init(0xC0069000, 4,2, CONFIG_MMC2_CLOCK);
#else
	err = nxp_dwmci_init(0xC0069000, 4,2, 52000000);
#endif
	return err;
}
