/*
 * Copyright 2004 Sun Microsystems, Inc.  All Rights Reserved.
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
 * @bug 4979440
 * @summary Test for signature parsing corner case
 * @author Joseph D. Darcy
 * @compile -source 1.5 getAnnotationTest.java
 * @run main getAnnotationTest
 */

import java.lang.reflect.*;
import java.lang.annotation.*;

/*
 * Make sure:
 * 1. getAnnotation can be called directly
 * 2. getAnnotation can be called reflectively
 * 3. generic information methods on the Method object for
 * getAnnotation can be called
 */

public class getAnnotationTest {
    public static void main (String[] args) throws Throwable {
        // Base level
        Class c = Class.forName("java.lang.annotation.Retention");
        Annotation result  = c.getAnnotation(Retention.class);
        // System.out.println("Base result:" + result);

        // Meta level, invoke Class.getAnnotation reflectively...
        Class meta_c = c.getClass();
        Method meta_getAnnotation = meta_c.getMethod("getAnnotation",
                                                     (Retention.class).getClass());

        Object meta_result = meta_getAnnotation.invoke(c, Retention.class);
        // System.out.println("Meta result:" + meta_result);

        if (!meta_result.equals(result)) {
            throw new RuntimeException("Base and meta results are not equal.");
        }

        meta_getAnnotation.getGenericExceptionTypes();
        meta_getAnnotation.getGenericParameterTypes();
        meta_getAnnotation.getGenericReturnType();
    }
}
