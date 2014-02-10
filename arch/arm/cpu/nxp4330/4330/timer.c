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

#include <common.h>
#include <platform.h>
#include <mach-api.h>

/* degug print */
#if	(0)
#define DBGOUT(msg...)		do { printf("timer: " msg); } while (0)
#else
#define DBGOUT(msg...)		do {} while (0)
#endif

#if (CFG_TIMER_SYS_TICK_CH > 3)
#error Not support timer channel "0~3"
#endif

/* global variables to save timer count */
static ulong timestamp;
static ulong lastdec;
/* set .data section, before u-boot is relocated */
static int	 timerinit __attribute__ ((section(".data"))) = 0;

/* macro to hw timer tick config */
static long	TIMER_FREQ  = 1000000;
static long	TIMER_HZ    = 1000000 / CONFIG_SYS_HZ;
static long	TIMER_COUNT = -1UL;

struct timer_register {
	volatile U32 TCFG0;
	volatile U32 TCFG1;
	volatile U32 TCON;
	volatile U32 TCNTB0;
	volatile U32 TCMPB0;
	volatile U32 TCNTO0;
 	volatile U32 TCNTB1;
	volatile U32 TCMPB1;
	volatile U32 TCNTO1;
	volatile U32 TCNTB2;
	volatile U32 TCMPB2;
	volatile U32 TCNTO2;
	volatile U32 TCNTB3;
	volatile U32 TCMPB3;
	volatile U32 TCNTO3;
	volatile U32 TCNTB4;
	volatile U32 TCNTO4;
	volatile U32 TINT_CSTAT;
};

#define	TCON_AUTO		(1<<3)
#define	TCON_INVERT		(1<<2)
#define	TCON_UPDATE		(1<<1)
#define	TCON_START		(1<<0)
#define TCFG0_CH(ch)	(ch == 0 && ch == 1 ? 8 : 0)
#define TCFG1_CH(ch)	(ch * 4)
#define TCON_CH(ch)		(ch ? ch * 4  + 4 : 0)
#define TINT_CH(ch)		(ch)
#define TINT_CS_CH(ch)	(ch + 5)
#define	TINT_CS_MASK	(0x1F)
#define CH_OFFSET	 	(0xC)

/*
 * Timer HW
 */
#define	TMR_BASE		((struct timer_register *)IO_ADDRESS(PHY_BASEADDR_TIMER))
#define	TIMER_READ(ch)	(TIMER_COUNT - ReadIODW((U8*)&(TMR_BASE)->TCNTO0 + (CH_OFFSET * ch)))

static inline void timer_clock(int ch, int mux, int scl)
{
	struct timer_register *preg = TMR_BASE;
	volatile U32 val;

	val  = ReadIODW(&preg->TCFG0);
	val &= ~(0xFF   << TCFG0_CH(ch));
	val |=  ((scl-1)<< TCFG0_CH(ch));
	WriteIODW(&preg->TCFG0, val);

	val  = ReadIODW(&preg->TCFG1);
	val &= ~(0xF << TCFG1_CH(ch));
	val |=  (mux << TCFG1_CH(ch));
	WriteIODW(&preg->TCFG1, val);
}

static inline void timer_count(int ch, unsigned int cnt)
{
	struct timer_register *preg = TMR_BASE;
	WriteIODW(((U8*)&preg->TCNTB0+(CH_OFFSET * ch)), (cnt-1));
	WriteIODW(((U8*)&preg->TCMPB0+(CH_OFFSET * ch)), (cnt-1));
}


static inline void timer_start(int ch, int irqon)
{
	struct timer_register *preg = TMR_BASE;
	volatile U32 val;
	int on = irqon ? 1 : 0;

	val  = ReadIODW(&preg->TINT_CSTAT);
	val &= ~(TINT_CS_MASK<<5 | 0x1 << TINT_CH(ch));
	val |=  (0x1 << TINT_CS_CH(ch) | on << TINT_CH(ch));
	WriteIODW(&preg->TINT_CSTAT, val);

	val = ReadIODW(&preg->TCON);
	val &= ~(0xE << TCON_CH(ch));
	val |=  (TCON_UPDATE << TCON_CH(ch));
	WriteIODW(&preg->TCON, val);

	val &= ~(TCON_UPDATE << TCON_CH(ch));
	val |=  ((TCON_AUTO | TCON_START)  << TCON_CH(ch));
	WriteIODW(&preg->TCON, val);
}

static inline void timer_stop(int ch, int irqon)
{
	struct timer_register *preg = TMR_BASE;
	volatile U32 val;
	int on = irqon ? 1 : 0;

	val  = ReadIODW(&preg->TINT_CSTAT);
	val &= ~(TINT_CS_MASK<<5 | 0x1 << TINT_CH(ch));
	val |=  (0x1 << TINT_CS_CH(ch) | on << TINT_CH(ch));
	WriteIODW(&preg->TINT_CSTAT, val);

	val  = ReadIODW(&preg->TCON);
	val &= ~(TCON_START << TCON_CH(ch));
	WriteIODW(&preg->TCON, val);
}

static inline void timer_irq_clear(int ch)
{
	struct timer_register *preg = TMR_BASE;
	volatile U32 val;
	val  = ReadIODW(&preg->TINT_CSTAT);
	val &= ~(TINT_CS_MASK<<5);
	val |= (0x1 << TINT_CS_CH(ch));
	WriteIODW(&preg->TINT_CSTAT, val);
}

/*------------------------------------------------------------------------------
 * u-boot timer interface
 */
int timer_init(void)
{
	struct clk *clk = NULL;
	char name[16] = "pclk";
	int ch = CFG_TIMER_SYS_TICK_CH;
	unsigned long rate, tclk = 0;
	unsigned long mout, thz, cmp = -1UL;
	int tcnt, tscl = 0, tmux = 0;
	int mux = 0, scl = 0;
	int version = nxp_cpu_version();

	if (timerinit)
		return 0;

	/* get with PCLK */
	clk  = clk_get(NULL, name);
   	rate = clk_get_rate(clk);
   	for (mux = 0; 5 > mux; mux++) {
   		mout = rate/(1<<mux), scl = mout/TIMER_FREQ, thz = mout/scl;
   		if (!(mout%TIMER_FREQ) && 256 > scl) {
   			tclk = thz, tmux = mux, tscl = scl;
   			break;
   		}
		if (scl > 256)
			continue;
		if (abs(thz-TIMER_FREQ) >= cmp)
			continue;
		tclk = thz, tmux = mux, tscl = scl;
		cmp = abs(thz-TIMER_FREQ);
   	}
	tcnt = tclk;	/* Timer Count := 1 Mhz counting */

	/* get with CLKGEN */
	if (version) {
		sprintf(name, "%s.%d", DEV_NAME_TIMER, ch);
		clk  = clk_get(NULL, name);
		rate = clk_round_rate(clk, TIMER_FREQ);
		if (abs(tclk - TIMER_FREQ) >= abs(rate - TIMER_FREQ)) {
			clk_set_rate(clk, rate);
			tmux = 5, tscl = 1, tcnt = TIMER_FREQ;
		}
	}

   	TIMER_FREQ = tcnt;	/* Timer Count := 1 Mhz counting */
   	TIMER_HZ = TIMER_FREQ / CONFIG_SYS_HZ;
	tcnt = TIMER_COUNT == -1UL ? TIMER_COUNT+1 : tcnt;

	/* reset control: Low active ___|---   */
	NX_RSTCON_SetnRST(RESETINDEX_OF_TIMER_MODULE_PRESETn, RSTCON_nDISABLE);
	NX_RSTCON_SetnRST(RESETINDEX_OF_TIMER_MODULE_PRESETn, RSTCON_nENABLE);

	timer_stop (ch, 0);
	timer_clock(ch, tmux, tscl);
	timer_count(ch, tcnt);
	timer_start(ch, 0);

	reset_timer_masked();
	timerinit = 1;
	return 0;
}

void reset_timer(void)
{
	reset_timer_masked();
}

ulong get_timer(ulong base)
{
	ulong time = get_timer_masked();
	ulong hz = TIMER_HZ;
	return (time/hz - base);
}

void set_timer(ulong t)
{
	timestamp = (ulong)t;
}

void reset_timer_masked(void)
{
	int ch = CFG_TIMER_SYS_TICK_CH;
	/* reset time */
	lastdec = TIMER_READ(ch);  	/* capure current decrementer value time */
	timestamp = 0;	       		/* start "advancing" time stamp from 0 */
}

ulong get_timer_masked(void)
{
	int ch = CFG_TIMER_SYS_TICK_CH;
	ulong now = TIMER_READ(ch);		/* current tick value */

	if (now >= lastdec) {			/* normal mode (non roll) */
		timestamp += now - lastdec; /* move stamp fordward with absoulte diff ticks */
	} else {						/* we have overflow of the count down timer */
		/* nts = ts + ld + (TLV - now)
		 * ts=old stamp, ld=time that passed before passing through -1
		 * (TLV-now) amount of time after passing though -1
		 * nts = new "advancing time stamp"...it could also roll and cause problems.
		 */
		timestamp += now + TIMER_COUNT - lastdec;
	}
	/* save last */
	lastdec = now;

	DBGOUT("now=%ld, last=%ld, timestamp=%ld\n", now, lastdec, timestamp);
	return (ulong)timestamp;
}

void __udelay(unsigned long usec)
{
	ulong tmo, tmp;
	DBGOUT("+udelay=%ld\n", usec);

	if (! timerinit)
		timer_init();

	if (usec >= 1000) {			// if "big" number, spread normalization to seconds //
		tmo  = usec / 1000;		// start to normalize for usec to ticks per sec //
		tmo *= TIMER_FREQ;		// find number of "ticks" to wait to achieve target //
		tmo /= 1000;			// finish normalize. //
	} else {						// else small number, don't kill it prior to HZ multiply //
		tmo = usec * TIMER_FREQ;
		tmo /= (1000*1000);
	}

	tmp = get_timer_masked ();			// get current timestamp //
	DBGOUT("A. tmo=%ld, tmp=%ld\n", tmo, tmp);

	if ( tmp > (tmo + tmp + 1) )	// if setting this fordward will roll time stamp //
		reset_timer_masked();		// reset "advancing" timestamp to 0, set lastdec value //
	else
		tmo += tmp;					// else, set advancing stamp wake up time //

	DBGOUT("B. tmo=%ld, tmp=%ld\n", tmo, tmp);

	while (tmo > get_timer_masked ())// loop till event //
	{
		//*NOP*/;
	}

	DBGOUT("-udelay=%ld\n", usec);
	return;
}

void udelay_masked(unsigned long usec)
{
	ulong tmo, endtime;
	signed long diff;

	if (usec >= 1000) {		/* if "big" number, spread normalization to seconds */
		tmo = usec / 1000;	/* start to normalize for usec to ticks per sec */
		tmo *= TIMER_FREQ;		/* find number of "ticks" to wait to achieve target */
		tmo /= 1000;		/* finish normalize. */
	} else {			/* else small number, don't kill it prior to HZ multiply */
		tmo = usec * TIMER_FREQ;
		tmo /= (1000*1000);
	}

	endtime = get_timer_masked() + tmo;

	do {
		ulong now = get_timer_masked();
		diff = endtime - now;
	} while (diff >= 0);
}

unsigned long long get_ticks(void)
{
	return get_timer_masked();
}

ulong get_tbclk(void)
{
	ulong  tbclk = TIMER_FREQ;
	return tbclk;
}
