/*
 * Copyright 1998-1999 Sun Microsystems, Inc.  All Rights Reserved.
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
   @bug 4091757
   @summary Basic test for setLastModified method
 */

import java.io.*;


public class SetLastModified {

    private static void ck(File f, long nt, long rt) throws Exception {
        if (rt == nt) return;
        if ((rt / 10 == nt / 10)
            || (rt / 100 == nt / 100)
            || (rt / 1000 == nt / 1000)
            || (rt / 10000 == (nt / 10000))) {
            System.err.println(f + ": Time set to " + nt
                               + ", rounded down by filesystem to " + rt);
            return;
        }
        if ((rt / 10 == (nt + 5) / 10)
            || (rt / 100 == (nt + 50) / 100)
            || (rt / 1000 == (nt + 500) / 1000)
            || (rt / 10000 == ((nt + 5000) / 10000))) {
            System.err.println(f + ": Time set to " + nt
                               + ", rounded up by filesystem to " + rt);
            return;
        }
        throw new Exception(f + ": Time set to " + nt
                            + ", then read as " + rt);
    }

    public static void main(String[] args) throws Exception {
        File d = new File(System.getProperty("test.dir", "."));
        File d2 = new File(d, "x.SetLastModified.dir");
        File f = new File(d2, "x.SetLastModified");
        long ot, t;

        /* New time: One week ago */
        long nt = System.currentTimeMillis() - 1000 * 60 * 60 * 24 * 7;

        if (f.exists()) f.delete();
        if (d2.exists()) d2.delete();
        if (!d2.mkdir()) {
            throw new Exception("Can't create test directory " + d2);
        }

        boolean threw = false;
        try {
            d2.setLastModified(-nt);
        } catch (IllegalArgumentException x) {
            threw = true;
        }
        if (!threw)
            throw new Exception("setLastModified succeeded with a negative time");

        ot = d2.lastModified();
        if (ot != 0) {
            if (d2.setLastModified(nt)) {
                ck(d2, nt, d2.lastModified());
                d2.setLastModified(ot);
            } else {
                System.err.println("Warning: setLastModified on directories "
                                   + "not supported");
            }
        }

        if (f.exists()) {
            if (!f.delete())
                throw new Exception("Can't delete test file " + f);
        }
        if (f.setLastModified(nt))
            throw new Exception("Succeeded on non-existent file: " + f);

        OutputStream o = new FileOutputStream(f);
        o.write('x');
        o.close();
        ot = f.lastModified();
        if (!f.setLastModified(nt))
            throw new Exception("setLastModified failed on file: " + f);
        ck(f, nt, f.lastModified());

        if (!f.delete()) throw new Exception("Can't delete test file " + f);
        if (!d2.delete()) throw new Exception("Can't delete test directory " + d2);
    }

}
