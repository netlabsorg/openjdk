/*
 * Copyright 2005-2006 Sun Microsystems, Inc.  All Rights Reserved.
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
 * @bug     6341173 6341072
 * @summary Test presence of Messager methods
 * @author  Joseph D. Darcy
 * @compile MessagerBasics.java
 * @compile -processor MessagerBasics -proc:only MessagerBasics.java
 * @compile/fail -processor MessagerBasics -proc:only -AfinalError MessagerBasics.java
 * @compile -processor MessagerBasics MessagerBasics.java
 * @compile/fail -processor MessagerBasics -AfinalError MessagerBasics.java
 */

import java.util.Set;
import javax.annotation.processing.*;
import javax.lang.model.element.*;
import javax.lang.model.util.*;
import static javax.tools.Diagnostic.Kind.*;

@SupportedAnnotationTypes("*")
@SupportedOptions("finalError")
public class MessagerBasics extends AbstractProcessor {
    public boolean process(Set<? extends TypeElement> annotations,
                           RoundEnvironment roundEnv) {
        Messager m = processingEnv.getMessager();
        if (roundEnv.processingOver()) {
            if (processingEnv.getOptions().containsKey("finalError"))
                m.printMessage(ERROR,   "Does not compute");
            else {
                m.printMessage(NOTE,    "Post no bills");
                m.printMessage(WARNING, "Beware the ides of March!");
            }
        }
        return true;
    }
}
