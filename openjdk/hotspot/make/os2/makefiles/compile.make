#
# Copyright 1997-2009 Sun Microsystems, Inc.  All Rights Reserved.
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

# Generic compiler settings
CC = gcc
CXX = g++

CC_FLAGS = $(C_FLAGS) $(CPP_FLAGS)
CXX_FLAGS = $(C_FLAGS) $(CPP_FLAGS)

C_FLAGS = -Zomf -march=i486 -mtune=generic

# We compile a lot of MSVC code which seems to silently tolerate these
C_FLAGS += -Wno-sign-compare -Wno-write-strings

CXX_FLAGS += -fno-rtti
CXX_FLAGS += -fno-exceptions
CXX_FLAGS += -fcheck-new

# Based on BUILDARCH we add some flags and select the default compiler name
ifeq ($(BUILDARCH),i486)
CPP_FLAGS += -DIA32
endif

# Hotspot uses very unstrict aliasing turn this optimization off
OPT_CFLAGS = -fno-strict-aliasing

# Compile for space above time.
ifeq ($(Variant),kernel)
PRODUCT_OPT_OPTION   = -s -Os $(OPT_CFLAGS)
FASTDEBUG_OPT_OPTION = -s -Os $(OPT_CFLAGS)
DEBUG_OPT_OPTION     = -g
else
PRODUCT_OPT_OPTION   = -s -O3 $(OPT_CFLAGS)
FASTDEBUG_OPT_OPTION = -s -O3 $(OPT_CFLAGS)
DEBUG_OPT_OPTION     = -g
endif

# If NO_OPTIMIZATIONS is defined in the environment, turn everything off
ifdef NO_OPTIMIZATIONS
PRODUCT_OPT_OPTION   = $(DEBUG_OPT_OPTION)
FASTDEBUG_OPT_OPTION = $(DEBUG_OPT_OPTION)
endif

# Used for platform dispatching. Note that on OS/2 we pretend to be 'windows'
# (it works in most cases and all differences are usually handled by
# TARGET_COMPILER_gcc).
CPP_FLAGS += -DTARGET_OS_FAMILY_windows
CPP_FLAGS += -DTARGET_ARCH_$(Platform_arch)
CPP_FLAGS += -DTARGET_ARCH_MODEL_$(Platform_arch_model)
CPP_FLAGS += -DTARGET_OS_ARCH_windows_$(Platform_arch)
CPP_FLAGS += -DTARGET_OS_ARCH_MODEL_windows_$(Platform_arch_model)
CPP_FLAGS += -DTARGET_COMPILER_gcc

# Additional define for differences not covered by TARGET_COMPILER_gcc
CPP_FLAGS += -DTARGET_OS_FAMILY_os2

# Generate dependencies
CPP_FLAGS += -MMD -MP -MF $(@:%=%.d)
SUFFIXES += .d

# Generic linker settings
LINK = g++
LINK_FLAGS = \
 -Zomf -Zmap -Zstack 0x2000 -Zhigh-mem -Zno-fork -Zno-unix

ifeq ($(EMXOMFLD_TYPE), WLINK)
  LINK_FLAGS += -Zlinker "DISABLE 1121"
  MAPSYM = wmapsym.cmd
endif
ifeq ($(EMXOMFLD_TYPE), VAC308)
  LINK_FLAGS += -Zlinker /OPTFUNC
  MAPSYM = mapsym.exe
endif
 
IMPLIB = emximp

# Odin SDK

CPP_FLAGS += -D__WIN32OS2__ -D__i386__ -DSTRICT -D_POSIX_SOURCE \
             -D_POSIX_C_SOURCE=200112 -D_EMX_SOURCE -D_XOPEN_SOURCE=600 \
        	 -D_SVID_SOURCE
CPP_FLAGS += -I$(ALT_ODINSDK_HEADERS_PATH)/Win -I$(ALT_ODINSDK_HEADERS_PATH)

PRODUCT_LINK_FLAGS      += -s -L$(ALT_ODINSDK_LIB_PATH)
FASTDEBUG_LINK_FLAGS    += -s -L$(firstword $(ALT_ODINSDK_DBGLIB_PATH) $(ALT_ODINSDK_LIB_PATH))
DEBUG_LINK_FLAGS        += -g -L$(firstword $(ALT_ODINSDK_DBGLIB_PATH) $(ALT_ODINSDK_LIB_PATH))

LINK_FLAGS  += -lkernel32.lib -luser32.lib -lgdi32.lib -lwinspool.lib \
               -lcomdlg32.lib -ladvapi32.lib -lshell32.lib -lole32.lib \
               -loleaut32.lib -lWsock32.lib -lwinmm.lib -llibwrap.lib
