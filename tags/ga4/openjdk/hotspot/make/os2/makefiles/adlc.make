#
# Copyright 1999-2009 Sun Microsystems, Inc.  All Rights Reserved.
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

# adlc is an internal tool, always generate the debug version with assert()
CPP_FLAGS += -DASSERT
C_FLAGS += $(DEBUG_OPT_OPTION)
LINK_FLAGS += $(DEBUG_LINK_FLAGS)

# Rules for building adlc.exe

ifdef ALT_ADLC_PATH
ADLC=$(ALT_ADLC_PATH)/adlc.exe
else
ADLC=adlc
endif

CPP_INCLUDE_DIRS=\
  -I'../generated' \
  -I'$(WorkSpace)/src/share/vm/' \
  -I'$(WorkSpace)/src/os/windows/vm' \
  -I'$(WorkSpace)/src/cpu/$(Platform_arch)/vm'

# NOTE! If you add any files here, you must also update GENERATED_NAMES_IN_DIR
# and ProjectCreatorIDEOptions in projectcreator.make.
GENERATED_NAMES=\
  ad_$(Platform_arch_model).cpp \
  ad_$(Platform_arch_model).hpp \
  ad_$(Platform_arch_model)_clone.cpp \
  ad_$(Platform_arch_model)_expand.cpp \
  ad_$(Platform_arch_model)_format.cpp \
  ad_$(Platform_arch_model)_gen.cpp \
  ad_$(Platform_arch_model)_misc.cpp \
  ad_$(Platform_arch_model)_peephole.cpp \
  ad_$(Platform_arch_model)_pipeline.cpp \
  adGlobals_$(Platform_arch_model).hpp \
  dfa_$(Platform_arch_model).cpp

# NOTE! This must be kept in sync with GENERATED_NAMES
GENERATED_NAMES_IN_DIR=$(GENERATED_NAMES:%=$(AdlcOutDir)/%)

VPATH += $(WorkSpace)/src/share/vm/adlc;$(WorkSpace)/src/share/vm/opto

%.obj: %.cpp
	$(CXX) $(CPP_INCLUDE_DIRS) $(CXX_FLAGS) -c $< -o $@

adlc.exe: main.obj adlparse.obj archDesc.obj arena.obj dfa.obj dict2.obj filebuff.obj \
          forms.obj formsopt.obj formssel.obj opcodes.obj output_c.obj output_h.obj
	$(LINK) $(LINK_FLAGS) -o $@ $^

.NOTPARALLEL: $(GENERATED_NAMES_IN_DIR)

$(GENERATED_NAMES_IN_DIR): $(Platform_arch_model).ad adlc.exe
	rm -f $(GENERATED_NAMES)
	if [ -d $(AdlcOutDir) ]; then rm -rf $(AdlcOutDir); fi
	mkdir -p $(AdlcOutDir)
	$(ADLC) $(ADLCFLAGS) $(Platform_arch_model).ad
	mv $(GENERATED_NAMES) $(AdlcOutDir)/

$(Platform_arch_model).ad: $(WorkSpace)/src/cpu/$(Platform_arch)/vm/$(Platform_arch_model).ad $(WorkSpace)/src/os_cpu/windows_$(Platform_arch)/vm/windows_$(Platform_arch_model).ad
	rm -f $(Platform_arch_model).ad
	cat $(WorkSpace)/src/cpu/$(Platform_arch)/vm/$(Platform_arch_model).ad  \
	    $(WorkSpace)/src/os_cpu/windows_$(Platform_arch)/vm/windows_$(Platform_arch_model).ad >$(Platform_arch_model).ad
