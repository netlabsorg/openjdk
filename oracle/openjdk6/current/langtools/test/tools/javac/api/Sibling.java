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

/**
 * @test
 * @bug     6399602
 * @summary Verify that files are created relative to sibling
 * @author  Peter von der Ah\u00e9
 * @ignore 6877223 test ignored because of issues with File.toUri on Windows (6877206)
 */

import java.io.File;
import java.io.IOException;
import java.util.Arrays;
import javax.tools.*;

import static javax.tools.StandardLocation.CLASS_OUTPUT;
import static javax.tools.JavaFileObject.Kind.CLASS;

public class Sibling {
    public static void main(String... args) throws IOException {
        JavaCompiler compiler = ToolProvider.getSystemJavaCompiler();
        StandardJavaFileManager fm = compiler.getStandardFileManager(null, null, null);
        JavaFileObject sibling =
            fm.getJavaFileObjectsFromFiles(Arrays.asList(new File("Test.java")))
            .iterator().next();
        JavaFileObject classFile =  fm.getJavaFileForOutput(CLASS_OUTPUT,
                                                            "foo.bar.baz.Test",
                                                            CLASS,
                                                            sibling);
        String name =
            new File("Test.class").getAbsolutePath().replace(File.separatorChar, '/');
        if (!classFile.toUri().getPath().equals(name))
            throw new AssertionError("Expected " + name + ", got " +
                                     classFile.toUri().getPath());
    }
}
