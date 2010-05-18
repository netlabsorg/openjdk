#!/bin/sh

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


#
#
# Application Setup - creates ${TESTCLASSES}/Application.jar and the following
# procedures:
#	startApplication - starts target application
#	stopApplication $1 - stops application via TCP shutdown port $1

$JAVAC -d "${TESTCLASSES}" "${TESTSRC}"/Application.java "${TESTSRC}"/Shutdown.java
$JAR -cfm "${TESTCLASSES}"/Application.jar "${TESTSRC}"/application.mf \
  -C "${TESTCLASSES}" Application.class

OUTPUTFILE=${TESTCLASSES}/Application.out
rm -f ${OUTPUTFILE}

startApplication() 
{
  ${JAVA} $1 $2 $3 -jar "${TESTCLASSES}"/Application.jar > ${OUTPUTFILE} &
  pid="$!"

  # MKS creates an intermediate shell to launch ${JAVA} so
  # ${pid} is not the actual pid. We have put in a small sleep
  # to give the intermediate shell process time to launch the
  # "java" process.
  if [ "$OS" = "Windows" ]; then
    sleep 2
    realpid=`ps -o pid,ppid,comm|grep ${pid}|grep "java"|cut -c1-6`
    pid=${realpid}
  fi
                                                                                                                  
  echo "Waiting for Application to initialize..."
  attempts=0
  while true; do
    sleep 1
    port=`tail -1 ${OUTPUTFILE}`
    if [ ! -z "$port" ]; then
      # In case of errors wait time for output to be flushed
      sleep 1
      cat ${OUTPUTFILE}
      break
    fi
    attempts=`expr $attempts + 1`
    echo "Waiting $attempts second(s) ..."
  done
  echo "Application is process $pid, shutdown port is $port"
  return $port
}

stopApplication() 
{
  $JAVA -classpath "${TESTCLASSES}" Shutdown $1
}

