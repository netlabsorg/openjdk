#
# Copyright 2004-2005 Sun Microsystems, Inc.  All Rights Reserved.
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

# @test
# @bug 5055293
# @summary Test non US-ASCII characters in the value of the Boot-Class-Path 
#          attribute.

if [ "${TESTJAVA}" = "" ]
then
  echo "TESTJAVA not set.  Test cannot execute.  Failed."
  exit 1
fi

if [ "${TESTSRC}" = "" ]
then
  echo "TESTSRC not set.  Test cannot execute.  Failed."
  exit 1
fi

if [ "${TESTCLASSES}" = "" ]
then
  echo "TESTCLASSES not set.  Test cannot execute.  Failed."
  exit 1
fi

JAVAC="${TESTJAVA}"/bin/javac
JAVA="${TESTJAVA}"/bin/java
JAR="${TESTJAVA}"/bin/jar

echo "Creating manifest file..."

"$JAVAC" -d "${TESTCLASSES}" "${TESTSRC}"/Setup.java

# java Setup <workdir> <premain-class>
# - outputs boot class path to boot.dir

"$JAVA" -classpath "${TESTCLASSES}" Setup "${TESTCLASSES}" Agent
BOOTDIR=`cat ${TESTCLASSES}/boot.dir`

echo "Created ${BOOTDIR}"

echo "Building test classes..."

"$JAVAC" -d "${TESTCLASSES}" "${TESTSRC}"/Agent.java "${TESTSRC}"/DummyMain.java
"$JAVAC" -d "${BOOTDIR}" "${TESTSRC}"/AgentSupport.java

echo "Creating agent jar file..."

"$JAR" -cvfm "${TESTCLASSES}"/Agent.jar "${TESTCLASSES}"/MANIFEST.MF \
    -C "${TESTCLASSES}" Agent.class || exit 1

echo "Running test..."

"${JAVA}" -javaagent:"${TESTCLASSES}"/Agent.jar -classpath "${TESTCLASSES}" DummyMain
result=$?

echo "Cleanup..."

"$JAVAC" -d "${TESTCLASSES}" "${TESTSRC}"/Cleanup.java
"$JAVA" -classpath "${TESTCLASSES}" Cleanup "${BOOTDIR}"

exit $result
