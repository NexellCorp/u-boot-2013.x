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

COBJS-y := ../$(PROTOTYPE)/$(BASEDIR)/nx_bit_accessor.o

COBJS-y += ../$(PROTOTYPE)/$(MODULES)/nx_rstcon.o
COBJS-y += ../$(PROTOTYPE)/$(MODULES)/nx_clkgen.o
COBJS-y += ../$(PROTOTYPE)/$(MODULES)/nx_clkpwr.o
COBJS-y += ../$(PROTOTYPE)/$(MODULES)/nx_mcus.o
COBJS-y += ../$(PROTOTYPE)/$(MODULES)/nx_alive.o
COBJS-y += ../$(PROTOTYPE)/$(MODULES)/nx_gpio.o
COBJS-y += ../$(PROTOTYPE)/$(MODULES)/nx_pwm.o
#juno
COBJS-y += ../$(PROTOTYPE)/$(MODULES)/nx_usb20host.o
COBJS-y += ../$(PROTOTYPE)/$(MODULES)/nx_usb20otg.o
COBJS-y += ../$(PROTOTYPE)/$(MODULES)/nx_tieoff.o

COBJS-$(CONFIG_CMD_MMC)				+= ../$(PROTOTYPE)/$(MODULES)/nx_sdmmc.o
COBJS-$(CONFIG_SPI)					+= ../$(PROTOTYPE)/$(MODULES)/nx_ssp.o

COBJS-$(CONFIG_DISPLAY_OUT)			+= 	../$(PROTOTYPE)/$(MODULES)/nx_displaytop.o		\
								   		../$(PROTOTYPE)/$(MODULES)/nx_disptop_clkgen.o	\
								   		../$(PROTOTYPE)/$(MODULES)/nx_dualdisplay.o		\
								   		../$(PROTOTYPE)/$(MODULES)/nx_mlc.o				\
								   		../$(PROTOTYPE)/$(MODULES)/nx_dpc.o

COBJS-$(CONFIG_DISPLAY_OUT_LVDS)	+= ../$(PROTOTYPE)/$(MODULES)/nx_lvds.o
COBJS-$(CONFIG_DISPLAY_OUT_MIPI)	+= ../$(PROTOTYPE)/$(MODULES)/nx_mipi.o
COBJS-$(CONFIG_DISPLAY_OUT_RESCONV)	+= ../$(PROTOTYPE)/$(MODULES)/nx_resconv.o

COBJS-$(CONFIG_DISPLAY_OUT_HDMI)	+= ../$(PROTOTYPE)/$(MODULES)/nx_hdmi.o
COBJS-$(CONFIG_DISPLAY_OUT_HDMI)	+= ../$(PROTOTYPE)/$(MODULES)/nx_ecid.o
COBJS-$(CONFIG_DISPLAY_OUT_HDMI)	+= ../$(PROTOTYPE)/$(MODULES)/nx_i2s.o
# COBJS-$(CONFIG_DISPLAY_OUT_HDMI)	+= 	../$(PROTOTYPE)/$(MODULES)/nx_displaytop.o		\
# 								   		../$(PROTOTYPE)/$(MODULES)/nx_disptop_clkgen.o	\
# 								   		../$(PROTOTYPE)/$(MODULES)/nx_dualdisplay.o		\
# 								   		../$(PROTOTYPE)/$(MODULES)/nx_mlc.o				\
# 								   		../$(PROTOTYPE)/$(MODULES)/nx_dpc.o
COBJS-y	+= ../$(PROTOTYPE)/$(MODULES)/nx_rtc.o
COBJS-$(CONFIG_NXP4330_VIP)		    += ../$(PROTOTYPE)/$(MODULES)/nx_vip.o
