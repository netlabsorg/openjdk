#
# Copyright 2005-2008 Sun Microsystems, Inc.  All Rights Reserved.
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

include ../local.make
include $(WorkSpace)/make/os2/makefiles/projectcreator.make
include local.make

# Pick up rules for building JVMTI (JSR-163)
JvmtiOutDir=jvmtifiles
include $(WorkSpace)/make/os2/makefiles/jvmti.make

# Pick up rules for building SA
include $(WorkSpace)/make/os2/makefiles/sa.make

AdlcOutDir=adfiles

ifeq ($(filter-out compiler2 tiered,$(Variant)),)
default:: $(AdlcOutDir)/ad_$(Platform_arch_model).cpp $(AdlcOutDir)/dfa_$(Platform_arch_model).cpp $(JvmtiGeneratedFiles) buildobjfiles
else
default:: $(JvmtiGeneratedFiles) buildobjfiles
endif

buildobjfiles:
	@ sh $(WorkSpace)/make/windows/create_obj_files.sh $(Variant) $(Platform_arch) $(Platform_arch_model) $(WorkSpace) .	> objfiles.make

classes/ProjectCreator.class: $(ProjectCreatorSources)
	if [ -d classes ]; then rm -rf classes; fi
	mkdir -p classes
	$(COMPILE_JAVAC) -classpath $(WorkSpace)/src/share/tools/ProjectCreator -d classes $(ProjectCreatorSources)

ifeq ($(filter-out compiler2 tiered,$(Variant)),)

include $(WorkSpace)/make/os2/makefiles/compile.make
include $(WorkSpace)/make/os2/makefiles/adlc.make

endif

include $(WorkSpace)/make/os2/makefiles/shared.make
