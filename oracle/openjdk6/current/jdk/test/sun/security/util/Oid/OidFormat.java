/*
 * Copyright 2006 Sun Microsystems, Inc.  All Rights Reserved.
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
 * @author Weijun Wang
 * @bug 6418422
 * @bug 6418425
 * @bug 6418433
 * @summary ObjectIdentifier should reject 1.2.3.-4 and throw IOException on all format errors
 */

import java.io.IOException;
import java.security.NoSuchAlgorithmException;
import org.ietf.jgss.GSSException;
import org.ietf.jgss.Oid;
import javax.crypto.EncryptedPrivateKeyInfo;
import sun.security.util.*;
import java.util.Arrays;

public class OidFormat {
    public static void main(String[] args) throws Exception {
        String[] badOids = {
            // number problems
            "0", "1", "2",
            "3.1.1", "3", "4",
            "1.40", "1.111.1",
            "-1.2", "0,-2", "1.-2", "2.-2",
            "1.2.-3.4", "1.2.3.-4",
            // format problems
            "aa", "aa.aa",
            "", "....", "1.2..4", "1.2.3.", "1.", "1.2.", "0.1.",
            "1,2",
        };

        for (String s: badOids) {
            testBad(s);
        }

        String[] goodOids = {
            "0.0", "0.1", "1.0", "1.2",
            "0.39", "1.39", "2.47", "2.40.3.6", "2.100.3", "2.123456.3",
            "1.2.3", "1.2.3445",
            "1.3.6.1.4.1.42.2.17",
            // 4811968: ASN.1 cannot handle huge OID components
            //"2.16.764.1.3101555394.1.0.100.2.1",
            //"1.2.2147483647.4",
            //"1.2.268435456.4",
        };

        for (String s: goodOids) {
            testGood(s);
        }

        int[][] goodInts = {
            {0,0}, {0,1}, {1,0}, {1,2},
            {0,39}, {1,39}, {2,47}, {2,40,3,6}, {2,100,3}, {2,123456,3},
            {1,2,3}, {1,2,3445},
            {1,3,6,1,4,1,42,2,17},
        };

        for (int[] is: goodInts) {
            testGood(is);
        }

        int[][] badInts = new int[][] {
            {0}, {1}, {2},
            {3,1,1}, {3}, {4},
            {1,40}, {1,111,1},
            {-1,2}, {0,-2}, {1,-2}, {2,-2},
            {1,2,-3,4}, {1,2,3,-4},
        };

        for (int[] is: badInts) {
            testBad(is);
        }

    }

    static void testBad(int[] ints) throws Exception {
        System.err.println("Trying " + Arrays.toString(ints));
        try {
            new ObjectIdentifier(ints);
            throw new Exception("should be invalid ObjectIdentifier");
        } catch (IOException ioe) {
            System.err.println(ioe);
        }
    }

    static void testGood(int[] ints) throws Exception {
        System.err.println("Trying " + Arrays.toString(ints));
        ObjectIdentifier oid = new ObjectIdentifier(ints);
        DerOutputStream os = new DerOutputStream();
        os.putOID(oid);
        DerInputStream is = new DerInputStream(os.toByteArray());
        ObjectIdentifier oid2 = is.getOID();
        if (!oid.equals((Object)oid2)) {
            throw new Exception("Test DER I/O fails: " + oid + " and " + oid2);
        }
    }

    static void testGood(String s) throws Exception {
        System.err.println("Trying " + s);
        ObjectIdentifier oid = new ObjectIdentifier(s);
        if (!oid.toString().equals(s)) {
            throw new Exception("equal test fail");
        }
        DerOutputStream os = new DerOutputStream();
        os.putOID(oid);
        DerInputStream is = new DerInputStream(os.toByteArray());
        ObjectIdentifier oid2 = is.getOID();
        if (!oid.equals((Object)oid2)) {
            throw new Exception("Test DER I/O fails: " + oid + " and " + oid2);
        }
    }

    static void testBad(String s) throws Exception {
        System.err.println("Trying " + s);
        try {
            new ObjectIdentifier(s);
            throw new Exception("should be invalid ObjectIdentifier");
        } catch (IOException ioe) {
            System.err.println(ioe);
        }

        try {
            new Oid(s);
            throw new Exception("should be invalid Oid");
        } catch (GSSException gsse) {
            ;
        }

        try {
            new EncryptedPrivateKeyInfo(s, new byte[8]);
            throw new Exception("should be invalid algorithm");
        } catch (NoSuchAlgorithmException e) {
            ;
        }
    }
}
