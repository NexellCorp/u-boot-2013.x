#
# (C) Copyright 2002
# Gary Jennejohn, DENX Software Engineering, <garyj@denx.de>
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
PLATFORM_RELFLAGS += -fno-common -ffixed-r8 -msoft-float

# Make ARMv5 to allow more compilers to work, even though its v6.
PLATFORM_CPPFLAGS += -march=armv5

# =========================================================================
#
# Supply options according to compiler version
#
# =========================================================================
PF_RELFLAGS_SLB_AT := $(call cc-option,-mshort-load-bytes,$(call cc-option,-malignment-traps,))
PLATFORM_RELFLAGS += $(PF_RELFLAGS_SLB_AT)

# =========================================================================
# For EABI, make sure to provide raise()
# =========================================================================
PLATFORM_LIBS 	:= $(OBJTREE)/arch/arm/lib/eabi_compat.o \
	$(filter-out %/arch/arm/lib/eabi_compat.o, $(PLATFORM_LIBS))

# =========================================================================
# Add cpu common library
# =========================================================================
LIBCOMMON_PATH 	:= common
LIBCOMMON 	   	:= $(obj)lib$(CPU)-common.o
PLATFORM_LIBS  	:= $(OBJTREE)/arch/arm/cpu/$(CPU)/$(LIBCOMMON_PATH)/$(LIBCOMMON) \
	$(filter-out %/arch/arm/cpu/$(CPU)/$(LIBCOMMON_PATH)/$(LIBCOMMON), $(PLATFORM_LIBS))

# =========================================================================
# Add cpu device library
# =========================================================================
ifeq ($(BOARD),3200)
LIBDEVICE_PATH 	:= devices_3200
else
LIBDEVICE_PATH 	:= devices
endif

LIBDEVICE 		:= $(obj)lib$(CPU)-devices.o
PLATFORM_LIBS 	:= $(OBJTREE)/arch/arm/cpu/$(CPU)/$(LIBDEVICE_PATH)/$(LIBDEVICE) \
	$(filter-out %/arch/arm/cpu/$(CPU)/$(LIBDEVICE_PATH)/$(LIBDEVICE), $(PLATFORM_LIBS))

