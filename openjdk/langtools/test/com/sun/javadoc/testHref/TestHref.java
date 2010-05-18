/*
 * Copyright 2003-2005 Sun Microsystems, Inc.  All Rights Reserved.
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
 * @bug      4663254
 * @summary  Verify that spaces do not appear in hrefs and anchors.
 * @author   jamieh
 * @library  ../lib/
 * @build    JavadocTester
 * @build    TestHref
 * @run main TestHref
 */

public class TestHref extends JavadocTester {

    //Test information.
    private static final String BUG_ID = "4663254";

    //Javadoc arguments.
    private static final String[] ARGS = new String[] {
        "-d", BUG_ID, "-source", "1.5", "-sourcepath", SRC_DIR, "-linkoffline",
        "http://java.sun.com/j2se/1.4/docs/api/", SRC_DIR, "pkg"
    };

    //Input for string search tests.
    private static final String[][] TEST = {
        //External link.
        {BUG_ID + FS + "pkg" + FS + "C1.html",
            "HREF=\"http://java.sun.com/j2se/1.4/docs/api/java/lang/Object.html?is-external=true#wait(long, int)\""
        },
        //Member summary table link.
        {BUG_ID + FS + "pkg" + FS + "C1.html",
            "HREF=\"../pkg/C1.html#method(int, int, java.util.ArrayList)\""
        },
        //Anchor test.
        {BUG_ID + FS + "pkg" + FS + "C1.html",
            "<A NAME=\"method(int, int, java.util.ArrayList)\"><!-- --></A>"
        },
        //Backward compatibility anchor test.
        {BUG_ID + FS + "pkg" + FS + "C1.html",
            "<A NAME=\"method(int, int, java.util.ArrayList)\"><!-- --></A>"
        },
        //{@link} test.
        {BUG_ID + FS + "pkg" + FS + "C2.html",
            "Link: <A HREF=\"../pkg/C1.html#method(int, int, java.util.ArrayList)\">"
        },
        //@see test.
        {BUG_ID + FS + "pkg" + FS + "C2.html",
            "See Also:</B><DD><A HREF=\"../pkg/C1.html#method(int, int, java.util.ArrayList)\">"
        },

        //Header does not link to the page itself.
        {BUG_ID + FS + "pkg" + FS + "C4.html",
            "Class C4&lt;E extends C4&lt;E&gt;&gt;</H2>"
        },

        //Signature does not link to the page itself.
        {BUG_ID + FS + "pkg" + FS + "C4.html",
            "public abstract class <B>C4&lt;E extends C4&lt;E&gt;&gt;</B>"
        },
    };
    private static final String[][] NEGATED_TEST =
    {
        {WARNING_OUTPUT,  "<a> tag is malformed"}
    };

    /**
     * The entry point of the test.
     * @param args the array of command line arguments.
     */
    public static void main(String[] args) {
        TestHref tester = new TestHref();
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
