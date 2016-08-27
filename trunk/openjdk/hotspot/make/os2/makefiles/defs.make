#
# Copyright 2006-2008 Sun Microsystems, Inc.  All Rights Reserved.
# DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
#
# This code is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 2 only, as
# published by the Free Software Foundation.
#
# This code is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
# version 2 for more details (a copy is included in the LICENSE file that
# accompanied this code).
#
# You should have received a copy of the GNU General Public License version
# 2 along with this work; if not, write to the Free Software Foundation,
# Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
#
# Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa Clara,
# CA 95054 USA or visit www.sun.com if you need additional information or
# have any questions.
#
#

# The common definitions for hotspot OS/2 builds.
# Include the top level defs.make under make directory instead of this one.
# This file is included into make/defs.make.
# On OS/2 it is only used to construct parameters for
# make/os2/build.make when make/Makefile is used to build VM.

SLASH_JAVA ?= J:
PATH_SEP = ;

# Need PLATFORM (os-arch combo names) for jdk and hotspot, plus libarch name
ARCH_DATA_MODEL=32
PLATFORM=os2-i586
VM_PLATFORM=os2_i486
HS_ARCH=x86
MAKE_ARGS += ARCH=x86
MAKE_ARGS += BUILDARCH=i486
MAKE_ARGS += Platform_arch=x86
MAKE_ARGS += Platform_arch_model=x86_32

JDK_INCLUDE_SUBDIR=os2

ifndef USERNAME
USERNAME := $(USER)
endif

# HOTSPOT_RELEASE_VERSION and HOTSPOT_BUILD_VERSION are defined
# and added to MAKE_ARGS list in $(GAMMADIR)/make/defs.make.

# next parameters are defined in $(GAMMADIR)/make/defs.make.
MAKE_ARGS += JDK_MKTG_VERSION=$(JDK_MKTG_VERSION)
MAKE_ARGS += JDK_MAJOR_VER=$(JDK_MAJOR_VERSION)
MAKE_ARGS += JDK_MINOR_VER=$(JDK_MINOR_VERSION)
MAKE_ARGS += JDK_MICRO_VER=$(JDK_MICRO_VERSION)

ifdef COOKED_JDK_UPDATE_VERSION
  MAKE_ARGS += JDK_UPDATE_VER=$(COOKED_JDK_UPDATE_VERSION)
endif

# COOKED_BUILD_NUMBER should only be set if we have a numeric
# build number.  It must not be zero padded.
ifdef COOKED_BUILD_NUMBER
  MAKE_ARGS += JDK_BUILD_NUMBER=$(COOKED_BUILD_NUMBER)
endif

NMAKE=make

# FIXUP: The subdirectory for a debug build is NOT the same on all platforms
VM_DEBUG=debug

ABS_OUTPUTDIR     := $(shell mkdir -p $(OUTPUTDIR); $(CD) $(OUTPUTDIR); $(PWD))
ABS_BOOTDIR       := $(shell $(CD) $(BOOTDIR); $(PWD))
ABS_GAMMADIR      := $(shell $(CD) $(GAMMADIR); $(PWD))
ABS_OS_MAKEFILE   := $(shell $(CD) $(HS_MAKE_DIR)/$(OSNAME); $(PWD))/build.make

# Disable building SA on OS/2 until we are sure
# we want to release it.  If we build it here,
# the SDK makefiles will copy it over and put it into
# the created image.
BUILD_OS2_SA = 0
ifneq ($(ALT_BUILD_OS2_SA),)
  BUILD_OS2_SA = $(ALT_BUILD_OS2_SA)
endif

ifndef BUILD_CLIENT_ONLY
EXPORT_SERVER_DIR = $(EXPORT_JRE_BIN_DIR)/server
EXPORT_LIST += $(EXPORT_SERVER_DIR)/Xusage.txt
EXPORT_LIST += $(EXPORT_SERVER_DIR)/jvm.dll
EXPORT_LIST += $(EXPORT_SERVER_DIR)/jvm.map
endif

EXPORT_LIST += $(EXPORT_LIB_DIR)/jvm.lib

EXPORT_CLIENT_DIR = $(EXPORT_JRE_BIN_DIR)/client
EXPORT_LIST += $(EXPORT_CLIENT_DIR)/Xusage.txt
EXPORT_LIST += $(EXPORT_CLIENT_DIR)/jvm.dll
EXPORT_LIST += $(EXPORT_CLIENT_DIR)/jvm.map

ifndef BUILD_CLIENT_ONLY
# kernel vm
EXPORT_KERNEL_DIR = $(EXPORT_JRE_BIN_DIR)/kernel
EXPORT_LIST += $(EXPORT_KERNEL_DIR)/Xusage.txt
EXPORT_LIST += $(EXPORT_KERNEL_DIR)/jvm.dll
EXPORT_LIST += $(EXPORT_KERNEL_DIR)/jvm.map
endif

ifeq ($(BUILD_OS2_SA), 1)
  EXPORT_LIST += $(EXPORT_JRE_BIN_DIR)/saos2dbg.dll
  EXPORT_LIST += $(EXPORT_JRE_BIN_DIR)/saos2dbg.map
  EXPORT_LIST += $(EXPORT_LIB_DIR)/sa-jdi.jar
  # Must pass this down to make.
  MAKE_ARGS += BUILD_OS2_SA=1
endif
