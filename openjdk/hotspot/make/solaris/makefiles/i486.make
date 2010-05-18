#
# Copyright 1998-2008 Sun Microsystems, Inc.  All Rights Reserved.
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

# Must also specify if CPU is little endian
CFLAGS += -DVM_LITTLE_ENDIAN

# TLS helper, assembled from .s file
# Not included in includeDB because it has no dependencies
Obj_Files += solaris_x86_32.o

#
# Special case flags for compilers and compiler versions on i486.
#
ifeq ("${Platform_compiler}", "sparcWorks")

# _lwp_create_interpose must have a frame
OPT_CFLAGS/os_solaris_x86.o = -xO1
else

ifeq ("${Platform_compiler}", "gcc")
# gcc
# _lwp_create_interpose must have a frame
OPT_CFLAGS/os_solaris_x86.o = -fno-omit-frame-pointer
#
else
# error
_JUNK2_ := $(shell echo >&2 \
       "*** ERROR: this compiler is not yet supported by this code base!")
	@exit 1
endif
endif

ifeq ("${Platform_compiler}", "sparcWorks")
# ILD is gone as of SS11 (5.8), not supported in SS10 (5.7)
ifeq ($(shell expr $(COMPILER_REV_NUMERIC) \< 507), 1)
  #
  # Bug in ild causes it to fail randomly. Until we get a fix we can't
  # use ild.
  #
  ILDFLAG/debug     = -xildoff
endif
endif
