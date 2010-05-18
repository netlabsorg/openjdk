#
# Copyright 2005 Sun Microsystems, Inc.  All Rights Reserved.
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
# @bug 6173575
# @summary Unit tests for appendToBootstrapClassLoaderSearch and
#   appendToSystemClasLoaderSearch methods.
#
# @build ClassUnloadTest
# @run shell ClassUnloadTest.sh

if [ "${TESTSRC}" = "" ]
then
  echo "TESTSRC not set.  Test cannot execute.  Failed."
  exit 1
fi
                                                                                                        
. ${TESTSRC}/CommonSetup.sh

# Create Foo and Bar
# Foo has a reference to Bar but we deleted Bar so that 
# a NoClassDefFoundError will be thrown when Foo tries to
# resolve the reference to Bar

OTHERDIR="${TESTCLASSES}"/other
mkdir "${OTHERDIR}"

FOO="${OTHERDIR}"/Foo.java
BAR="${OTHERDIR}"/Bar.java
rm -f "${FOO}" "${BAR}"

cat << EOF > "${FOO}"
  public class Foo {
      public static boolean doSomething() {
          try {
	      Bar b = new Bar();
	      return true;
	  } catch (NoClassDefFoundError x) {
	      return false;
	  }
      }
  }
EOF

echo "public class Bar { }" > "${BAR}"

(cd "${OTHERDIR}"; \
  $JAVAC Foo.java Bar.java; $JAR cf "${OTHERDIR}"/Bar.jar Bar.class; \
  rm -f Bar.class)

# Create the manifest
MANIFEST="${TESTCLASSES}"/agent.mf
rm -f "${MANIFEST}"
echo "Premain-Class: ClassUnloadTest" > "${MANIFEST}"

# Setup test case as an agent
$JAR -cfm "${TESTCLASSES}"/ClassUnloadTest.jar "${MANIFEST}" \
  -C "${TESTCLASSES}" ClassUnloadTest.class

# Finally we run the test
(cd "${TESTCLASSES}"; \
  $JAVA -Xverify:none -XX:+TraceClassUnloading -javaagent:ClassUnloadTest.jar \
    ClassUnloadTest "${OTHERDIR}" Bar.jar)
