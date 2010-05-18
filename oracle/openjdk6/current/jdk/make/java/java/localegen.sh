#!/bin/sh

#
# Copyright 2005 Sun Microsystems, Inc.  All Rights Reserved.
# DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
#
# This code is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 2 only, as
# published by the Free Software Foundation.  Sun designates this
# particular file as subject to the "Classpath" exception as provided
# by Sun in the LICENSE file that accompanied this code.
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
# This script is to generate the supported locale list string and replace the
# LocaleDataMetaInfo-XLocales.java in <ws>/src/share/classes/sun/util
# 
# NAWK & SED is passed in as environment variables.
#

# A list of resource base name list;
RESOURCE_NAMES=$1

# A list of European resources;
EURO_FILES_LIST=$2

# A list of non-European resources;
NONEURO_FILES_LIST=$3

INPUT_FILE=$4
OUTPUT_FILE=$5

localelist=
getlocalelist() {
    localelist=""
    localelist=`$NAWK -F$1_ '{print $2}' $2 | sort`
}

sed_script="$SED -e \"s@^#warn .*@// -- This file was mechanically generated: Do not edit! -- //@\" " 

for FILE in $RESOURCE_NAMES 
do
    getlocalelist $FILE $EURO_FILES_LIST
    sed_script=$sed_script"-e \"s/#"$FILE"_EuroLocales#/$localelist/g\" "
    getlocalelist $FILE $NONEURO_FILES_LIST
    sed_script=$sed_script"-e \"s/#"$FILE"_NonEuroLocales#/$localelist/g\" "
done

sed_script=$sed_script"$INPUT_FILE > $OUTPUT_FILE"
eval $sed_script


