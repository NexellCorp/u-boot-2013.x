/*------------------------------------------------------------------------------
 *
 *	Copyright (C) 2009 Nexell Co., Ltd All Rights Reserved
 *	Nexell Co. Proprietary & Confidential
 *
 *	NEXELL INFORMS THAT THIS CODE AND INFORMATION IS PROVIDED "AS IS" BASE
 *  AND	WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING
 *  BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS
 *  FOR A PARTICULAR PURPOSE.
 *
 ------------------------------------------------------------------------------*/
#ifndef __CFG_MEM_H__
#define __CFG_MEM_H__

/*------------------------------------------------------------------------------
 * 	u-boot dram memory map
 *
 *  CONFIG_RELOC_TO_TEXT_BASE
 *
 *	=========================
 *	|					|
 *  |   u-boot 			|
 *	|	FB base			|
 *	=========================	0x8x00_0000		: CONFIG_FB_ADDR = CFG_MEM_PHY_LINEAR_BASE
 *	|					|
 *	|	u-boot mem 		|
 *	|	test zone		|
 *	|					|
 *	=========================   0x8100_0000		: u-boot heap (CONFIG_SYS_MALLOC_END, CONFIG_SYS_MEMTEST_START)
 *	|	U-BOOT HEAP		|
 *	|	....			|
 *	|					|
 *  |   U-BOOT 			|
 *  |   TEXT 	     	|
 *	=========================   0x80C0_0000		: u-boot text  (CONFIG_SYS_TEXT_BASE, CONFIG_SYS_SDRAM_BASE)
 *	|	gd				|
 *  -------------------------
 *	|	bd				|
 *  -------------------------	0x80CX_0000		: u-boot stack (CONFIG_SYS_INIT_SP_ADDR)
 *	|	U-BOOT			|
 *	|	STACK			|
 *	|-------------------|
 *	|	Kernel 			|
 *	|	(uImage)		|
 *  -------------------------	0x8000_8000		: Kernel TEXT_BASE
 *	|	TAGS			|
 *  -------------------------	0x8000_0100
 *	|					|
 *  =========================   0x8000_0000  	: u-boot (CONFIG_SYS_SDRAM_BASE)
 *
 ------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
 * 	 System memory map
 */
#define	CFG_MEM_PHY_SYSTEM_BASE			0x40000000	/* System, must be at an evne 2MB boundary (head.S) */
#define	CFG_MEM_PHY_SYSTEM_SIZE			0x40000000	/* 1G MB */

/*------------------------------------------------------------------------------
 * 	 Framebuffer
 */
#define	CFG_MEM_PHY_FB_BASE				0x60000000	/* FrameBuffer */

#endif /* __CFG_MEM_H__ */
