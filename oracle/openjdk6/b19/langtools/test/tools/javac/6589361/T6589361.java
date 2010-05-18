/**
 * @test    @(#)T6589361.java   1.1 07/07/18
 * @bug     6589361
 * @summary 6589361:Failing building ct.sym file as part of the control build
 */

import com.sun.tools.javac.util.Context;
import com.sun.tools.javac.util.JavacFileManager;
import java.io.File;
import javax.tools.FileObject;
import javax.tools.JavaFileObject;
import javax.tools.JavaFileObject.Kind;
import javax.tools.StandardLocation;
import java.util.Set;
import java.util.HashSet;

public class T6589361 {
    public static void main(String [] args) throws Exception {
        JavacFileManager fm = null;
        try {
            fm = new JavacFileManager(new Context(), false, null);
            Set<JavaFileObject.Kind> set = new HashSet<JavaFileObject.Kind>();
            set.add(JavaFileObject.Kind.CLASS);
            Iterable<JavaFileObject> files = fm.list(StandardLocation.PLATFORM_CLASS_PATH, "java.lang", set, false);
            for (JavaFileObject file : files) {

                if (file.toString().startsWith("java" + File.separator + "lang" + File.separator + "Object.class")) {
                    String str = fm.inferBinaryName(StandardLocation.CLASS_PATH, file);
                    if (!str.equals("java.lang.Object")) {
                        throw new AssertionError("Error in JavacFileManager.inferBinaryName method!");
                    }
                    else {
                        return;
                    }
                }
            }
        }
        finally {
            if (fm != null) {
                fm.close();
            }
        }
        throw new AssertionError("Could not find java/lang/Object.class while compiling");
    }

}
