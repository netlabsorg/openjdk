#
# Copyright 2003-2009 Sun Microsystems, Inc.  All Rights Reserved.
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

# This makefile is used to build Serviceability Agent code
# and generate JNI header file for native methods.

AGENT_DIR = $(WorkSpace)/agent
checkAndBuildSA::

ifneq ($(BUILD_OS2_SA),1)
# Already warned about this in build.make
else

# This first part is used to build sa-jdi.jar
include $(WorkSpace)/make/os2/makefiles/rules.make
include $(WorkSpace)/make/sa.files

GENERATED = ../generated

# tools.jar is needed by the JDI - SA binding
SA_CLASSPATH = $(BOOT_JAVA_HOME)/lib/tools.jar

# turn wildcards to individual files
AGENT_FILES1 := $(wildcard $(AGENT_FILES1))
AGENT_FILES2 := $(wildcard $(AGENT_FILES2))

SA_CLASSDIR = $(GENERATED)/saclasses

SA_BUILD_VERSION_PROP = sun.jvm.hotspot.runtime.VM.saBuildVersion=$(SA_BUILD_VERSION)

SA_PROPERTIES = $(SA_CLASSDIR)/sa.properties

default::  $(GENERATED)/sa-jdi.jar

# Remove the space between $(SA_BUILD_VERSION_PROP) and > below as it adds a white space
# at the end of SA version string and causes a version mismatch with the target VM version.

$(GENERATED)\sa-jdi.jar: $(AGENT_FILES1) $(AGENT_FILES2)
	@if [ ! -d $(SA_CLASSDIR) ]; then mkdir -p $(SA_CLASSDIR); fi
	@echo ...Building sa-jdi.jar
	@echo ...$(COMPILE_JAVAC) -source 1.4 -target 1.4 -classpath $(SA_CLASSPATH) -d $(SA_CLASSDIR) ....
	@$(COMPILE_JAVAC) -source 1.4 -target 1.4 -classpath $(SA_CLASSPATH) -sourcepath $(AGENT_SRC_DIR) -d $(SA_CLASSDIR) $(AGENT_FILES1:/=\)
	@$(COMPILE_JAVAC) -source 1.4 -target 1.4 -classpath $(SA_CLASSPATH) -sourcepath $(AGENT_SRC_DIR) -d $(SA_CLASSDIR) $(AGENT_FILES2:/=\)
	$(COMPILE_RMIC) -classpath $(SA_CLASSDIR) -d $(SA_CLASSDIR) sun.jvm.hotspot.debugger.remote.RemoteDebuggerServer
	$(QUIETLY) echo $(SA_BUILD_VERSION_PROP)> $(SA_PROPERTIES)
	$(QUIETLY) rm -f $(SA_CLASSDIR)/sun/jvm/hotspot/utilities/soql/sa.js
	$(QUIETLY) cp $(AGENT_SRC_DIR)/sun/jvm/hotspot/utilities/soql/sa.js $(SA_CLASSDIR)/sun/jvm/hotspot/utilities/soql
	$(QUIETLY) rm -rf $(SA_CLASSDIR)/sun/jvm/hotspot/ui/resources
	$(QUIETLY) mkdir $(SA_CLASSDIR)\sun\jvm\hotspot\ui\resources
	$(QUIETLY) cp $(AGENT_SRC_DIR)/sun/jvm/hotspot/ui/resources/*.png $(SA_CLASSDIR)/sun/jvm/hotspot/ui/resources
	$(QUIETLY) cp -r $(AGENT_SRC_DIR)/images/* $(SA_CLASSDIR)
	$(RUN_JAR) cf $@ -C saclasses .
	$(RUN_JAR) uf $@ -C $(AGENT_SRC_DIR:/=\) META-INF\services\com.sun.jdi.connect.Connector
	$(RUN_JAVAH) -classpath $(SA_CLASSDIR) -jni sun.jvm.hotspot.debugger.windbg.WindbgDebuggerLocal
	$(RUN_JAVAH) -classpath $(SA_CLASSDIR) -jni sun.jvm.hotspot.debugger.x86.X86ThreadContext
	$(RUN_JAVAH) -classpath $(SA_CLASSDIR) -jni sun.jvm.hotspot.debugger.ia64.IA64ThreadContext
	$(RUN_JAVAH) -classpath $(SA_CLASSDIR) -jni sun.jvm.hotspot.debugger.amd64.AMD64ThreadContext



# This second part is used to build saos2dbg.dll
# We currently build it the same way for product, debug, and fastdebug.

SAOSDBG=saos2dbg.dll

checkAndBuildSA:: $(SAOS2DBG)

# @todo look at windows/makefiles/sa.make for details on how to build the SA
# debug DLL


cleanall :
	rm -rf $(GENERATED:\=/)/saclasses
	rm -rf $(GENERATED:\=/)/sa-jdi.jar

endif # ifneq ($(BUILD_OS2_SA),1)
