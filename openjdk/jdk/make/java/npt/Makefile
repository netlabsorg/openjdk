#
# Copyright (c) 2004, 2005, Oracle and/or its affiliates. All rights reserved.
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

BUILDDIR = ../..
LIBRARY = npt
PRODUCT = sun
LIBRARY_OUTPUT = npt

ifeq ($(PLATFORM), os2)
LIBRARY_SHORT = jnpt
endif

# Configure the CFLAGS for this library.

FILES_m = mapfile-vers

include $(BUILDDIR)/common/Defs.gmk

SRCDIR=$(SHARE_SRC)/npt
PSRCDIR=$(PLATFORM_SRC)/npt

# Use the mapfile-vers (See the mapfile located with this Makefile)
ifdef FILES_m
include $(BUILDDIR)/common/Mapfile-vers.gmk
endif

#
# Files to compile.
#
FILES_c = 			\
	npt.c 	\
	utf.c 	\
	utf_md.c

OTHER_INCLUDES = -I$(SRCDIR) -I$(PSRCDIR)

#
# This removes all asserts in the optimized version
#
CPPFLAGS_OPT += -DNDEBUG

# We don't want to link against -ljava
JAVALIB=

# Add -export options to explicitly spell exported symbols
ifeq ($(PLATFORM), windows)
  OTHER_LCF += -export:nptInitialize -export:nptTerminate
endif

ifeq ($(PLATFORM), os2)

# exports live in .def file
DEF_FILE = $(OBJDIR)/$(LIBRARY).def
$(DEF_FILE): $(MAKEFILE_LIST)
	@$(ECHO) "LIBRARY $(LIBRARY_NAME) INITINSTANCE TERMINSTANCE" > $@
	@$(ECHO) "EXPORTS\n\
  nptInitialize\n\
  nptTerminate\n\
" >> $@

clean::
	-$(RM) $(DEF_FILE)

endif

#
# Library to compile.
#
include $(BUILDDIR)/common/Library.gmk

#
# Add to ambient vpath so we pick up the library files
#
vpath %.c $(SRCDIR) $(PSRCDIR)

