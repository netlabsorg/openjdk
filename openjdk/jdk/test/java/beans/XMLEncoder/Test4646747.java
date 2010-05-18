/*
 * Copyright 2004-2007 Sun Microsystems, Inc.  All Rights Reserved.
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
 * @bug 4646747
 * @summary Tests that persistence delegate is correct after memory stress
 * @author Mark Davidson
 */

import java.beans.DefaultPersistenceDelegate;
import java.beans.PersistenceDelegate;
import java.beans.XMLEncoder;

/**
 * This bug was introduced in 1.4 FCS but was working in 1.4.beta3
 */
public class Test4646747 {
    public static void main(String[] args) {
        XMLEncoder encoder = new XMLEncoder(System.out);
        encoder.setPersistenceDelegate(Test4646747.class, new MyPersistenceDelegate());
        // WARNING: This can eat up a lot of memory
        Object[] obs = new Object[10000];
        for (int i = 0; i < obs.length; i++) {
            obs[i] = new int[1000];
        }
        System.gc();
        System.gc();
        PersistenceDelegate pd = encoder.getPersistenceDelegate(Test4646747.class);
        if (!(pd instanceof MyPersistenceDelegate))
            throw new Error("persistence delegate has been lost");
    }

    private static class MyPersistenceDelegate extends DefaultPersistenceDelegate {
    }
}
