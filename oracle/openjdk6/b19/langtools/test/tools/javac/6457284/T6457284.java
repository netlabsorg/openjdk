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
 * @bug     6457284
 * @summary Internationalize "unnamed package" when the term is used in diagnostics
 * @author  Peter von der Ah\u00e9
 */

import java.io.IOException;
import java.net.URI;
import javax.lang.model.element.Element;

import com.sun.tools.javac.api.JavacTaskImpl;
import com.sun.tools.javac.util.Context;
import com.sun.tools.javac.util.List;
import com.sun.tools.javac.util.Messages;

import javax.tools.*;

public class T6457284 {
    static class MyFileObject extends SimpleJavaFileObject {
        public MyFileObject() {
            super(URI.create("myfo:/Test.java"), JavaFileObject.Kind.SOURCE);
        }
        public CharSequence getCharContent(boolean ignoreEncodingErrors) {
            return "class Test {}";
        }
    }
    public static void main(String[] args) throws IOException {
        JavaCompiler compiler = ToolProvider.getSystemJavaCompiler();
        JavacTaskImpl task = (JavacTaskImpl)compiler.getTask(null, null, null, null, null,
                                                             List.of(new MyFileObject()));
        MyMessages.preRegister(task.getContext());
        task.parse();
        for (Element e : task.analyze()) {
            if (!e.getEnclosingElement().toString().equals("compiler.misc.unnamed.package"))
                throw new AssertionError(e.getEnclosingElement());
            System.out.println("OK: " + e.getEnclosingElement());
            return;
        }
        throw new AssertionError("No top-level classes!");
    }

    static class MyMessages extends Messages {
        static void preRegister(Context context) {
            context.put(messagesKey, new MyMessages());
        }
        MyMessages() {
            super("com.sun.tools.javac.resources.compiler");
        }
        public String getLocalizedString(String key, Object... args) {
            if (key.equals("compiler.misc.unnamed.package"))
                return key;
            else
                return super.getLocalizedString(key, args);
        }
    }
}
