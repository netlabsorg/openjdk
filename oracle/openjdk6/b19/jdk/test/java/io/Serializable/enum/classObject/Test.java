/*
 * Copyright 2003 Sun Microsystems, Inc.  All Rights Reserved.
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
 * @bug 4838379
 * @summary Verify that serialization of Class objects for enum types works
 *          properly.
 *
 * @compile -source 1.5 Test.java
 * @run main Test
 */

import java.io.*;

enum Foo { foo, bar { int i = 0; }, baz { double d = 3.0; } }

public class Test {
    public static void main(String[] args) throws Exception {
        ByteArrayOutputStream bout = new ByteArrayOutputStream();
        ObjectOutputStream oout = new ObjectOutputStream(bout);
        Class[] classes = { Enum.class, Foo.foo.getClass(),
                            Foo.bar.getClass(), Foo.baz.getClass() };
        for (int i = 0; i < classes.length; i++) {
            oout.writeObject(classes[i]);
        }
        oout.close();

        ObjectInputStream oin = new ObjectInputStream(
            new ByteArrayInputStream(bout.toByteArray()));
        for (int i = 0; i < classes.length; i++) {
            Object obj = oin.readObject();
            if (obj != classes[i]) {
                throw new Error("expected " + classes[i] + ", got " + obj);
            }
        }
    }
}
