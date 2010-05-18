#! /bin/sh

#
# Copyright 2003 Sun Microsystems, Inc.  All Rights Reserved.
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

SPP='sh ../../../../make/java/nio/spp.sh'

gen() {
#  if [ $3 = "true" ]
#  then $SPP -K$1 -Dtype=$1 -DType=$2 -Kprim<Basic-X.java >Basic$2.java
#  else $SPP -K$1 -Dtype=$1 -DType=$2 -K$3 <Basic-X.java >Basic$2.java
#  fi
 $SPP -K$1 -Dtype=$1 -DType=$2 -K$3 -K$4 -K$5 -K$6 <Basic-X.java >Basic$2.java
}

gen boolean Boolean       prim  ""  ""   ""
gen Boolean BooleanObject ""    ""  ""   ""
gen byte Byte             prim  ""  dec  ""
gen Byte ByteObject       ""    ""  dec  ""
gen char Char             prim  ""  ""   ""
gen Character CharObject  ""    ""  ""   ""
gen short Short           prim  ""  dec  ""
gen Short ShortObject     ""    ""  dec  ""
gen int Int               prim  ""  dec  ""
gen Integer IntObject     ""    ""  dec  ""
gen long Long             prim  ""  dec  ""
gen Long LongObject       ""    ""  dec  ""
gen BigInteger BigInteger ""    ""  ""   ""

gen float Float           prim  fp  ""   ""
gen Float FloatObject     ""    fp  ""   ""
gen double Double         prim  fp  ""   ""
gen Double DoubleObject   ""    fp  ""   ""
gen BigDecimal BigDecimal ""    fp  ""   ""

gen Calendar DateTime     ""    ""  ""   datetime
