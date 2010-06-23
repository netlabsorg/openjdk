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
include $(WorkSpace)/make/os2/makefiles/makedeps.make
include local.make

# Pick up rules for building JVMTI (JSR-163)
JvmtiOutDir=jvmtifiles
include $(WorkSpace)/make/os2/makefiles/jvmti.make

# Pick up rules for building SA
include $(WorkSpace)/make/os2/makefiles/sa.make

ifeq ($(filter-out compiler2 tiered,$(Variant)),)
default:: includeDB.current Dependencies incls/ad_$(Platform_arch_model).cpp incls/dfa_$(Platform_arch_model).cpp $(JvmtiGeneratedFiles)
else
default:: includeDB.current Dependencies $(JvmtiGeneratedFiles)
endif

# core plus serial gc
IncludeDBs_base=$(WorkSpace)/src/share/vm/includeDB_core \
           $(WorkSpace)/src/share/vm/includeDB_jvmti \
           $(WorkSpace)/src/share/vm/includeDB_gc \
           $(WorkSpace)/src/share/vm/gc_implementation/includeDB_gc_serial

# parallel gc
IncludeDBs_gc= $(WorkSpace)/src/share/vm/includeDB_gc_parallel \
           $(WorkSpace)/src/share/vm/gc_implementation/includeDB_gc_parallelScavenge \
           $(WorkSpace)/src/share/vm/gc_implementation/includeDB_gc_shared \
           $(WorkSpace)/src/share/vm/gc_implementation/includeDB_gc_parNew \
           $(WorkSpace)/src/share/vm/gc_implementation/includeDB_gc_concurrentMarkSweep \
           $(WorkSpace)/src/share/vm/gc_implementation/includeDB_gc_g1

IncludeDBs_core=$(IncludeDBs_base) $(IncludeDBs_gc) \
                $(WorkSpace)/src/share/vm/includeDB_features

ifeq ($(Variant),core)
IncludeDBs=$(IncludeDBs_core)
endif

ifeq ($(Variant),kernel)
IncludeDBs=$(IncludeDBs_base) $(WorkSpace)/src/share/vm/includeDB_compiler1
endif

ifeq ($(Variant),compiler1)
IncludeDBs=$(IncludeDBs_core) $(WorkSpace)/src/share/vm/includeDB_compiler1
endif


ifeq ($(Variant),compiler2)
IncludeDBs=$(IncludeDBs_core) $(WorkSpace)/src/share/vm/includeDB_compiler2
endif

ifeq ($(Variant),tiered)
IncludeDBs=$(IncludeDBs_core) $(WorkSpace)/src/share/vm/includeDB_compiler1 \
           $(WorkSpace)/src/share/vm/includeDB_compiler2
endif

includeDB.current Dependencies: classes/MakeDeps.class $(IncludeDBs)
	cat $(IncludeDBs) > includeDB
	if [ -d incls ]; then rmdir -fr incls; fi
	mkdir -p incls
	$(RUN_JAVA) -Djava.class.path=classes MakeDeps WinGammaPlatform$(VcVersion) $(WorkSpace)/make/windows/platform_$(BUILDARCH) includeDB $(MakeDepsOptions)
	rm -f includeDB.current
	cp includeDB includeDB.current

classes/MakeDeps.class: $(MakeDepsSources)
	if [ -d classes ]; then rmdir -fr classes; fi
	mkdir -p classes
	$(COMPILE_JAVAC) -classpath $(WorkSpace)/src/share/tools/MakeDeps -d classes $(MakeDepsSources)

ifeq ($(filter-out compiler2 tiered,$(Variant)),)

include $(WorkSpace)/make/os2/makefiles/adlc.make

endif

include $(WorkSpace)/make/os2/makefiles/shared.make
