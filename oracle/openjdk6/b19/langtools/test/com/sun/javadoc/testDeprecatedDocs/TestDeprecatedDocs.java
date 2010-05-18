/*
 * Copyright 2003-2004 Sun Microsystems, Inc.  All Rights Reserved.
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
 * @bug      4927552
 * @summary  <DESC>
 * @author   jamieh
 * @library  ../lib/
 * @build    JavadocTester
 * @build    TestDeprecatedDocs
 * @run main TestDeprecatedDocs
 */

public class TestDeprecatedDocs extends JavadocTester {

    //Test information.
    private static final String BUG_ID = "4927552";

    //Javadoc arguments.
    private static final String[] ARGS = new String[] {
        "-d", BUG_ID, "-source", "1.5", "-sourcepath", SRC_DIR, "pkg"
    };

    private static final String TARGET_FILE  =
        BUG_ID + FS + "deprecated-list.html";

    private static final String TARGET_FILE2  =
        BUG_ID + FS + "pkg" + FS + "DeprecatedClassByAnnotation.html";

    //Input for string search tests.
    private static final String[][] TEST = {
        {TARGET_FILE, "annotation_test1 passes"},
        {TARGET_FILE, "annotation_test2 passes"},
        {TARGET_FILE, "annotation_test3 passes"},
        {TARGET_FILE, "class_test1 passes"},
        {TARGET_FILE, "class_test2 passes"},
        {TARGET_FILE, "class_test3 passes"},
        {TARGET_FILE, "class_test4 passes"},
        {TARGET_FILE, "enum_test1 passes"},
        {TARGET_FILE, "enum_test2 passes"},
        {TARGET_FILE, "error_test1 passes"},
        {TARGET_FILE, "error_test2 passes"},
        {TARGET_FILE, "error_test3 passes"},
        {TARGET_FILE, "error_test4 passes"},
        {TARGET_FILE, "exception_test1 passes"},
        {TARGET_FILE, "exception_test2 passes"},
        {TARGET_FILE, "exception_test3 passes"},
        {TARGET_FILE, "exception_test4 passes"},
        {TARGET_FILE, "interface_test1 passes"},
        {TARGET_FILE, "interface_test2 passes"},
        {TARGET_FILE, "interface_test3 passes"},
        {TARGET_FILE, "interface_test4 passes"},
        {TARGET_FILE, "pkg.DeprecatedClassByAnnotation"},
        {TARGET_FILE, "pkg.DeprecatedClassByAnnotation()"},
        {TARGET_FILE, "pkg.DeprecatedClassByAnnotation.method()"},
        {TARGET_FILE, "pkg.DeprecatedClassByAnnotation.field"},

        {TARGET_FILE2, "<B>Deprecated.</B>" + NL +
                "<P>" + NL +
            "<DL>" + NL +
            "<DT><PRE><FONT SIZE=\"-1\">@Deprecated" + NL +
            "</FONT>public class <B>DeprecatedClassByAnnotation</B>"},

        {TARGET_FILE2, "public int <B>field</B></PRE>" + NL +
            "<DL>" + NL +
            "<DD><B>Deprecated.</B>&nbsp;<DL>"},

        {TARGET_FILE2, "<FONT SIZE=\"-1\">@Deprecated" + NL +
            "</FONT>public <B>DeprecatedClassByAnnotation</B>()</PRE>" + NL +
            "<DL>" + NL +
            "<DD><B>Deprecated.</B>"},

        {TARGET_FILE2, "<FONT SIZE=\"-1\">@Deprecated" + NL +
            "</FONT>public void <B>method</B>()</PRE>" + NL +
            "<DL>" + NL +
            "<DD><B>Deprecated.</B>"},
    };

    private static final String[][] NEGATED_TEST = NO_TEST;

    /**
     * The entry point of the test.
     * @param args the array of command line arguments.
     */
    public static void main(String[] args) {
        TestDeprecatedDocs tester = new TestDeprecatedDocs();
        run(tester, ARGS, TEST, NEGATED_TEST);
        tester.printSummary();
    }

    /**
     * {@inheritDoc}
     */
    public String getBugId() {
        return BUG_ID;
    }

    /**
     * {@inheritDoc}
     */
    public String getBugName() {
        return getClass().getName();
    }
}
