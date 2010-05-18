#!/bin/sh

#
# Copyright 2005-2006 Sun Microsystems, Inc.  All Rights Reserved.
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
# @bug 6173612 6273707 6277253 6335921 6348630 6342019 6381757
# @summary Basic unit tests for the VM attach mechanism.
#
# @build BasicTests
# @run shell BasicTests.sh

if [ "${TESTSRC}" = "" ]
then
  echo "TESTSRC not set.  Test cannot execute.  Failed."
  exit 1
fi

. ${TESTSRC}/CommonSetup.sh
. ${TESTSRC}/ApplicationSetup.sh
. ${TESTSRC}/AgentSetup.sh

startApplication -Dattach.test=true
# pid = process-id, port = shutdown port
                                                                                                      
failures=0

echo "Running tests ..."

$JAVA -classpath ${TESTCLASSES}${PS}${TESTJAVA}/lib/tools.jar \
  BasicTests $pid $agent $badagent $redefineagent 2>&1
if [ $? != 0 ]; then failures=`expr $failures + 1`; fi

stopApplication $port

if [ $failures = 0 ]; 
  then echo "All tests passed.";
  else echo "$failures test(s) failed:"; cat ${OUTPUTFILE};
fi
exit $failures
