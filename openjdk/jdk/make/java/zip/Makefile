#
# Copyright (c) 1996, 2006, Oracle and/or its affiliates. All rights reserved.
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
PACKAGE = java.util.zip
LIBRARY = zip
PRODUCT = sun

ifeq ($(PLATFORM), os2)
LIBRARY_SHORT = jzip
endif

include $(BUILDDIR)/common/Defs.gmk

ZLIB_VERSION = 1.1.3

#
# Files to compile.
#
include FILES_c.gmk
AUTO_FILES_JAVA_DIRS = java/util/zip

FILES_export = \
	java/util/zip/CRC32.java \
	java/util/zip/Adler32.java \
	java/util/zip/Inflater.java \
	java/util/zip/Deflater.java \
	java/util/zip/ZipFile.java \
	java/util/zip/ZipEntry.java \
	java/util/jar/JarFile.java

ifneq ($(filter-out windows os2, $(PLATFORM)),)
OTHER_CFLAGS += -DUSE_MMAP
endif
ifeq ($(PLATFORM), windows)
  # Files built here do not compile with warning level 3 if warnings are fatal
  COMPILER_WARNINGS_FATAL=false
endif

#
# Library to compile.
#
ifeq ($(PLATFORM), solaris)
  ifneq ($(ARCH), amd64)
    FILES_reorder += reorder-$(ARCH)
  endif
endif

ifeq ($(PLATFORM), windows)
OTHER_LCF = -export:ZIP_Open -export:ZIP_Close -export:ZIP_FindEntry \
            -export:ZIP_ReadEntry -export:ZIP_GetNextEntry
else
ifeq ($(PLATFORM), os2)
# exports live in .def file
DEF_FILE = $(OBJDIR)/$(LIBRARY).def
$(DEF_FILE): $(MAKEFILE_LIST)
	@$(ECHO) "LIBRARY $(LIBRARY_NAME) INITINSTANCE TERMINSTANCE" > $@
	@$(ECHO) "EXPORTS\n\
  ZIP_CRC32 = _ZIP_CRC32\n\
  ZIP_Open\n\
  ZIP_Close\n\
  ZIP_FindEntry\n\
  ZIP_ReadEntry\n\
  ZIP_GetNextEntry\n\
" >> $@
clean::
	-$(RM) $(DEF_FILE)
else
CPPFLAGS += -UDEBUG 
endif
endif

include $(BUILDDIR)/common/Mapfile-vers.gmk
include $(BUILDDIR)/common/Library.gmk

CPPFLAGS += -I$(SHARE_SRC)/native/java/util/zip/zlib-$(ZLIB_VERSION)
CPPFLAGS += -I$(SHARE_SRC)/native/java/io
CPPFLAGS += -I$(PLATFORM_SRC)/native/java/io

#
# Add to ambient vpath so we pick up the library files
#
vpath %.c $(SHARE_SRC)/native/$(PKGDIR)/zlib-$(ZLIB_VERSION)

#
# Link to JVM library for JVM_Zip* functions
#
OTHER_LDLIBS = $(JVMLIB)
