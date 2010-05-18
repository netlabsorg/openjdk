#
# Copyright 2006 Sun Microsystems, Inc.  All Rights Reserved.
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
 # @bug 6502503
 # @run shell/timeout=140 ChunkedCharEncoding.sh
 # @summary Http URL connection don't work when default encoding is Cp037: HTTP Transfer-Encoding:chunked

OS=`uname -s`
case "$OS" in
  SunOS | Linux )
    PS=":"
    FS="/"
    ;;
  Windows* )
    PS=";"
    FS="\\"
    ;;
  * )
    echo "Unrecognized system!"
    exit 1;
    ;;
esac

# compile
${TESTJAVA}${FS}bin${FS}javac -d . ${TESTSRC}${FS}TestAvailable.java

# run with CP037 encoding specified. 
${TESTJAVA}${FS}bin${FS}java -Dfile.encoding=Cp037 TestAvailable 2>&1 

result=$?
if [ "$result" -ne "0" ]; then
    exit 1
fi

# no failures, exit.
exit 0

