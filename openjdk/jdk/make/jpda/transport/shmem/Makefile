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
# Makefile for building JDI shared memory transport implementation
#

BUILDDIR = ../../..
LIBRARY  = dt_shmem
PRODUCT = jbug

ifeq ($(PLATFORM), os2)
LIBRARY_SHORT = jdtshmem
endif

# Mapfile only used on Solaris/Linux
FILES_m = mapfile-vers

include $(BUILDDIR)/common/Defs.gmk

# 64-bit windows does not build at -W3 if warnings are fatal
ifeq ($(PLATFORM), windows)
  ifeq ($(ARCH_DATA_MODEL), 64)
    COMPILER_WARNINGS_FATAL=false
  endif
endif

FILES_c = \
        SharedMemoryTransport.c \
        SharedMemoryConnection.c \
        shmemBack.c \
        shmemBase.c \
        shmem_md.c

OTHER_INCLUDES = -I$(INCLUDEDIR) -I$(PLATFORM_INCLUDE) \
                 -I$(SHARE_SRC)/transport/export \
                 -I$(SHARE_SRC)/transport/shmem \
                 -I$(SHARE_SRC)/back/export \
                 -I$(PLATFORM_SRC)/transport/shmem

#
# Stuff to feed into javah.
#
FILES_export = \
	com/sun/tools/jdi/SharedMemoryConnection.java \
	com/sun/tools/jdi/SharedMemoryTransportService.java

# Use the mapfile-vers (See the mapfile located with this Makefile)
ifdef FILES_m
  include $(BUILDDIR)/common/Mapfile-vers.gmk
endif

# Add -export options to explicitly spell exported symbols
ifeq ($(PLATFORM), windows)
  OTHER_LCF += -export:jdwpTransport_OnLoad
endif

ifeq ($(PLATFORM), os2)

# exports live in .def file
DEF_FILE = $(OBJDIR)/$(LIBRARY).def
$(DEF_FILE): $(MAKEFILE_LIST)
	@$(ECHO) "LIBRARY $(LIBRARY_NAME) INITINSTANCE TERMINSTANCE" > $@
	@$(ECHO) "EXPORTS\n\
  jdwpTransport_OnLoad\n\
" >> $@

clean::
	-$(RM) $(DEF_FILE)

endif

#
# Rules.
#
include $(BUILDDIR)/common/Library.gmk

#
# vpaths for the transport (should not be seen by others)
#
vpath %.c $(SHARE_SRC)/native/com/sun/tools/jdi $(SHARE_SRC)/transport/shmem $(PLATFORM_SRC)/transport/shmem
