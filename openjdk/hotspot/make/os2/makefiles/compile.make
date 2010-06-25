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
CXX = g++

CXX_FLAGS = -Zomf -march=i486 -mtune=generic

# Based on BUILDARCH we add some flags and select the default compiler name
ifeq ($(BUILDARCH),i486)
CXX_FLAGS += -DIA32
endif

# Compile for space above time.
ifeq ($(Variant),kernel)
PRODUCT_OPT_OPTION   = -s -Os
FASTDEBUG_OPT_OPTION = -s -Os
DEBUG_OPT_OPTION     = -g
ese
PRODUCT_OPT_OPTION   = -s -O3
FASTDEBUG_OPT_OPTION = -s -O3
DEBUG_OPT_OPTION     = -g
endif

# If NO_OPTIMIZATIONS is defined in the environment, turn everything off
ifdef NO_OPTIMIZATIONS
PRODUCT_OPT_OPTION   = $(DEBUG_OPT_OPTION)
FASTDEBUG_OPT_OPTION = $(DEBUG_OPT_OPTION)
endif

# Generic linker settings
LINK = g++
LINK_FLAGS = \
 -Zomf -Zmap -Zstack 0x2000 -Zlinker "DISABLE 1121" -Zhigh-mem \

 # @todo
 #kernel32.lib user32.lib gdi32.lib winspool.lib \
 #comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib \
 #uuid.lib Wsock32.lib winmm.lib
