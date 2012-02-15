#
# Copyright (c) 2010, Oracle and/or its affiliates. All rights reserved.
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
# Please contact Oracle, 500 Oracle Parkway, Redwood Shores, CA 94065 USA
# or visit www.oracle.com if you need additional information or have any
# questions.
#  
#


LAUNCHER_FLAGS = $(CPP_FLAGS) $(ARCHFLAG) \
	-DFULL_VERSION=\"$(HOTSPOT_RELEASE_VERSION)\" \
	-DJDK_MAJOR_VERSION=\"$(JDK_MAJOR_VERSION)\" \
	-DJDK_MINOR_VERSION=\"$(JDK_MINOR_VERSION)\" \
	-DGAMMA \
	-DLAUNCHER_TYPE=\"gamma\" \
	-D_CRT_SECURE_NO_WARNINGS \
	-D_CRT_SECURE_NO_DEPRECATE \
	-DLINK_INTO_LIBJVM \
	-I$(WorkSpace)\src\os\windows\launcher \
	-I$(WorkSpace)\src\share\tools\launcher \
	-I$(WorkSpace)\src\share\vm\prims \
	-I$(WorkSpace)\src\share\vm \
	-I$(WorkSpace)\src\cpu\$(Platform_arch)\vm \
	-I$(WorkSpace)\src\os\windows\vm

LINK_FLAGS += -l$(HS_INTERNAL_NAME).lib -g -Zlinker /PM:VIO

LAUNCHERDIR = $(WorkSpace)/src/os/windows/launcher
LAUNCHERDIR_SHARE = $(WorkSpace)/src/share/tools/launcher

LAUNCHER_OUT = launcher

SUFFIXES += .d

OBJS := $(LAUNCHER_OUT)/java.obj $(LAUNCHER_OUT)/java_md.obj $(LAUNCHER_OUT)/jli_util.obj

DEPFILES := $(patsubst %.obj,%.d,$(OBJS))
-include $(DEPFILES)

launcher-out:
	mkdir -p $(LAUNCHER_OUT)

$(LAUNCHER_OUT)/%.obj: $(LAUNCHERDIR_SHARE)/%.c | launcher-out
	$(QUIETLY) $(CXX) $(CXXFLAGS) -g -o $@ -c $< -MMD $(LAUNCHER_FLAGS)

$(LAUNCHER_OUT)/%.obj: $(LAUNCHERDIR)/%.c | launcher-out
	$(QUIETLY) $(CXX) $(CXXFLAGS) -g -o $@ -c $< -MMD $(LAUNCHER_FLAGS)

launcher: $(OBJS)
	echo $(JAVA_HOME) > jdkpath.txt  
	$(LINK) $(LINK_FLAGS) -o hotspot.exe $(OBJS)


