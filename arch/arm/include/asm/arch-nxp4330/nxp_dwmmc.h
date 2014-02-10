/*
 * (C) Copyright 2012 SAMSUNG Electronics
 * Jaehoon Chung <jh80.chung@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
#ifndef __NXP_DW_MMC_H__
#define __NXP_DW_MMC_H__



int nxp_dwmci_init(u32 regbase, int bus_width, int index);

static inline unsigned int nxp_dwmmc_init(int index, int bus_width)
{
	unsigned int base;
	switch (index) {
	case 0:	base = 0xC0062000; break;
	case 1:	base = 0xC0068000; break;
	case 2:	base = 0xC0069000; break;
	}
	return nxp_dwmci_init(base, bus_width, index);
}

#endif