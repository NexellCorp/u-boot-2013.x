#
#

include $(TOPDIR)/config.mk
include prototype.mk

LIB		 = $(obj)lib$(SOC).o

COBJS-y	+= cache_v7.o
COBJS-y	+= cpu.o
COBJS-y	+= nxp4330.o
COBJS-y += clock.o
COBJS-y	+= reset.o
COBJS-y	+= timer.o

COBJS-$(CONFIG_MMU_ENABLE) += mmu.o
SOBJS-$(CONFIG_MMU_ENABLE) += mmu_func.o

SOBJS-y	+= low_init.o



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
