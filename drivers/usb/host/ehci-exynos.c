/*
 * SAMSUNG EXYNOS USB HOST EHCI Controller
 *
 * Copyright (C) 2012 Samsung Electronics Co.Ltd
 *	Vivek Gautam <gautam.vivek@samsung.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301 USA
 */

#include <common.h>
#include <fdtdec.h>
#include <libfdt.h>
#include <malloc.h>
#include <usb.h>
#include <asm-generic/errno.h>
#include <linux/compat.h>
#include "ehci.h"
#include <asm/io.h>

#include <nx_chip.h>
#include <nx_usb20host.h>
#include <nx_tieoff.h>
#include <nx_rstcon.h>
#include <nx_clkgen.h>
#include <nx_gpio.h>

#undef	NX_CONSOLE_Printf
#define	NX_CONSOLE_Printf
#ifdef NX_CONSOLE_Printf
	#define	NX_CONSOLE_Printf(fmt,args...)	printf (fmt ,##args)
#else
	#define NX_CONSOLE_Printf(fmt,args...)
#endif

/* Declare global data pointer */
DECLARE_GLOBAL_DATA_PTR;

/*
 * EHCI-initialization
 * Create the appropriate control structures to manage
 * a new EHCI host controller.
 */

int ehci_hcd_init(int index, struct ehci_hccr **hccr, struct ehci_hcor **hcor)
{
	// Release common reset of host controller
	writel(readl(0xc0012004) & ~(1<<24), 0xc0012004);			// reset on
	writel(readl(0xc0012004) |  (1<<24), 0xc0012004);			// reset off
	NX_CONSOLE_Printf( "\nRSTCON[24](0xc0012004) set 1'b1 0x%08x\n", readl( 0xc0012004 ) );

	// Program AHB Burst type
	writel(readl(0xc001101c) & ~(7<<25), 0xc001101c);
	writel(readl(0xc001101c) |  (7<<25), 0xc001101c);
	NX_CONSOLE_Printf( "TIEOFFREG7[27:25](0xc001101c) set 3'b111 0x%08x\n", readl( 0xc001101c ) );

	// Select word interface and enable word interface selection
	writel(readl(0xc0011014) & ~(3<<25), 0xc0011014);
	writel(readl(0xc0011014) |  (3<<25), 0xc0011014);			// 2'b01 8bit, 2'b11 16bit word
	NX_CONSOLE_Printf( "TIEOFFREG5[26:25](0xc0011014) set 2'b11 0x%08x\n", readl( 0xc0011014 ) );

	writel(readl(0xc0011024) & ~(3<<8), 0xc0011024);
	writel(readl(0xc0011024) |  (3<<8), 0xc0011024);				// 2'b01 8bit, 2'b11 16bit word
	NX_CONSOLE_Printf( "TIEOFFREG9[10:9](0xc0011024) set 2'b11 0x%08x\n", readl( 0xc0011024 ) );

	// POR of PHY
	writel(readl(0xc0011020) & ~(3<<7), 0xc0011020);
	writel(readl(0xc0011020) |  (1<<7), 0xc0011020);
	NX_CONSOLE_Printf( "TIEOFFREG8[8:7](0xc0011020) set 2'b01 0x%08x\n", readl( 0xc0011020 ) );

	// Wait clock of PHY - about 40 micro seconds
	udelay(40); // 40us delay need.

	// Release utmi reset
	writel(readl(0xc0011014) & ~(3<<20), 0xc0011014);
	writel(readl(0xc0011014) |  (3<<20), 0xc0011014);
	NX_CONSOLE_Printf( "TIEOFFREG5[21:20](0xc0011014) set 2'b11 0x%08x\n", readl( 0xc0011014 ) );

	//Release ahb reset of EHCI, OHCI
	writel(readl(0xc0011014) & ~(7<<17), 0xc0011014);
	writel(readl(0xc0011014) |  (7<<17), 0xc0011014);
	NX_CONSOLE_Printf( "TIEOFFREG13[19:17](0xc0011014) set 3'b111 0x%08x\n", readl( 0xc0011014 ) );

	// set Base Address
	NX_USB20HOST_SetBaseAddress( 0, NX_USB20HOST_GetPhysicalAddress(0) );

	*hccr	= ( struct ehci_hccr * )NX_USB20HOST_GetBaseAddress(0);
	*hcor	= ( struct ehci_hcor * )( NX_USB20HOST_GetBaseAddress(0) + 0x10 );
	NX_CONSOLE_Printf( "Set EHCI Base Address : hccr( 0x%08x ), hcor( 0x%08x )\n" , *hccr, *hcor );

	return 0;
}

/*
 * Destroy the appropriate control structures corresponding
 * the EHCI host controller.
 */
int ehci_hcd_stop(int index)
{
	writel(readl(0xc0012004) & ~(1<<24), 0xc0012004);			// reset on

	return 0;
}
