/*
 * Copyright 2001 Sun Microsystems, Inc.  All Rights Reserved.
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
 * @bug 4510489
 * @summary Verify IOUtil.java reads to buffer limits
 */
import java.io.*;
import java.nio.*;
import java.nio.channels.*;

public class MapReadOnly {
    public static void main (String args[]) throws Exception {
        File testFile = File.createTempFile("test2", null);
        testFile.deleteOnExit();
        RandomAccessFile raf = new RandomAccessFile(testFile, "rw");
        FileChannel fc = raf.getChannel();
        MappedByteBuffer mbb = fc.map(FileChannel.MapMode.READ_ONLY, 0L,
                                      (int) fc.size());
        mbb.load();
        try {
            mbb.put((byte)3);
            throw new Exception("Test failed");
        } catch (ReadOnlyBufferException robe) {
            // Correct result
        }
        fc.close();
        raf.close();
    }
}
