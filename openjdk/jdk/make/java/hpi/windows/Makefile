#
# Copyright (c) 1999, 2005, Oracle and/or its affiliates. All rights reserved.
# DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
#
# This code is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 2 only, as
# published by the Free Software Foundation.  Oracle designates this
# particular file as subject to the "Classpath" exception as provided
# by Oracle in the LICENSE file that accompanied this code.
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
# Please contact Oracle, 500 Oracle Parkway, Redwood Shores, CA 94065 USA
# or visit www.oracle.com if you need additional information or have any
# questions.
#

#
# Makefile for Windows HPI DLL
#
BUILDDIR = ../../..
LIBRARY   = hpi
PRODUCT      = java
THREADDIR    = windows_threads
LIB_LOCATION = $(BINDIR)

ifeq ($(PLATFORM), os2)
LIBRARY_SHORT = jhpi
endif

include $(BUILDDIR)/common/Defs.gmk

# windows compiler flags
ifeq ($(PLATFORM),windows)
  CPPFLAGS_DBG += -DLOGGING
  # Files built here do not compile with warning level 3 if warnings are fatal
  COMPILER_WARNINGS_FATAL=false
endif

FILES_c = \
    linker_md.c \
    memory_md.c \
    monitor_md.c \
    path_md.c \
    socket_md.c \
    sys_api_md.c \
    system_md.c \
    threads_md.c \
    hpi.c # trailing blank required!

JVMLIB = 
JAVALIB =
EXTRA_LIBS =

ifeq ($(PLATFORM),windows)
OTHER_LCF = -export:DLL_Initialize
endif

ifeq ($(PLATFORM),os2)
# exports live in .def file
DEF_FILE = $(OBJDIR)/$(LIBRARY).def
$(DEF_FILE): $(MAKEFILE_LIST)
	@$(ECHO) "LIBRARY $(LIBRARY_NAME) INITINSTANCE TERMINSTANCE" > $@
	@$(ECHO) "EXPORTS\n  DLL_Initialize\n" >> $@
clean::
	-$(RM) $(DEF_FILE)
endif

#
# Other files/flags shared between the HPIs.
#
include $(BUILDDIR)/java/hpi/hpi_common.gmk

#
# Rules for the .so file.
#
include $(BUILDDIR)/common/Library.gmk

