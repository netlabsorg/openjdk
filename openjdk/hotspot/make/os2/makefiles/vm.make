#
# Copyright 1997-2009 Sun Microsystems, Inc.  All Rights Reserved.
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

ifdef RELEASE
ifdef DEVELOP
CPP_FLAGS += -DDEBUG
else
CPP_FLAGS += -DPRODUCT
endif
else
CPP_FLAGS += -DASSERT
endif

ifeq ($(Variant), core)
# No need to define anything, CORE is defined as !COMPILER1 && !COMPILER2
endif

ifeq ($(Variant), kernel)
CPP_FLAGS += -DKERNEL
endif

ifeq ($(Variant), compiler1)
CPP_FLAGS += -DCOMPILER1
endif

ifeq ($(Variant), compiler2)
CPP_FLAGS += -DCOMPILER2
endif

ifeq ($(Variant), tiered)
CPP_FLAGS += -DCOMPILER1 -DCOMPILER2
endif

ifeq ($(BUILDARCH), i486)
HOTSPOT_LIB_ARCH = i386
else
HOTSPOT_LIB_ARCH = $(BUILDARCH)
endif

# The following variables are defined in the generated local.make file.
CPP_FLAGS += -D'HOTSPOT_RELEASE_VERSION="$(HS_BUILD_VER)"'
CPP_FLAGS += -D'JRE_RELEASE_VERSION="$(JRE_RELEASE_VER)"'
CPP_FLAGS += -D'HOTSPOT_LIB_ARCH="$(HOTSPOT_LIB_ARCH)"'
CPP_FLAGS += -D'HOTSPOT_BUILD_TARGET="$(BUILD_FLAVOR)"'
CPP_FLAGS += -D'HOTSPOT_BUILD_USER="$(BuildUser)"'
CPP_FLAGS += -D'HOTSPOT_VM_DISTRO="$(HOTSPOT_VM_DISTRO)"'

CPP_FLAGS += -DOS2

# Must specify this for sharedRuntimeTrig.cpp
CPP_FLAGS += -DVM_LITTLE_ENDIAN

# Define that so jni.h is on correct side
CPP_FLAGS += -D_JNI_IMPLEMENTATION_

ifeq ($(Variant), kernel)
AGCT_EXPORT=
else
AGCT_EXPORT=AsyncGetCallTrace
endif

MAKEFILE = $(word $(words $(MAKEFILE_LIST)),$(MAKEFILE_LIST))

Def_File = vm.def
$(Def_File): $(MAKEFILE)
	@echo -e 'EXPORTS \n \
  JNI_GetDefaultJavaVMInitArgs = "_JNI_GetDefaultJavaVMInitArgs@4" \n \
  JNI_CreateJavaVM = "_JNI_CreateJavaVM@12" \n \
  JNI_GetCreatedJavaVMs = "_JNI_GetCreatedJavaVMs@12" \n \
  _jio_snprintf \n \
  _jio_printf \n \
  _jio_fprintf \n \
  _jio_vfprintf \n \
  _jio_vsnprintf \n \
  _$(AGCT_EXPORT) \n \
  $(AGCT_EXPORT) = _$(AGCT_EXPORT) \n \
  JVM_GetVersionInfo = "_JVM_GetVersionInfo@12" \n \
  JVM_GetThreadStateNames = "_JVM_GetThreadStateNames@12" \n \
  JVM_GetThreadStateValues = "_JVM_GetThreadStateValues@8" \n \
  JVM_InitAgentProperties = "_JVM_InitAgentProperties@8" \n \
  JVM_FindClassFromBootLoader = "_JVM_FindClassFromBootLoader@8" \n \
' > $(Def_File)

LINK_FLAGS += -Zdll

Src_Dirs = \
  ../generated                          \
  ../generated/incls                    \
  ../generated/jvmtifiles               \
  $(WorkSpace)/src/share/vm/c1          \
  $(WorkSpace)/src/share/vm/compiler    \
  $(WorkSpace)/src/share/vm/code        \
  $(WorkSpace)/src/share/vm/interpreter \
  $(WorkSpace)/src/share/vm/ci          \
  $(WorkSpace)/src/share/vm/classfile   \
  $(WorkSpace)/src/share/vm/gc_implementation/parallelScavenge\
  $(WorkSpace)/src/share/vm/gc_implementation/shared\
  $(WorkSpace)/src/share/vm/gc_implementation/parNew\
  $(WorkSpace)/src/share/vm/gc_implementation/concurrentMarkSweep\
  $(WorkSpace)/src/share/vm/gc_implementation/g1\
  $(WorkSpace)/src/share/vm/gc_interface\
  $(WorkSpace)/src/share/vm/asm         \
  $(WorkSpace)/src/share/vm/memory      \
  $(WorkSpace)/src/share/vm/oops        \
  $(WorkSpace)/src/share/vm/prims       \
  $(WorkSpace)/src/share/vm/runtime     \
  $(WorkSpace)/src/share/vm/services    \
  $(WorkSpace)/src/share/vm/utilities   \
  $(WorkSpace)/src/share/vm/libadt      \
  $(WorkSpace)/src/share/vm/opto        \
  $(WorkSpace)/src/os/windows/vm        \
  $(WorkSpace)/src/os/os2/vm            \
  $(WorkSpace)/src/os_cpu/windows_$(Platform_arch)/vm \
  $(WorkSpace)/src/os_cpu/os2_$(Platform_arch)/vm \
  $(WorkSpace)/src/cpu/$(Platform_arch)/vm \
  $(WorkSpace)/src/share/vm/opto

# @todo PCH once GCC 4 for OS/2 supports it well
CPP_FLAGS += # PCH output: "vm.pch" PCH sources: "incls/_precompiled.incl"

# Where to find the include files for the virtual machine
CPP_FLAGS += $(Src_Dirs:%=-I'%')

# Where to find the source code for the virtual machine
VPATH += $(Src_Dirs:%=%;)

# Special case files not using precompiled header files.

c1_RInfo_$(Platform_arch).obj: $(WorkSpace)/src/cpu/$(Platform_arch)/vm/c1_RInfo_$(Platform_arch).cpp
os_windows.obj: $(WorkSpace)/src/os/windows/vm/os_windows.cpp
os_windows_$(Platform_arch).obj: $(WorkSpace)/src/os_cpu/windows_$(Platform_arch)/vm/os_windows_$(Platform_arch).cpp
osThread_windows.obj: $(WorkSpace)/src/os/windows/vm/osThread_windows.cpp
conditionVar_windows.obj: $(WorkSpace)/src/os/windows/vm/conditionVar_windows.cpp
getThread_windows_$(Platform_arch).obj: $(WorkSpace)/src/os_cpu/windows_$(Platform_arch)/vm/getThread_windows_$(Platform_arch).cpp
opcodes.obj: $(WorkSpace)/src/share/vm/opto/opcodes.cpp
bytecodeInterpreter.obj: $(WorkSpace)/src/share/vm/interpreter/bytecodeInterpreter.cpp
bytecodeInterpreterWithChecks.obj: ../generated/jvmtifiles/bytecodeInterpreterWithChecks.cpp

# Default rules for the Virtual Machine
%.obj: %.cpp
	$(CXX) $(CXX_FLAGS) -c $< -o $@

%.obj: %.s
	$(CXX) $(CXX_FLAGS) -c $< -o $@

default::

# @todo PCH once GCC 4 for OS/2 supports it well
_build_pch_file.obj:
	@#echo '#include "incls/_precompiled.incl"' > ../generated/_build_pch_file.cpp
	@#$(CXX) $(CXX_FLAGS) PCH output: "vm.pch", PCH sources (create mode): "incls/_precompiled.incl" -c ../generated/_build_pch_file.cpp -o $@
