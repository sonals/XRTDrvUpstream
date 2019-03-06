#
# Copyright (C) 2016-2018 Xilinx, Inc. All rights reserved.
#
# Authors:
#
# This software is licensed under the terms of the GNU General Public
# License version 2, as published by the Free Software Foundation, and
# may be copied, distributed, and modified under those terms.
#
# This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#

obj-$(CONFIG_DRM_XOCL)	+= xocl.o

include $(src)/../lib/Makefile.in

xocl-y := \
	../xocl_subdev.o \
	../xocl_ctx.o \
	../xocl_thread.o \
	../subdev/xdma.o \
	../subdev/feature_rom.o \
	../subdev/mb_scheduler.o \
	../subdev/mailbox.o \
	../subdev/xvc.o \
	../subdev/icap.o \
	../subdev/xmc.o \
	$(xocl_lib-y)	\
	xocl_drv.o	\
	xocl_bo.o	\
	xocl_drm.o	\
	xocl_ioctl.o	\
	xocl_sysfs.o


ccflags-y += -DSUBDEV_SUFFIX=USER_SUFFIX
ifeq ($(DEBUG),1)
ccflags-y += -DDEBUG
endif
