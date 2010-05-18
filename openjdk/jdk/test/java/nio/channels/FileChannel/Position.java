/*
 * Copyright 2000-2001 Sun Microsystems, Inc.  All Rights Reserved.
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
 * @summary Test position method of FileChannel
 */

import java.io.*;
import java.nio.MappedByteBuffer;
import java.nio.channels.*;
import java.nio.channels.FileChannel;
import java.util.Random;


/**
 * Testing FileChannel's position method.
 */

public class Position {

    private static PrintStream err = System.err;

    private static Random generator = new Random();

    private static int CHARS_PER_LINE = File.separatorChar == '/' ? 5 : 6;

    private static File blah;

    public static void main(String[] args) throws Exception {
        blah = File.createTempFile("blah", null);
        blah.deleteOnExit();
        initTestFile(blah);

        FileInputStream fis = new FileInputStream(blah);
        FileChannel c = fis.getChannel();

        for(int i=0; i<100; i++) {
            long newPos = generator.nextInt(1000);
            c.position(newPos);
            if (c.position() != newPos)
                throw new RuntimeException("Position failed");
        }

        c.close();
        fis.close();
    }

    /**
     * Creates file blah:
     * 0000
     * 0001
     * 0002
     * 0003
     * .
     * .
     * .
     * 3999
     *
     */
    private static void initTestFile(File blah) throws Exception {
        FileOutputStream fos = new FileOutputStream(blah);
        BufferedWriter awriter
            = new BufferedWriter(new OutputStreamWriter(fos, "8859_1"));

        for(int i=0; i<4000; i++) {
            String number = new Integer(i).toString();
            for (int h=0; h<4-number.length(); h++)
                awriter.write("0");
            awriter.write(""+i);
            awriter.newLine();
        }
       awriter.flush();
       awriter.close();
    }
}
