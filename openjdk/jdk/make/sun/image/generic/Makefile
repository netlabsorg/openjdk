#
# Copyright (c) 1998, 2005, Oracle and/or its affiliates. All rights reserved.
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
# Makefile for building the generic version of medialib
#

BUILDDIR = ../../..
PACKAGE = sun.awt.medialib
LIBRARY = mlib_image
PRODUCT = sun

ifeq ($(PLATFORM), os2)
LIBRARY_SHORT = jmlibimg
endif

include $(BUILDDIR)/common/Defs.gmk

#
# Use highest level of optimization on this library
#
_OPT = $(CC_HIGHEST_OPT)

#
# Use mapfile
#
FILES_m = mapfile-vers
include $(BUILDDIR)/common/Mapfile-vers.gmk

#
# Files
#
include FILES_c.gmk

ifeq ($(PLATFORM), os2)

# exports live in .def file
DEF_FILE = $(OBJDIR)/$(LIBRARY).def
$(DEF_FILE): $(MAKEFILE_LIST)
	@$(ECHO) "LIBRARY $(LIBRARY_NAME) INITINSTANCE TERMINSTANCE" > $@
	@$(ECHO) "EXPORTS\n\
  j2d_mlib_ImageConvMxN = _j2d_mlib_ImageConvMxN\n\
  j2d_mlib_ImageConvKernelConvert = _j2d_mlib_ImageConvKernelConvert\n\
  j2d_mlib_ImageCreateStruct = _j2d_mlib_ImageCreateStruct\n\
  j2d_mlib_ImageCreate = _j2d_mlib_ImageCreate\n\
  j2d_mlib_ImageDelete = _j2d_mlib_ImageDelete\n\
  j2d_mlib_ImageAffine = _j2d_mlib_ImageAffine\n\
  j2d_mlib_ImageLookUp = _j2d_mlib_ImageLookUp\n\
" >> $@

clean::
	-$(RM) $(DEF_FILE)

endif

#
# Rules
#
include $(BUILDDIR)/common/Library.gmk

#
# set compile options for solaris 32- or 64-bit build
#
ifeq ($(ARCH_DATA_MODEL), 64)
  CFLAGS += -DMLIB_OS64BIT
endif # ARCH_DATA_MODEL

ifneq ($(PLATFORM), sparc)
      OTHER_CFLAGS += -DMLIB_NO_LIBSUNMATH
endif

CPPFLAGS += \
            -I$(PLATFORM_SRC)/native/$(PKGDIR)/medialib 

OTHER_CFLAGS += -D__USE_J2D_NAMES -D__MEDIALIB_OLD_NAMES
OTHER_LDLIBS = $(LIBM) -ldl
