/*
 * (C) Copyright 2009
 * jung hyun kim, Nexell Co, <jhkim@nexell.co.kr>
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
 */
#ifndef __BOARD_PM_H__
#define __BOARD_PM_H__

#define	SCR_ARM_SECOND_BOOT		IO_ADDRESS(0xC0010C1C)	// RTC scratch

#define	SCR_ALIVE_BASE			IO_ADDRESS(PHY_BASEADDR_ALIVE)
#define	SCR_SIGNAGURE_RESET		(SCR_ALIVE_BASE + 0x68)
#define	SCR_SIGNAGURE_SET		(SCR_ALIVE_BASE + 0x6C)
#define	SCR_SIGNAGURE_READ		(SCR_ALIVE_BASE + 0x70)
#define	SCR_WAKE_FN_RESET		(SCR_ALIVE_BASE + 0xAC)	// ALIVESCRATCHRST1
#define	SCR_WAKE_FN_SET			(SCR_ALIVE_BASE + 0xB0)
#define	SCR_WAKE_FN_READ		(SCR_ALIVE_BASE + 0xB4)
#define	SCR_CRC_RET_RESET		(SCR_ALIVE_BASE + 0xB8)	// ALIVESCRATCHRST2
#define	SCR_CRC_RET_SET			(SCR_ALIVE_BASE + 0xBC)
#define	SCR_CRC_RET_READ		(SCR_ALIVE_BASE + 0xC0)
#define	SCR_CRC_PHY_RESET		(SCR_ALIVE_BASE + 0xC4)	// ALIVESCRATCHRST3
#define	SCR_CRC_PHY_SET			(SCR_ALIVE_BASE + 0xC8)
#define	SCR_CRC_PHY_READ		(SCR_ALIVE_BASE + 0xCC)
#define	SCR_CRC_LEN_RESET		(SCR_ALIVE_BASE + 0xD0)	// ALIVESCRATCHRST4
#define	SCR_CRC_LEN_SET			(SCR_ALIVE_BASE + 0xD4)
#define	SCR_CRC_LEN_READ		(SCR_ALIVE_BASE + 0xD8)

#define	SCR_RESET_SIG_RESET		(SCR_ALIVE_BASE + 0xDC)	// ALIVESCRATCHRST5
#define	SCR_RESET_SIG_SET		(SCR_ALIVE_BASE + 0xE0)
#define	SCR_RESET_SIG_READ		(SCR_ALIVE_BASE + 0xE4)

#define RECOVERY_SIGNATURE		(0x52455343)	/* (ASCII) : R.E.S.C */

#endif /* __BOARD_PM_H__ */