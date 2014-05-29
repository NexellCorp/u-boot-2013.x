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

#ifndef _DW_ETH_H
#define _DW_ETH_H

#define CONFIG_TX_DESCR_NUM 16
#define CONFIG_RX_DESCR_NUM 16
#define CONFIG_ETH_BUFSIZE  2048
#define TX_TOTAL_BUFSIZE    (CONFIG_ETH_BUFSIZE * CONFIG_TX_DESCR_NUM)
#define RX_TOTAL_BUFSIZE    (CONFIG_ETH_BUFSIZE * CONFIG_RX_DESCR_NUM)

#define CONFIG_MACRESET_TIMEOUT (3 * CONFIG_SYS_HZ)
#define CONFIG_MDIO_TIMEOUT     (3 * CONFIG_SYS_HZ)
#define CONFIG_PHYRESET_TIMEOUT (3 * CONFIG_SYS_HZ)
#define CONFIG_AUTONEG_TIMEOUT  (5 * CONFIG_SYS_HZ)

struct eth_mac_regs {
    u32 conf;           /* 0x00 */
    u32 framefilt;      /* 0x04 */
    u32 hashtablehigh;  /* 0x08 */
    u32 hashtablelow;   /* 0x0c */
    u32 miiaddr;        /* 0x10 */
    u32 miidata;        /* 0x14 */
    u32 flowcontrol;    /* 0x18 */
    u32 vlantag;        /* 0x1c */
    u32 version;        /* 0x20 */
    u8 reserved_1[20];
    u32 intreg;         /* 0x38 */
    u32 intmask;        /* 0x3c */
    u32 macaddr0hi;     /* 0x40 */
    u32 macaddr0lo;     /* 0x44 */
};

/* MAC configuration register definitions */
#define SARC3               (3 << 28)
#define SARC2               (2 << 28)
#define TWOKPE_802_3        (1 << 27)
#define FRAMEBURSTENABLE    (1 << 21)
#define MII_PORTSELECT      (1 << 15)
#define FES_100             (1 << 14)
#define DISABLERXOWN        (1 << 13)
#define FULLDPLXMODE        (1 << 11)
#define RXENABLE            (1 << 2)
#define TXENABLE            (1 << 3)

/* MII address register definitions */
#define MII_BUSY            (1 << 0)
#define MII_WRITE           (1 << 1)

#define MII_CLKRANGE_60_100M    (0)     /* DIV 42 */
#define MII_CLKRANGE_100_150M   (0x4)   /* DIV 62 */
#define MII_CLKRANGE_20_35M     (0x8)   /* DIV 16 */
#define MII_CLKRANGE_35_60M     (0xC)   /* DIV 26 */
#define MII_CLKRANGE_150_250M   (0x10)  /* DIV 102 */
#define MII_CLKRANGE_250_300M   (0x14)  /* DIV 124 */

#define MII_CLKRANGE_DIV_4      (0x20)
#define MII_CLKRANGE_DIV_6      (0x24)
#define MII_CLKRANGE_DIV_8      (0x28)
#define MII_CLKRANGE_DIV_10     (0x2C)
#define MII_CLKRANGE_DIV_12     (0x30)
#define MII_CLKRANGE_DIV_14     (0x34)
#define MII_CLKRANGE_DIV_16     (0x38)
//#define MII_CLKRANGE_DIV_16     (0x8)
#define MII_CLKRANGE_DIV_18     (0x3C)
#define MII_CLKRANGE_DIV_26     (0xC)
#define MII_CLKRANGE_DIV_42     (0)
#define MII_CLKRANGE_DIV_62     (0x4)
#define MII_CLKRANGE_DIV_102    (0x10)
#define MII_CLKRANGE_DIV_124    (0x14)

#define MIIADDRSHIFT    (11)
#define MIIREGSHIFT     (6)
#define MII_REGMSK      (0x1F << 6)
#define MII_ADDRMSK     (0x1F << 11)


struct eth_dma_regs {
    u32 busmode;                /* 0x00 */
    u32 txpolldemand;           /* 0x04 */
    u32 rxpolldemand;           /* 0x08 */
    u32 rxdesclistaddr;         /* 0x0c */
    u32 txdesclistaddr;         /* 0x10 */
    u32 status;                 /* 0x14 */
    u32 opmode;                 /* 0x18 */
    u32 intenable;              /* 0x1c */
    u32 missframbuffoverflow;   /* 0x20 */
    u32 rxintwatchdog;          /* 0x24 */
    u32 axibusmode;             /* 0x28 */
    u32 axistatus;              /* 0x2c */
    u8 reserved[24];
    u32 currhosttxdesc;         /* 0x48 */
    u32 currhostrxdesc;         /* 0x4c */
    u32 currhosttxbuffaddr;     /* 0x50 */
    u32 currhostrxbuffaddr;     /* 0x54 */
    u32 hwfeatures;             /* 0x58 */
};

#define DW_DMA_BASE_OFFSET  (0x1000)

/* Bus mode register definitions */
#define PBLx8           (1 << 24)
#define RPBL_SHIFT      (17)
#define FIXEDBURST      (1 << 16)
#define PRIORXTX_41     (3 << 14)
#define PRIORXTX_31     (2 << 14)
#define PRIORXTX_21     (1 << 14)
#define PRIORXTX_11     (0 << 14)
#define PBL_SHIFT       (8)
#define BURST_1         (1)
#define BURST_2         (2)
#define BURST_4         (4)
#define BURST_8         (8)
#define BURST_16        (16)
#define BURST_32        (32)
#define RXHIGHPRIO      (1 << 1)
#define DMAMAC_SRST     (1 << 0)

/* Poll demand definitions */
#define POLL_DATA       (0xFFFFFFFF)

/* Operation mode definitions */
#define STOREFORWARD    (1 << 21)
#define FLUSHTXFIFO     (1 << 20)
#define TXSTART         (1 << 13)
#define TXSECONDFRAME   (1 << 2)
#define RXSTART         (1 << 1)

/* AXI bus mode definitions */
#define WROSRLMT_SHIFT      (20)
#define RDOSRLMT_SHIFT      (16)
#define AXI_BURST_4         (1 << 1)
#define AXI_BURST_8         (1 << 2)
#define AXI_BURST_16        (1 << 3)
#define AXI_BURST_32        (1 << 4)
#define AXI_BURST_64        (1 << 5)
#define AXI_BURST_128       (1 << 6)
#define AXI_BURST_256       (1 << 7)


/* Descriptior related definitions */
#define MAC_MAX_FRAME_SZ    (1600)
//#define MAC_MAX_FRAME_SZ    (1536)
//#define MAC_MAX_FRAME_SZ    (1664)


struct dmamacdescr {
    u32 txrx_status;
    u32 dmamac_cntl;
    void *dmamac_addr;
    struct dmamacdescr *dmamac_next;
} __aligned(ARCH_DMA_MINALIGN);

/*
 * txrx_status definitions
 */

/* tx status bits definitions */
#if defined(CONFIG_DW_ALTDESCRIPTOR)

#define DESC_TXSTS_OWNBYDMA         (1 << 31)
#define DESC_TXSTS_TXINT            (1 << 30)
#define DESC_TXSTS_TXLAST           (1 << 29)
#define DESC_TXSTS_TXFIRST          (1 << 28)
#define DESC_TXSTS_TXCRCDIS         (1 << 27)

#define DESC_TXSTS_TXPADDIS         (1 << 26)
#define DESC_TXSTS_TXCHECKINSCTRL   (3 << 22)
#define DESC_TXSTS_TXRINGEND        (1 << 21)
#define DESC_TXSTS_TXCHAIN          (1 << 20)
#define DESC_TXSTS_MSK              (0x1FFFF << 0)

#else

#define DESC_TXSTS_OWNBYDMA         (1 << 31)
#define DESC_TXSTS_MSK              (0x1FFFF << 0)

#endif

/* rx status bits definitions */
#define DESC_RXSTS_OWNBYDMA         (1 << 31)
#define DESC_RXSTS_DAFILTERFAIL     (1 << 30)
#define DESC_RXSTS_FRMLENMSK        (0x3FFF << 16)
#define DESC_RXSTS_FRMLENSHFT       (16)

#define DESC_RXSTS_ERROR            (1 << 15)
#define DESC_RXSTS_RXTRUNCATED      (1 << 14)
#define DESC_RXSTS_SAFILTERFAIL     (1 << 13)
#define DESC_RXSTS_RXIPC_GIANTFRAME (1 << 12)
#define DESC_RXSTS_RXDAMAGED        (1 << 11)
#define DESC_RXSTS_RXVLANTAG        (1 << 10)
#define DESC_RXSTS_RXFIRST          (1 << 9)
#define DESC_RXSTS_RXLAST           (1 << 8)
#define DESC_RXSTS_RXIPC_GIANT      (1 << 7)
#define DESC_RXSTS_RXCOLLISION      (1 << 6)
#define DESC_RXSTS_RXFRAMEETHER     (1 << 5)
#define DESC_RXSTS_RXWATCHDOG       (1 << 4)
#define DESC_RXSTS_RXMIIERROR       (1 << 3)
#define DESC_RXSTS_RXDRIBBLING      (1 << 2)
#define DESC_RXSTS_RXCRC            (1 << 1)

/*
 * dmamac_cntl definitions
 */

/* tx control bits definitions */
#if defined(CONFIG_DW_ALTDESCRIPTOR)

#define DESC_TXCTRL_SIZE1MASK       (0x1FFF << 0)
#define DESC_TXCTRL_SIZE1SHFT       (0)
#define DESC_TXCTRL_SIZE2MASK       (0x1FFF << 16)
#define DESC_TXCTRL_SIZE2SHFT       (16)

#else

#define DESC_TXCTRL_TXINT           (1 << 31)
#define DESC_TXCTRL_TXLAST          (1 << 30)
#define DESC_TXCTRL_TXFIRST         (1 << 29)
#define DESC_TXCTRL_TXCHECKINSCTRL  (3 << 27)
#define DESC_TXCTRL_TXCRCDIS        (1 << 26)
#define DESC_TXCTRL_TXRINGEND       (1 << 25)
#define DESC_TXCTRL_TXCHAIN         (1 << 24)

#define DESC_TXCTRL_SIZE1MASK       (0x7FF << 0)
#define DESC_TXCTRL_SIZE1SHFT       (0)
#define DESC_TXCTRL_SIZE2MASK       (0x7FF << 11)
#define DESC_TXCTRL_SIZE2SHFT       (11)

#endif

/* rx control bits definitions */
#if defined(CONFIG_DW_ALTDESCRIPTOR)

#define DESC_RXCTRL_RXINTDIS        (1 << 31)
#define DESC_RXCTRL_RXRINGEND       (1 << 15)
#define DESC_RXCTRL_RXCHAIN         (1 << 14)

#define DESC_RXCTRL_SIZE1MASK       (0x1FFF << 0)
#define DESC_RXCTRL_SIZE1SHFT       (0)
#define DESC_RXCTRL_SIZE2MASK       (0x1FFF << 16)
#define DESC_RXCTRL_SIZE2SHFT       (16)

#else

#define DESC_RXCTRL_RXINTDIS        (1 << 31)
#define DESC_RXCTRL_RXRINGEND       (1 << 25)
#define DESC_RXCTRL_RXCHAIN         (1 << 24)

#define DESC_RXCTRL_SIZE1MASK       (0x7FF << 0)
#define DESC_RXCTRL_SIZE1SHFT       (0)
#define DESC_RXCTRL_SIZE2MASK       (0x7FF << 11)
#define DESC_RXCTRL_SIZE2SHFT       (11)

#endif

#define DMA_DESCR_PAD_NUM           2
#define DMA_BUFF_PAD_SIZE           16
#define DMA_BUFF_ALIGN_MASK         0xFFFFFFF0


struct dw_eth_dev {
    u32 address;
    u32 interface;
    u32 speed;
    u32 duplex;
    u32 tx_currdescnum;
    u32 rx_currdescnum;
    u32 phy_configured;
    u32 link_configured;
    int link_printed;

    u32 padding_0[4];
    struct dmamacdescr tx_mac_descrtable[CONFIG_TX_DESCR_NUM + DMA_DESCR_PAD_NUM];

    u32 padding_1[4];
    struct dmamacdescr rx_mac_descrtable[CONFIG_RX_DESCR_NUM + DMA_DESCR_PAD_NUM];

    u32 padding_2[4];
    char txbuffs[TX_TOTAL_BUFSIZE + CONFIG_ETH_BUFSIZE];

    u32 padding_3[4];
    char rxbuffs[RX_TOTAL_BUFSIZE + CONFIG_ETH_BUFSIZE];

    u32 padding_4[4];

    struct eth_mac_regs *mac_regs_p;
    struct eth_dma_regs *dma_regs_p;

    struct eth_device *dev;
    struct phy_device *phydev;
    struct mii_dev *bus;
} __attribute__ ((aligned(8)));


#if defined(CONFIG_DW_ALTDESCRIPTOR)

struct EMAC_DMA_DESC_T
{

    /* Receive descriptor */
    union
    {
        u32     data;   /* RDES0 or TDES0 */
        struct
        {
            /* RDES0 */
            u32     reserved1:1;
            u32     crc_error:1;
            u32     dribbling:1;
            u32     mii_error:1;
            u32     receive_watchdog:1;
            u32     frame_type:1;
            u32     collision:1;
            u32     frame_too_long:1;
            u32     last_descriptor:1;
            u32     first_descriptor:1;
            u32     multicast_frame:1;
            u32     runt_frame:1;
            u32     length_error:1;
            u32     partial_frame_error:1;
            u32     descriptor_error:1;
            u32     error_summary:1;
            u32     frame_length:14;
            u32     filtering_fail:1;
            u32     own:1;
        } rx;

        struct
        {
            /* Enhanced TDES0 */
            u32     deferred:1;
            u32     underflow_error:1;
            u32     excessive_deferral:1;
            u32     collision_count:4;
            u32     heartbeat_fail:1;
            u32     excessive_collisions:1;
            u32     late_collision:1;
            u32     no_carrier:1;
            u32     loss_carrier:1;
            u32     reserved1:3;
            u32     error_summary:1;
            u32     reserved2:2;
            u32     reserved3:2;                //[19:18] VLAN Insertion Control
            u32     second_address_chained:1;   //[20]
            u32     end_ring:1;                 //[21]
            u32     checksum_insert:2;          //[23:22]
            u32     reserved4:2;                //[25:24] Transmit Timestamp Enable, CRC Replacement Contr (N.C)
            u32     disable_padding:1;          //[26]
            u32     crc_disable:1;              //[27]
            u32     first_segment:1;            //[28]
            u32     last_segment:1;             //[29]
            u32     interrupt:1;                //[30]
            u32     own:1;                      //[31]
        } tx;
    } des0;

    union
    {
        u32     data;

        struct
        {
            /* Enhanced RDES1 */
            u32     buffer1_size:13;
            u32     reserved2:1;
            u32     second_address_chained:1;
            u32     end_ring:1;
            u32     buffer2_size:13;
            u32     reserved3:2;
            u32     disable_ic:1;
        } rx;

        struct
        {
            /* Enhanced TDES1 */
            u32     buffer1_size:13;
            u32     reserved4:3;
            u32     buffer2_size:13;
            u32     reserved5:3;
        } tx;
    } des1;

    void *des2;
    void *des3;
} __attribute__((aligned(8)));
#else


struct EMAC_DMA_DESC_T
{
    /* Receive descriptor */
    union
    {
        u32     data;   /* RDES0 or TDES0 */

        struct
        {
            /* RDES0 */
            u32     reserved1:1;
            u32     crc_error:1;
            u32     dribbling:1;
            u32     mii_error:1;
            u32     receive_watchdog:1;
            u32     frame_type:1;
            u32     collision:1;
            u32     frame_too_long:1;
            u32     last_descriptor:1;
            u32     first_descriptor:1;
            u32     multicast_frame:1;
            u32     runt_frame:1;
            u32     length_error:1;
            u32     partial_frame_error:1;
            u32     descriptor_error:1;
            u32     error_summary:1;
            u32     frame_length:14;
            u32     filtering_fail:1;
            u32     own:1;
        } rx;

        struct
        {
            /* TDES0 */
            u32     deferred:1;
            u32     underflow_error:1;
            u32     excessive_deferral:1;
            u32     collision_count:4;
            u32     heartbeat_fail:1;
            u32     excessive_collisions:1;
            u32     late_collision:1;
            u32     no_carrier:1;
            u32     loss_carrier:1;
            u32     reserved1:3;
            u32     error_summary:1;
            u32     reserved2:15;
            u32     own:1;
        } tx;
    } des0;

    union
    {
        u32     data;

        struct
        {
            /* RDES1 */
            u32     buffer1_size:11;
            u32     buffer2_size:11;
            u32     reserved2:2;
            u32     second_address_chained:1;
            u32     end_ring:1;
            u32     reserved3:5;
            u32     disable_ic:1;
        } rx;

        struct
        {
            /* TDES1 */
            u32     buffer1_size:11;
            u32     buffer2_size:11;
            u32     reserved3:1;
            u32     disable_padding:1;
            u32     second_address_chained:1;
            u32     end_ring:1;
            u32     crc_disable:1;
            u32     checksum_insert:2;
            u32     first_segment:1;
            u32     last_segment:1;
            u32     interrupt:1;
        } tx;
    } des1;

    void *des2;
    void *des3;
} __attribute__((aligned(8)));

#endif

/* Speed specific definitions */
#define SPEED_10M       1
#define SPEED_100M      2
#define SPEED_1000M     3

/* Duplex mode specific definitions */
#define HALF_DUPLEX     1
#define FULL_DUPLEX     2

#endif
