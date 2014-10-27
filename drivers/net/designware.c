/*
 * (C) Copyright 2010
 * Vipin Kumar, ST Micoelectronics, vipin.kumar@st.com.
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

/*
 * Designware ethernet IP driver for u-boot
 */

#include <common.h>
#include <miiphy.h>
#include <malloc.h>
#include <linux/compiler.h>
#include <linux/err.h>
#include <asm/io.h>
#include "designware.h"

#ifndef CONFIG_MII
#error "CONFIG_MII has to be defined!"
#endif

//#define CONFIG_802_3_ENABLE

#if 0
#define ENTER_FUNC()    (printf("DWCETH (%s) ++\n", __func__))
#define EXIT_FUNC()     (printf("DWCETH (%s) --\n", __func__))
#else
#define ENTER_FUNC()
#define EXIT_FUNC()
#endif

/* Set CSR, MDC = 125Mhz(P_Clk) / n */
//#define MII_CLK_DIV		MII_CLKRANGE_DIV_42
//#define MII_CLK_DIV		MII_CLKRANGE_DIV_16
//#define MII_CLK_DIV		MII_CLKRANGE_DIV_10
#define MII_CLK_DIV		MII_CLKRANGE_DIV_8


#if defined(CONFIG_DW_SEARCH_PHY)
static int find_phy(struct eth_device *dev);
#endif
static int eth_mdio_read(struct eth_device *dev, u8 addr, u8 reg, u16 *val);
static int eth_mdio_write(struct eth_device *dev, u8 addr, u8 reg, u16 val);
static int configure_phy(struct eth_device *dev);

static void tx_descs_init(struct eth_device *dev)
{
	struct dw_eth_dev *priv = dev->priv;
	struct eth_dma_regs *dma_p = priv->dma_regs_p;
	struct dmamacdescr *desc_table_p;// = &priv->tx_mac_descrtable[0];
	char *txbuffs;// = &priv->txbuffs[0];
	struct dmamacdescr *desc_p;
	u32 idx, temp;

ENTER_FUNC();

	temp            = (u32)&priv->tx_mac_descrtable[0];
	desc_table_p    = (struct dmamacdescr *)(temp & DMA_BUFF_ALIGN_MASK);

	temp            = (u32)&priv->txbuffs[0];
	txbuffs         = (char *)(temp & DMA_BUFF_ALIGN_MASK);

	for (idx = 0; idx < CONFIG_TX_DESCR_NUM; idx++) {
		desc_p = &desc_table_p[idx];
		desc_p->dmamac_addr = &txbuffs[idx * CONFIG_ETH_BUFSIZE];
		desc_p->dmamac_next = &desc_table_p[idx + 1];

#if defined(CONFIG_DW_ALTDESCRIPTOR)
		desc_p->txrx_status &= ~(DESC_TXSTS_TXINT | DESC_TXSTS_TXLAST |
				DESC_TXSTS_TXFIRST | DESC_TXSTS_TXCRCDIS | \
				DESC_TXSTS_TXCHECKINSCTRL | \
				DESC_TXSTS_TXRINGEND | DESC_TXSTS_TXPADDIS);

		desc_p->txrx_status |= DESC_TXSTS_TXCHAIN;
		desc_p->dmamac_cntl = 0;
		desc_p->txrx_status &= ~(DESC_TXSTS_MSK | DESC_TXSTS_OWNBYDMA);
#else
		desc_p->dmamac_cntl = DESC_TXCTRL_TXCHAIN;
		desc_p->txrx_status = 0;
#endif
	}

	/* Correcting the last pointer of the chain */
	desc_p->dmamac_next = &desc_table_p[0];

	/* Flush all Tx buffer descriptors at once */
	flush_dcache_range((unsigned int)desc_table_p,
				(unsigned int)desc_table_p +
				sizeof(priv->tx_mac_descrtable));

	writel((ulong)&desc_table_p[0], &dma_p->txdesclistaddr);
	priv->tx_currdescnum = 0;

EXIT_FUNC();
}

static void rx_descs_init(struct eth_device *dev)
{
	struct dw_eth_dev *priv = dev->priv;
	struct eth_dma_regs *dma_p = priv->dma_regs_p;
	struct dmamacdescr *desc_table_p;// = &priv->rx_mac_descrtable[0];
	char *rxbuffs;// = &priv->rxbuffs[0];
	struct dmamacdescr *desc_p;
	u32 idx, temp;

ENTER_FUNC();

	temp            = (u32)&priv->rx_mac_descrtable[0];
	desc_table_p    = (struct dmamacdescr *)(temp & DMA_BUFF_ALIGN_MASK);

	temp            = (u32)&priv->rxbuffs[0];
	rxbuffs         = (char *)(temp & DMA_BUFF_ALIGN_MASK);

	/* Before passing buffers to GMAC we need to make sure zeros
	 * written there right after "priv" structure allocation were
	 * flushed into RAM.
	 * Otherwise there's a chance to get some of them flushed in RAM when
	 * GMAC is already pushing data to RAM via DMA. This way incoming from
	 * GMAC data will be corrupted. */
	flush_dcache_range((unsigned int)rxbuffs, (unsigned int)rxbuffs +
			   RX_TOTAL_BUFSIZE);

	for (idx = 0; idx < CONFIG_RX_DESCR_NUM; idx++) {
		desc_p = &desc_table_p[idx];
		desc_p->dmamac_addr = &rxbuffs[idx * CONFIG_ETH_BUFSIZE];
		desc_p->dmamac_next = &desc_table_p[idx + 1];

		desc_p->dmamac_cntl =
			(MAC_MAX_FRAME_SZ & DESC_RXCTRL_SIZE1MASK) | \
				      DESC_RXCTRL_RXCHAIN;

		desc_p->txrx_status = DESC_RXSTS_OWNBYDMA;
	}

	/* Correcting the last pointer of the chain */
	desc_p->dmamac_next = &desc_table_p[0];

	/* Flush all Rx buffer descriptors at once */
	flush_dcache_range((unsigned int)desc_table_p,
				(unsigned int)desc_table_p +
				sizeof(priv->rx_mac_descrtable));

	writel((ulong)&desc_table_p[0], &dma_p->rxdesclistaddr);
	priv->rx_currdescnum = 0;

EXIT_FUNC();
}

static void descs_init(struct eth_device *dev)
{
	tx_descs_init(dev);
	rx_descs_init(dev);
}

static int mac_info(struct eth_device *dev)
{
	struct dw_eth_dev *priv = dev->priv;
	struct eth_mac_regs *mac_p = priv->mac_regs_p;
	struct eth_dma_regs *dma_p = priv->dma_regs_p;
	u32 mac_ver;

ENTER_FUNC();

	mac_ver = readl(&mac_p->version);

EXIT_FUNC();

	return 0;
}

#if 0
static int mac_reset(struct eth_device *dev)
{
	struct dw_eth_dev *priv = dev->priv;
	struct eth_mac_regs *mac_p = priv->mac_regs_p;
	struct eth_dma_regs *dma_p = priv->dma_regs_p;

	ulong start;
	int timeout = CONFIG_MACRESET_TIMEOUT;

ENTER_FUNC();

	writel(DMAMAC_SRST, &dma_p->busmode);
	writel(MII_PORTSELECT, &mac_p->conf);

	start = get_timer(0);
	while (get_timer(start) < timeout) {
		if (!(readl(&dma_p->busmode) & DMAMAC_SRST))
			return 0;

		/* Try again after 10usec */
		udelay(10);
	};

EXIT_FUNC();

	return -1;
}
#else

static int mac_reset(struct eth_device *dev)
{
	struct dw_eth_dev *priv = dev->priv;
	struct eth_dma_regs *dma_p = priv->dma_regs_p;
	unsigned int start;

	writel(readl(&dma_p->busmode) | DMAMAC_SRST, &dma_p->busmode);

	start = get_timer(0);
	while (readl(&dma_p->busmode) & DMAMAC_SRST) {
		if (get_timer(start) >= CONFIG_MACRESET_TIMEOUT)
			return -1;

		mdelay(100);
	};

	return 0;
}
#endif

static int dw_write_hwaddr(struct eth_device *dev)
{
	struct dw_eth_dev *priv = dev->priv;
	struct eth_mac_regs *mac_p = priv->mac_regs_p;
	u32 macid_lo, macid_hi;
	u8 *mac_id = &dev->enetaddr[0];

	macid_lo = mac_id[0] + (mac_id[1] << 8) +
		   (mac_id[2] << 16) + (mac_id[3] << 24);
	macid_hi = mac_id[4] + (mac_id[5] << 8);

	writel(macid_hi, &mac_p->macaddr0hi);
	writel(macid_lo, &mac_p->macaddr0lo);

	return 0;
}

static int init_phy(struct eth_device *dev)
{
	struct dw_eth_dev *priv = dev->priv;
	struct phy_device *phydev = NULL;
	u32 supported;

ENTER_FUNC();

#ifdef CONFIG_PHYLIB
	if (priv->bus) {
		phydev = phy_connect(priv->bus, priv->address, dev,
					priv->interface);
	}

	if (!phydev) {
		printf("Failed to connect\n");
		return -1;
	}

	priv->phydev = phydev;

	phy_config(phydev);
	phy_startup(phydev);
#endif

EXIT_FUNC();

	return 0;
}

static int dw_eth_init(struct eth_device *dev, bd_t *bis)
{
	struct dw_eth_dev *priv = dev->priv;
	struct eth_mac_regs *mac_p = priv->mac_regs_p;
	struct eth_dma_regs *dma_p = priv->dma_regs_p;
	u32 conf;

ENTER_FUNC();

	if ((priv->phy_configured != 1) || (priv->link_configured != 1))
		configure_phy(dev);
	else {
		priv->speed = miiphy_speed(dev->name, priv->address);
		priv->duplex = miiphy_duplex(dev->name, priv->address);
	}

	/* Print link status only once */
	if (!priv->link_printed) {
		mac_info(dev);
		printf("ENET Speed is %d Mbps - %s duplex connection\n",
				 priv->speed, (priv->duplex == HALF) ? "HALF" : "FULL");
		priv->link_printed = 1;
	}

	/* Reset ethernet hardware */
	if (mac_reset(dev) < 0)
		return -1;

	/* Set AXI bus mode */
	writel((8 << WROSRLMT_SHIFT) | (8 << RDOSRLMT_SHIFT) | AXI_BURST_8,
			&dma_p->axibusmode);
	/* Soft reset above clears HW address registers.
	 * So we have to set it here once again */
	dw_write_hwaddr(dev);

#if 0
	writel(FIXEDBURST | PRIORXTX_41 | BURST_16,
			&dma_p->busmode);
#else
	/* PBL = 8 burst / 8 = 1 */
	writel(PBLx8 | (1<<RPBL_SHIFT) | (1<<PBL_SHIFT) | PRIORXTX_11,
			&dma_p->busmode);
#endif

	writel(readl(&dma_p->opmode) | FLUSHTXFIFO | STOREFORWARD |
		TXSECONDFRAME, &dma_p->opmode);

#if defined(CONFIG_DW_AUTONEG) && defined(CONFIG_802_3_ENABLE)
	conf = TWOKPE_802_3 | FRAMEBURSTENABLE | MII_PORTSELECT | DISABLERXOWN;
#else
	conf = FRAMEBURSTENABLE | MII_PORTSELECT | DISABLERXOWN;
#endif

	if (priv->speed != 10) {
		conf |= FES_100;

		if (priv->speed == 1000)
		conf &= ~MII_PORTSELECT;
	}

	if (priv->duplex == FULL)
		conf |= FULLDPLXMODE;

	conf |= (1 << 24);

	writel(conf, &mac_p->conf);

	writel(readl(&dma_p->opmode) | STOREFORWARD, &dma_p->opmode);
	writel(readl(&dma_p->opmode) | (1 << 25), &dma_p->opmode);

	descs_init(dev);

	writel(0, &mac_p->hashtablehigh);
	writel(0, &mac_p->hashtablelow);

	conf  = readl(&mac_p->flowcontrol);
#if defined(CONFIG_DW_DUPLEXHALF)
	conf &= ~((1<<2) | (1<<1));
	conf |=  (1<<0);
#else
	conf |=  ((1<<2) | (1<<1));
#endif
	writel(conf, &mac_p->flowcontrol);

	writel(readl(&mac_p->framefilt) | (1<<5), &mac_p->framefilt);    // E_BIT_BROADF_DISABLE

	/*
	 * Start/Enable xfer at dma as well as mac level
	 */
	writel(readl(&dma_p->opmode) | RXSTART, &dma_p->opmode);
	writel(readl(&dma_p->opmode) | TXSTART, &dma_p->opmode);

	writel(readl(&mac_p->conf) | RXENABLE | TXENABLE, &mac_p->conf);

EXIT_FUNC();

	return 0;
}

static int dw_eth_send(struct eth_device *dev, void *packet, int length)
{
	struct dw_eth_dev *priv = dev->priv;
	struct eth_dma_regs *dma_p = priv->dma_regs_p;
	u32 desc_num = priv->tx_currdescnum;
	struct dmamacdescr *desc_p;// = &priv->tx_mac_descrtable[desc_num];
	u32 temp;

ENTER_FUNC();

	temp   = (u32)&priv->tx_mac_descrtable[desc_num];
	desc_p = (struct dmamacdescr *)(temp & DMA_BUFF_ALIGN_MASK);

	/* Invalidate only "status" field for the following check */
	invalidate_dcache_range((unsigned long)&desc_p->txrx_status,
				(unsigned long)&desc_p->txrx_status +
				sizeof(desc_p->txrx_status));

	/* Check if the descriptor is owned by CPU */
	if (desc_p->txrx_status & DESC_TXSTS_OWNBYDMA) {
		printf("CPU not owner of tx frame\n");
		return -1;
	}

	memcpy((void *)desc_p->dmamac_addr, packet, length);

	/* Flush data to be sent */
	flush_dcache_range((unsigned long)desc_p->dmamac_addr,
				(unsigned long)desc_p->dmamac_addr + length);

#if defined(CONFIG_DW_ALTDESCRIPTOR)
	desc_p->txrx_status |= DESC_TXSTS_TXFIRST | DESC_TXSTS_TXLAST;
	desc_p->dmamac_cntl |= (length << DESC_TXCTRL_SIZE1SHFT) & \
			       DESC_TXCTRL_SIZE1MASK;

	desc_p->txrx_status &= ~(DESC_TXSTS_MSK);
	desc_p->txrx_status |= DESC_TXSTS_OWNBYDMA;
#else
	desc_p->dmamac_cntl |= ((length << DESC_TXCTRL_SIZE1SHFT) & \
			       DESC_TXCTRL_SIZE1MASK) | DESC_TXCTRL_TXLAST | \
			       DESC_TXCTRL_TXFIRST;

	desc_p->txrx_status = DESC_TXSTS_OWNBYDMA;
#endif

	/* Flush modified buffer descriptor */
	flush_dcache_range((unsigned long)desc_p,
				(unsigned long)desc_p + sizeof(struct dmamacdescr));

	/* Test the wrap-around condition. */
	if (++desc_num >= CONFIG_TX_DESCR_NUM)
		desc_num = 0;

	priv->tx_currdescnum = desc_num;

	/* Start the transmission */
	writel(POLL_DATA, &dma_p->txpolldemand);

EXIT_FUNC();

	return 0;
}

static int dw_eth_recv(struct eth_device *dev)
{
	struct dw_eth_dev *priv = dev->priv;
	u32 status, desc_num = priv->rx_currdescnum;
	struct dmamacdescr *desc_p;// = &priv->rx_mac_descrtable[desc_num];
	int length = 0;
	u32 temp;

ENTER_FUNC();

	temp   = (u32)&priv->rx_mac_descrtable[desc_num];
	desc_p = (struct dmamacdescr *)(temp & DMA_BUFF_ALIGN_MASK);

	/* Invalidate entire buffer descriptor */
	invalidate_dcache_range((unsigned long)desc_p,
				(unsigned long)desc_p +
				sizeof(struct dmamacdescr));

	status = desc_p->txrx_status;

	/* Check  if the owner is the CPU */
	if (!(status & DESC_RXSTS_OWNBYDMA)) {

		length = (status & DESC_RXSTS_FRMLENMSK) >> \
			 DESC_RXSTS_FRMLENSHFT;

		/* Invalidate received data */
		invalidate_dcache_range((unsigned long)desc_p->dmamac_addr,
					(unsigned long)desc_p->dmamac_addr +
					length);

		NetReceive(desc_p->dmamac_addr, length);

		/*
		 * Make the current descriptor valid again and go to
		 * the next one
		 */
		desc_p->txrx_status |= DESC_RXSTS_OWNBYDMA;

		/* Flush only status field - others weren't changed */
		flush_dcache_range((unsigned long)&desc_p->txrx_status,
					(unsigned long)&desc_p->txrx_status +
					sizeof(desc_p->txrx_status));

		/* Test the wrap-around condition. */
		if (++desc_num >= CONFIG_RX_DESCR_NUM)
			desc_num = 0;
	}

	priv->rx_currdescnum = desc_num;

EXIT_FUNC();

	return length;
}

#if 0   // Fixed - 20140521
static void dw_eth_halt(struct eth_device *dev)
{
	struct dw_eth_dev *priv = dev->priv;

ENTER_FUNC();

	mac_reset(dev);
	priv->tx_currdescnum = priv->rx_currdescnum = 0;

EXIT_FUNC();
}
#else

static void dw_eth_halt(struct eth_device *dev)
{
	struct dw_eth_dev *priv = dev->priv;
	struct eth_mac_regs *mac_p = priv->mac_regs_p;
	struct eth_dma_regs *dma_p = priv->dma_regs_p;

ENTER_FUNC();

	writel(readl(&mac_p->conf) & ~(RXENABLE | TXENABLE), &mac_p->conf);
	writel(readl(&dma_p->opmode) & ~(RXSTART | TXSTART), &dma_p->opmode);

	if (priv->phydev)
		phy_shutdown(priv->phydev);

EXIT_FUNC();
}
#endif

static int eth_mdio_read(struct eth_device *dev, u8 addr, u8 reg, u16 *val)
{
	struct dw_eth_dev *priv = dev->priv;
	struct eth_mac_regs *mac_p = priv->mac_regs_p;
	ulong start;
	u32 miiaddr;
	int timeout = CONFIG_MDIO_TIMEOUT;

	miiaddr = ((addr << MIIADDRSHIFT) & MII_ADDRMSK) | \
		  ((reg << MIIREGSHIFT) & MII_REGMSK);

//	writel(miiaddr | MII_CLKRANGE_150_250M | MII_BUSY, &mac_p->miiaddr);
	writel(miiaddr | MII_CLK_DIV | MII_BUSY, &mac_p->miiaddr);

	start = get_timer(0);
	while (get_timer(start) < timeout) {
		if (!(readl(&mac_p->miiaddr) & MII_BUSY)) {
			*val = readl(&mac_p->miidata);
			return 0;
		}

		/* Try again after 10usec */
		udelay(10);
	};

	return -1;
}

static int eth_mdio_write(struct eth_device *dev, u8 addr, u8 reg, u16 val)
{
	struct dw_eth_dev *priv = dev->priv;
	struct eth_mac_regs *mac_p = priv->mac_regs_p;
	ulong start;
	u32 miiaddr;
	int ret = -1, timeout = CONFIG_MDIO_TIMEOUT;
	u16 value;

	writel(val, &mac_p->miidata);
	miiaddr = ((addr << MIIADDRSHIFT) & MII_ADDRMSK) |
		  ((reg << MIIREGSHIFT) & MII_REGMSK) | MII_WRITE;

//	writel(miiaddr | MII_CLKRANGE_150_250M | MII_BUSY, &mac_p->miiaddr);
	writel(miiaddr | MII_CLK_DIV | MII_BUSY, &mac_p->miiaddr);

	start = get_timer(0);
	while (get_timer(start) < timeout) {
		if (!(readl(&mac_p->miiaddr) & MII_BUSY)) {
			ret = 0;
			break;
		}

		/* Try again after 10usec */
		udelay(10);
	};

	/* Needed as a fix for ST-Phy */
	eth_mdio_read(dev, addr, reg, &value);

	return ret;
}

#if defined(CONFIG_DW_SEARCH_PHY)
static int find_phy(struct eth_device *dev)
{
	int phy_addr = 1;
	u16 ctrl, oldctrl;

ENTER_FUNC();

	do {
		eth_mdio_read(dev, phy_addr, MII_BMCR, &ctrl);
		oldctrl = ctrl & BMCR_ANENABLE;

		ctrl ^= BMCR_ANENABLE;
		eth_mdio_write(dev, phy_addr, MII_BMCR, ctrl);
		eth_mdio_read(dev, phy_addr, MII_BMCR, &ctrl);
		ctrl &= BMCR_ANENABLE;

		if (ctrl == oldctrl) {
			phy_addr++;
		} else {
			ctrl ^= BMCR_ANENABLE;
			eth_mdio_write(dev, phy_addr, MII_BMCR, ctrl);

			return phy_addr;
		}
	} while (phy_addr < 32);

EXIT_FUNC();

	return -1;
}
#endif

static int dw_reset_phy(struct eth_device *dev)
{
	struct dw_eth_dev *priv = dev->priv;
	u16 ctrl;
	ulong start;
	int timeout = CONFIG_PHYRESET_TIMEOUT;
	u32 phy_addr = priv->address;

ENTER_FUNC();

	eth_mdio_write(dev, phy_addr, MII_BMCR, BMCR_RESET);

	start = get_timer(0);
	while (get_timer(start) < timeout) {
		eth_mdio_read(dev, phy_addr, MII_BMCR, &ctrl);
		if (!(ctrl & BMCR_RESET))
			break;

		/* Try again after 10usec */
		udelay(10);
	};

	if (get_timer(start) >= CONFIG_PHYRESET_TIMEOUT) {
		EXIT_FUNC();
		return -1;
	}

#ifdef CONFIG_PHY_RESET_DELAY
	udelay(CONFIG_PHY_RESET_DELAY);
#endif

EXIT_FUNC();

	return 0;
}

/*
 * Add weak default function for board specific PHY configuration
 */
int __weak designware_board_phy_init(struct eth_device *dev, int phy_addr,
		int (*mii_write)(struct eth_device *, u8, u8, u16),
		int dw_reset_phy(struct eth_device *))
{
	return 0;
}

static int configure_phy(struct eth_device *dev)
{
	struct dw_eth_dev *priv = dev->priv;
	int phy_addr;
	u16 temp;
	u16 bmcr;
#if 1   //defined(CONFIG_DW_AUTONEG)
	u16 bmsr;
	u32 timeout;
	ulong start;
#endif

ENTER_FUNC();

#if defined(CONFIG_DW_SEARCH_PHY)
	phy_addr = find_phy(dev);
	printk ("phy addr: %d\n", phy_addr);
	if (phy_addr >= 0)
		priv->address = phy_addr;
	else
		return -1;
#else
	phy_addr = priv->address;
#endif

	/*
	 * Some boards need board specific PHY initialization. This is
	 * after the main driver init code but before the auto negotiation
	 * is run.
	 */
	if (designware_board_phy_init(dev, phy_addr,
					 eth_mdio_write, dw_reset_phy) < 0)
		return -1;

	if (init_phy(dev) < 0)
		return -1;

	priv->speed = miiphy_speed(dev->name, phy_addr);
	priv->duplex = miiphy_duplex(dev->name, phy_addr);

EXIT_FUNC();

	return 0;
}

#if defined(CONFIG_MII)
static int dw_mii_read(const char *devname, u8 addr, u8 reg, u16 *val)
{
	struct eth_device *dev;

	dev = eth_get_dev_by_name(devname);
	if (dev)
		eth_mdio_read(dev, addr, reg, val);

	return 0;
}

static int dw_mii_write(const char *devname, u8 addr, u8 reg, u16 val)
{
	struct eth_device *dev;

	dev = eth_get_dev_by_name(devname);
	if (dev)
		eth_mdio_write(dev, addr, reg, val);

	return 0;
}
#endif

int dw_phy_read(struct mii_dev *bus, int phyAddr, int dev_addr, int regAddr)
{
	int ret;
	u16 val;

	ret = eth_mdio_read(bus->priv, phyAddr, regAddr, &val);
	if (ret < 0)
		val = -1;

	return (int)val;
}

int dw_phy_write(struct mii_dev *bus, int phyAddr, int dev_addr, int regAddr,
		u16 data)
{
	return eth_mdio_write(bus->priv, phyAddr, regAddr, data);
}

int designware_initialize(u32 id, ulong base_addr, u32 phy_addr, u32 interface)
{
	struct eth_device *dev;
	struct dw_eth_dev *priv;
	struct mii_dev *bus;
#if defined(CONFIG_RANDOM_MACADDR)
	uchar enetaddr[6];
#endif
	int ret = 1;

	dev = (struct eth_device *) malloc(sizeof(struct eth_device));
	if (!dev) {
		puts("dwc_gmac: not enough malloc memory for eth_device\n");
		ret = -ENOMEM;
		goto err1;
	}

	/*
	 * Since the priv structure contains the descriptors which need a strict
	 * buswidth alignment, memalign is used to allocate memory
	 */
	priv = (struct dw_eth_dev *) memalign(16, sizeof(struct dw_eth_dev));
	if (!priv) {
		ret = -ENOMEM;
		goto err2;
	}

	memset(dev, 0, sizeof(struct eth_device));
	memset(priv, 0, sizeof(struct dw_eth_dev));

	sprintf(dev->name, "mii%d", id);
	dev->iobase = (int)base_addr;
	dev->priv = priv;

	eth_getenv_enetaddr_by_index("eth", id, &dev->enetaddr[0]);

#if defined(CONFIG_RANDOM_MACADDR)
	if (!eth_getenv_enetaddr("ethaddr", enetaddr)) {
		eth_random_enetaddr(enetaddr);
		if (eth_setenv_enetaddr("ethaddr", enetaddr)) {
			printf("Failed to set ethernet address\n");
		}
	}
#endif

	priv->dev = dev;
	priv->mac_regs_p = (struct eth_mac_regs *)base_addr;
	priv->dma_regs_p = (struct eth_dma_regs *)(base_addr +
			DW_DMA_BASE_OFFSET);
	priv->address = phy_addr;
	priv->phy_configured = 0;
	priv->link_configured = 0;
	priv->interface = interface;

	dev->init = dw_eth_init;
	dev->send = dw_eth_send;
	dev->recv = dw_eth_recv;
	dev->halt = dw_eth_halt;
	dev->write_hwaddr = dw_write_hwaddr;

	bus = mdio_alloc();
	if (!bus) {
		printf("mdio_alloc failed\n");
		ret = -ENOMEM;
		goto err3;
	}

	bus->read = dw_phy_read;
	bus->write = dw_phy_write;
	sprintf(bus->name, dev->name);

//	bus->priv = priv;
	bus->priv = dev;

	ret = mdio_register(bus);
	if (ret) {
		printf("mdio_register failed\n");
		free(bus);
		ret = -ENOMEM;
		goto err3;
	}
	priv->bus = bus;

	eth_register(dev);

#if defined(CONFIG_MII)
	miiphy_register(dev->name, dw_mii_read, dw_mii_write);
#endif

	return 1;

err3:
	free(priv);
err2:
	free(dev);
err1:
	return ret;
}
