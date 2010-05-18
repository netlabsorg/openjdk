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
 * @bug     6341534
 * @summary PackageElement.getEnclosedElements results in NullPointerException from parse(JavaCompiler.java:429)
 * @author  Steve Sides
 * @author  Peter von der Ahe
 * @compile T6341534.java
 * @compile -proc:only -processor T6341534 dir/package-info.java
 * @compile -processor T6341534 dir/package-info.java
 */

import javax.annotation.processing.*;
import javax.lang.model.element.*;
import javax.lang.model.util.*;
import static javax.lang.model.util.ElementFilter.*;
import java.util.*;
import java.util.Set;
import static javax.tools.Diagnostic.Kind.*;

@SupportedAnnotationTypes("*")
public class T6341534 extends AbstractProcessor {
    Elements elements;
    Messager messager;
    public void init(ProcessingEnvironment penv)  {
        super.init(penv);
        elements = penv.getElementUtils();
        messager = processingEnv.getMessager();
    }

    public boolean process(Set<? extends TypeElement> tes, RoundEnvironment renv)  {
        messager.printMessage(NOTE,
                              String.valueOf(elements.getPackageElement("no.such.package")));
        PackageElement dir = elements.getPackageElement("dir");
        messager.printMessage(NOTE, dir.getQualifiedName().toString());
        for (Element e : dir.getEnclosedElements())
            messager.printMessage(NOTE, e.toString());
        return true;
    }
}
