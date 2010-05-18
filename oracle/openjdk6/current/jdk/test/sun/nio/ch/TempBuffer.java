/*
 * Copyright 2002-2003 Sun Microsystems, Inc.  All Rights Reserved.
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
 * @bug 4738257 4853295
 * @summary Test Util.getTemporaryBuffer
 */

import java.io.*;
import java.nio.channels.*;
import java.nio.*;

public class TempBuffer {

    private static final int SIZE = 4000;

    private static final int ITERATIONS = 10;

    public static void main(String[] args) throws Exception {
        for (int i=0; i<ITERATIONS; i++)
            testTempBufs();
    }

    private static void testTempBufs() throws Exception {
        Pipe pipe = Pipe.open();
        Pipe.SourceChannel sourceChannel = pipe.source();
        final Pipe.SinkChannel sinkChannel = pipe.sink();

        Thread writerThread = new Thread() {
            public void run() {
                try {
                    OutputStream out = Channels.newOutputStream(sinkChannel);
                    File blah = File.createTempFile("blah1", null);
                    blah.deleteOnExit();
                    TempBuffer.initTestFile(blah);
                    RandomAccessFile raf = new RandomAccessFile(blah, "rw");
                    FileChannel fs = raf.getChannel();
                    fs.transferTo(0, SIZE, Channels.newChannel(out));
                    out.flush();
                } catch (IOException ioe) {
                    throw new RuntimeException(ioe);
                }
            }
        };

        writerThread.start();

        InputStream in = Channels.newInputStream(sourceChannel);
        File blah = File.createTempFile("blah2", null);
        blah.deleteOnExit();
        RandomAccessFile raf = new RandomAccessFile(blah, "rw");
        FileChannel fs = raf.getChannel();
        raf.setLength(SIZE);
        fs.transferFrom(Channels.newChannel(in), 0, SIZE);
        fs.close();
    }

    private static void initTestFile(File blah) throws IOException {
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
