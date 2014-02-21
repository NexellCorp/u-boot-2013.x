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
 * 	 System memory map
 */
#define	CFG_MEM_PHY_SYSTEM_BASE			0x40000000	/* System, must be at an evne 2MB boundary (head.S) */
#define	CFG_MEM_PHY_SYSTEM_SIZE			0x40000000	/* 1G MB */

/*------------------------------------------------------------------------------
 * 	 Framebuffer
 */
#define	CFG_MEM_PHY_FB_BASE				0x50000000	/* FrameBuffer */

#endif /* __CFG_MEM_H__ */
