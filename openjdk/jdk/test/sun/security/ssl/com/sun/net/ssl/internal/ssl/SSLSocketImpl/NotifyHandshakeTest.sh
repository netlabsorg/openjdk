#
# Copyright 2002-2003 Sun Microsystems, Inc.  All Rights Reserved.
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
# @bug 4673442
# @summary remove doPrivileged when creatingting the NotifyHandshakeThread
# @run shell NotifyHandshakeTest.sh
# @author Brad Wetmore
#
# To run independently:  sh NotifyHandshakeTest.sh

if [ "${TESTJAVA}" = "" ]
then
	echo "TESTJAVA not set.  Test cannot execute.  Failed."
	exit 1
fi

if [ "${TESTSRC}" = "" ]
then
	TESTSRC="."
fi

OS=`uname -s`
case "$OS" in
    SunOS | Linux )
	PATHSEP=":"
	;;

    Windows* )
	PATHSEP=";"
	;;
esac

set -ex

#
# Compile the tests, package into their respective jars
#
${TESTJAVA}/bin/javac -d . \
    ${TESTSRC}/NotifyHandshakeTest.java \
    ${TESTSRC}/NotifyHandshakeTestHeyYou.java
${TESTJAVA}/bin/jar -cvf com.jar com/NotifyHandshakeTest*.class
${TESTJAVA}/bin/jar -cvf edu.jar edu/NotifyHandshakeTestHeyYou.class

#
# Don't want the original class files to be used, because
# we want the jar files with the associated contexts to
# be used.
#
rm -rf com edu

#
# This is the only thing we really care about as far as
# test status goes.
#
${TESTJAVA}/bin/java \
    -Dtest.src=${TESTSRC} \
    -classpath "com.jar${PATHSEP}edu.jar" \
    -Djava.security.manager \
    -Djava.security.policy=${TESTSRC}/NotifyHandshakeTest.policy \
    com.NotifyHandshakeTest
retval=$?

rm com.jar edu.jar

exit $retval
