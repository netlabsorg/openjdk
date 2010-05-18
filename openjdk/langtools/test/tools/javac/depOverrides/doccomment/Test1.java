/*
 * Copyright 2005-2006 Sun Microsystems, Inc.  All Rights Reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * This code is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 only, as
 * published by the Free Software Foundation.
 *
 * This code is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * version 2 for more details (a copy is included in the LICENSE file that
 * accompanied this code).
 *
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa Clara,
 * CA 95054 USA or visit www.sun.com if you need additional information or
 * have any questions.
 */

/*
 * @test
 * @bug 5086088
 * @summary check warnings generated when overriding deprecated methods
 *
 * @compile/ref=empty    -XDstdout -XDrawDiagnostics -Xlint:deprecation I.java
 * @compile/ref=Test1A.out  -XDstdout -XDrawDiagnostics -Xlint:deprecation A.java
 * @compile/ref=Test1B.out  -XDstdout -XDrawDiagnostics -Xlint:deprecation B.java
 * @compile/ref=Test1B2.out -XDstdout -XDrawDiagnostics -Xlint:deprecation B2.java
 * @compile/ref=empty    -XDstdout -XDrawDiagnostics -Xlint:deprecation B3.java
 * @compile/ref=empty    -XDstdout -XDrawDiagnostics -Xlint:deprecation Test1.java
 */


// class should compile with no deprecation warnings
class Test1 extends B
{
}
