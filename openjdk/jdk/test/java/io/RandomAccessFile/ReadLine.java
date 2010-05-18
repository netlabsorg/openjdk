/*
 * Copyright 1997 Sun Microsystems, Inc.  All Rights Reserved.
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

/* @test
   @bug 1238814
   @summary check for correct implementation of RandomAccessFile.readLine
   */

import java.io.*;

public class ReadLine {

    public static void main(String args[]) throws Exception {
        File fn = new File("x.ReadLine");
        RandomAccessFile raf = new RandomAccessFile(fn,"rw");
        String line;
        int ctr = 1;
        String expected;

        raf.writeBytes
            ("ln1\rln2\r\nln3\nln4\rln5\r\nln6\n\rln8\r\rln10\n\nln12\r\r\nln14");
        raf.seek(0);

        while ((line=raf.readLine()) != null) {
            if ((ctr == 7) || (ctr == 9) ||
                (ctr == 11) || (ctr == 13)) {
                expected = "";
            } else {
                expected = "ln" + ctr;
            }
            if (!line.equals(expected)) {
                throw new Exception("Expected \"" + expected + "\"" +
                                    ", read \"" + line + "\"");
            }
            ctr++;
        }
        System.err.println("Successfully completed test!");
    }

}
