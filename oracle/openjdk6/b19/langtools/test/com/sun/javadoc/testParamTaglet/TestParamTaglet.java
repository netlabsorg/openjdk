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

/*
 * @test
 * @bug      4802275 4967243
 * @summary  Make sure param tags are still printed even though they do not
 *           match up with a real parameters.
 *           Make sure inheritDoc cannot be used in an invalid param tag.
 * @author   jamieh
 * @library  ../lib/
 * @build    JavadocTester
 * @build    TestParamTaglet
 * @run main TestParamTaglet
 */

public class TestParamTaglet extends JavadocTester {

    //Test information.
    private static final String BUG_ID = "4802275-4967243";

    //Javadoc arguments.
    private static final String[] ARGS = new String[] {
        "-d", BUG_ID, "-sourcepath", SRC_DIR, "pkg"
    };

    //Input for string search tests.
    private static final String[][] TEST = {
        //Regular param tags.
        {BUG_ID + FS + "pkg" + FS + "C.html",
            "<B>Parameters:</B><DD><CODE>param1</CODE> - testing 1 2 3." +
                "<DD><CODE>param2</CODE> - testing 1 2 3."
        },
        //Param tags that don't match with any real parameters.
        {BUG_ID + FS + "pkg" + FS + "C.html",
            "<B>Parameters:</B><DD><CODE><I>p1</I></CODE> - testing 1 2 3." +
                "<DD><CODE><I>p2</I></CODE> - testing 1 2 3."
        },
        //{@inherit} doc misuse does not cause doclet to throw exception.
        // Param is printed with nothing inherited.
        //XXX: in the future when Configuration is available during doc inheritence,
        //print a warning for this mistake.
        {BUG_ID + FS + "pkg" + FS + "C.html",
            "<CODE><I>inheritBug</I></CODE> -"
        },

    };
    private static final String[][] NEGATED_TEST = NO_TEST;

    /**
     * The entry point of the test.
     * @param args the array of command line arguments.
     */
    public static void main(String[] args) {
        TestParamTaglet tester = new TestParamTaglet();
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
