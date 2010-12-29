#
# Copyright 1997-2008 Sun Microsystems, Inc.  All Rights Reserved.
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

HS_INTERNAL_NAME = jvm
HS_FNAME = $(HS_INTERNAL_NAME).dll
AOUT = $(HS_FNAME)
SAWINDBG = sawindbg.dll
GENERATED = ../generated

default:: _build_pch_file.obj $(AOUT) checkAndBuildSA

include ../local.make
include $(WorkSpace)/make/os2/makefiles/compile.make

CXX_FLAGS += $(DEBUG_OPT_OPTION)
LINK_FLAGS += $(DEBUG_LINK_FLAGS)

include $(WorkSpace)/make/os2/makefiles/vm.make
include local.make

include $(GENERATED)/Dependencies

# _Copy functions, assembled from .s file
# Not included in includeDB because it has no dependencies
Obj_Files += os2_x86_32.obj

# Various OS/2 specific sources
# Not included in includeDB because they don't match the logic of that file
# (it expects only one platform at a time which is windows for now)
Obj_Files += os_os2_init.obj os_os2.obj

HS_BUILD_ID = $(HS_BUILD_VER)-debug

$(AOUT): $(Res_Files) $(Obj_Files) $(Def_File)
	$(LINK) $(LINK_FLAGS) -o $@ $(Def_File) $(Obj_Files) $(Res_Files)
	$(IMPLIB) -o $(basename $@).lib $@

include $(WorkSpace)/make/os2/makefiles/shared.make
include $(WorkSpace)/make/os2/makefiles/sa.make
