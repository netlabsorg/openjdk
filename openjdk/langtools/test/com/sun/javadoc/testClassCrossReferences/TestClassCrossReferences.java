/*
 * Copyright 2002-2005 Sun Microsystems, Inc.  All Rights Reserved.
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
 * @bug 4652655 4857717
 * @summary This test verifies that class cross references work properly.
 * @author jamieh
 * @library ../lib/
 * @build JavadocTester
 * @build TestClassCrossReferences
 * @run main TestClassCrossReferences
 */

public class TestClassCrossReferences extends JavadocTester {

    private static final String BUG_ID = "4652655-4857717";
    private static final String[][] TEST = {
        {BUG_ID + FS + "C.html",
            "<A HREF=\"http://java.sun.com/j2se/1.4/docs/api/java/math/package-summary.html?is-external=true\"><CODE>Link to math package</CODE></A>"},
        {BUG_ID + FS + "C.html",
            "<A HREF=\"http://java.sun.com/j2se/1.4/docs/api/javax/swing/text/AbstractDocument.AttributeContext.html?is-external=true\" " +
            "title=\"class or interface in javax.swing.text\"><CODE>Link to AttributeContext innerclass</CODE></A>"},
        {BUG_ID + FS + "C.html",
            "<A HREF=\"http://java.sun.com/j2se/1.4/docs/api/java/math/BigDecimal.html?is-external=true\" " +
                "title=\"class or interface in java.math\"><CODE>Link to external class BigDecimal</CODE></A>"},
        {BUG_ID + FS + "C.html",
            "<A HREF=\"http://java.sun.com/j2se/1.4/docs/api/java/math/BigInteger.html?is-external=true#gcd(java.math.BigInteger)\" " +
                "title=\"class or interface in java.math\"><CODE>Link to external member gcd</CODE></A>"},
        {BUG_ID + FS + "C.html",
            "<B>Overrides:</B><DD><CODE>toString</CODE> in class <CODE>java.lang.Object</CODE>"}
    };
    private static final String[][] NEGATED_TEST = NO_TEST;
    private static final String[] ARGS =
        new String[] {
            "-d", BUG_ID, "-sourcepath", SRC_DIR,
            "-linkoffline", "http://java.sun.com/j2se/1.4/docs/api/",
            SRC_DIR, SRC_DIR + FS + "C.java"};

    /**
     * The entry point of the test.
     * @param args the array of command line arguments.
     */
    public static void main(String[] args) {
        TestClassCrossReferences tester = new TestClassCrossReferences();
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
