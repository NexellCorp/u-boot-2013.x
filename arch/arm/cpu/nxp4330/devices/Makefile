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
LIB 	= $(obj)lib$(CPU)-devices.o

ARCH_CFLAGS	+= 	-I$(TOPDIR)/include

COBJS-$(CONFIG_PWM)					+= pwm.o
COBJS-$(CONFIG_MTD_NAND_NEXELL)		+= nand.o
COBJS-$(CONFIG_MTD_NAND_ECC_HW)		+= nand_ecc.o
COBJS-$(CONFIG_CMD_I2C)			+= i2c.o
COBJS-$(CONFIG_LCD)			+= lcd.o
COBJS-$(CONFIG_SPI)			+= spi.o
COBJS-$(CONFIG_PMIC_NXE1100)		+= nxe1100_power.o
COBJS-$(CONFIG_PMIC_NXE2000)		+= nxe2000_power.o
COBJS-$(CONFIG_POWER_BATTERY_NXE2000)	+= nxe2000_bat.o
COBJS-$(CONFIG_POWER_FG_NXE2000)	+= nxe2000_fg.o
COBJS-$(CONFIG_POWER_NXE2000)		+= nxe2000_pmic.o
COBJS-$(CONFIG_POWER_MUIC_NXE2000)	+= nxe2000_muic.o
COBJS-$(CONFIG_NXP_DWMMC)		+= nxp_dw_mmc.o
COBJS-$(CONFIG_DISPLAY_OUT)		+= display_dev.o
COBJS-$(CONFIG_DISPLAY_OUT_LVDS)	+= display_lvds.o
COBJS-$(CONFIG_DISPLAY_OUT_HDMI)	+= display_hdmi.o
COBJS-$(CONFIG_DISPLAY_OUT_MIPI)	+= display_mipi.o
COBJS-$(CONFIG_NXP4330_GPIO)		+= nxp_gpio.o
COBJS-$(CONFIG_NXP_RTC_USE)			+= nxp_rtc.o

COBJS   := $(COBJS-y)
SOBJS   := $(SOBJS-y)

SRCS	:= $(SOBJS:.o=.S) $(COBJS:.o=.c)
OBJS	:= $(addprefix $(obj),$(SOBJS) $(COBJS))

all:	$(obj).depend $(LIB)

$(LIB): $(OBJS)
	$(call cmd_link_o_target, $(OBJS))


#########################################################################

# defines $(obj).depend target
include $(SRCTREE)/rules.mk

sinclude $(obj).depend

#########################################################################
