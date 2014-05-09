
#
# (C) Copyright 2009
# jung hyun kim, Nexell Co, <jhkim@nexell.co.kr>
#
# See file CREDITS for list of people who contributed to this
# project.
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 2 of
# the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston,
# MA 02111-1307 USA
#

include $(TOPDIR)/config.mk
LIB	 	 = $(obj)lib$(CPU)-common.o

COBJS-y	+= cmd.o
COBJS-$(CONFIG_LOGO_NEXELL)		+= cmd_draw_logo.o cmd_load_logo.o draw_lcd.o
COBJS-$(CONFIG_CMD_SPI_EEPROM_UPDATE)   += cmd_spi_eeprom.o
COBJS-$(CONFIG_CMD_UPDATE_NAND)   += cmd_update_nand.o
COBJS-$(CONFIG_OTG_PHY_NEXELL)		+= otg_phy.o
COBJS-$(CONFIG_NXP_USBD)		+= cmd_usbdown.o
COBJS-$(CONFIG_MMC_PARTITIONS)		+= cmd_mmcparts.o
COBJS-$(CONFIG_FASTBOOT)		+= cmd_fastboot.o otg_bind_chk.o
COBJS-$(CONFIG_CMD_MMC_UPDATE)	+= cmd_mmc_update.o

COBJS   := $(COBJS-y)
SOBJS   := $(SOBJS-y)

SRCS	:= $(SOBJS:.o=.S) $(COBJS:.o=.c)
OBJS	:= $(addprefix $(obj),$(SOBJS) $(COBJS))

all:	$(obj).depend $(LIB)

$(LIB):	$(OBJS)
	$(call cmd_link_o_target, $(OBJS))


#########################################################################

# defines $(obj).depend target
include $(SRCTREE)/rules.mk

sinclude $(obj).depend

#########################################################################
