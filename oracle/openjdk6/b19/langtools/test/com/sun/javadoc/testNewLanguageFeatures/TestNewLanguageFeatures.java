/*
 * Copyright 2003-2006 Sun Microsystems, Inc.  All Rights Reserved.
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
 * @bug      4789689 4905985 4927164 4827184 4993906 5004549
 * @summary  Run Javadoc on a set of source files that demonstrate new
 *           language features.  Check the output to ensure that the new
 *           language features are properly documented.
 * @author   jamieh
 * @library  ../lib/
 * @build    JavadocTester
 * @build    TestNewLanguageFeatures
 * @run main TestNewLanguageFeatures
 */

public class TestNewLanguageFeatures extends JavadocTester {

    //Test information.
    private static final String BUG_ID = "4789689-4905985-4927164-4827184-4993906";

    //Javadoc arguments.
    private static final String[] ARGS = new String[] {
        "-d", BUG_ID, "-use", "-source", "1.5", "-sourcepath", SRC_DIR, "pkg", "pkg1", "pkg2"
    };

    //Input for string search tests.
    private static final String[][] TEST =
        {
            //=================================
            // ENUM TESTING
            //=================================
            //Make sure enum header is correct.
            {BUG_ID + FS + "pkg" + FS + "Coin.html", "Enum Coin</H2>"},
            //Make sure enum signature is correct.
            {BUG_ID + FS + "pkg" + FS + "Coin.html", "public enum "+
                "<B>Coin</B><DT>extends java.lang.Enum&lt;" +
                "<A HREF=\"../pkg/Coin.html\" title=\"enum in pkg\">Coin</A>&gt;"
            },
            //Check for enum constant section
            {BUG_ID + FS + "pkg" + FS + "Coin.html", "<B>Enum Constant Summary</B>"},
            //Detail for enum constant
            {BUG_ID + FS + "pkg" + FS + "Coin.html",
                "<B><A HREF=\"../pkg/Coin.html#Dime\">Dime</A></B>"},
            //Automatically insert documentation for values() and valueOf().
            {BUG_ID + FS + "pkg" + FS + "Coin.html",
                "Returns an array containing the constants of this enum type,"},
            {BUG_ID + FS + "pkg" + FS + "Coin.html",
                "Returns the enum constant of this type with the specified name"},
            {BUG_ID + FS + "pkg" + FS + "Coin.html", "for (Coin c : Coin.values())"},
            {BUG_ID + FS + "pkg" + FS + "Coin.html", "Overloaded valueOf() method has correct documentation."},
            {BUG_ID + FS + "pkg" + FS + "Coin.html", "Overloaded values method  has correct documentation."},

            //=================================
            // TYPE PARAMETER TESTING
            //=================================
            //Make sure the header is correct.
            {BUG_ID + FS + "pkg" + FS + "TypeParameters.html",
                "Class TypeParameters&lt;E&gt;</H2>"},
            //Check class type parameters section.
            {BUG_ID + FS + "pkg" + FS + "TypeParameters.html",
                "<DT><B>Type Parameters:</B><DD><CODE>E</CODE> - " +
                "the type parameter for this class."},
            //Type parameters in @see/@link
            {BUG_ID + FS + "pkg" + FS + "TypeParameters.html",
                "<DT><B>See Also:</B><DD><A HREF=\"../pkg/TypeParameters.html\" " +
                    "title=\"class in pkg\"><CODE>TypeParameters</CODE></A></DL>"},
            //Method that uses class type parameter.
            {BUG_ID + FS + "pkg" + FS + "TypeParameters.html",
                "(<A HREF=\"../pkg/TypeParameters.html\" title=\"type " +
                    "parameter in TypeParameters\">E</A>&nbsp;param)"},
            //Method type parameter section.
            {BUG_ID + FS + "pkg" + FS + "TypeParameters.html",
                "<B>Type Parameters:</B><DD><CODE>T</CODE> - This is the first " +
                    "type parameter.<DD><CODE>V</CODE> - This is the second type " +
                    "parameter."},
            //Signature of method with type parameters
            {BUG_ID + FS + "pkg" + FS + "TypeParameters.html",
                "public &lt;T extends java.util.List,V&gt; " +
                    "java.lang.String[] <B>methodThatHasTypeParameters</B>"},
            //Wildcard testing.
            {BUG_ID + FS + "pkg" + FS + "Wildcards.html",
                "<A HREF=\"../pkg/TypeParameters.html\" title=\"class in pkg\">" +
                "TypeParameters</A>&lt;? super java.lang.String&gt;&nbsp;a"},
            {BUG_ID + FS + "pkg" + FS + "Wildcards.html",
                "<A HREF=\"../pkg/TypeParameters.html\" title=\"class in pkg\">" +
                "TypeParameters</A>&lt;? extends java.lang.StringBuffer&gt;&nbsp;b"},
            {BUG_ID + FS + "pkg" + FS + "Wildcards.html",
                "<A HREF=\"../pkg/TypeParameters.html\" title=\"class in pkg\">" +
                    "TypeParameters</A>&nbsp;c"},
            //Bad type parameter warnings.
            {WARNING_OUTPUT, "warning - @param argument " +
                "\"<BadClassTypeParam>\" is not a type parameter name."},
            {WARNING_OUTPUT, "warning - @param argument " +
                "\"<BadMethodTypeParam>\" is not a type parameter name."},

            //Signature of subclass that has type parameters.
            {BUG_ID + FS + "pkg" + FS + "TypeParameterSubClass.html",
                "public class <B>TypeParameterSubClass&lt;T extends java.lang.String&gt;" +
                "</B><DT>extends <A HREF=\"../pkg/TypeParameterSuperClass.html\" " +
                "title=\"class in pkg\">TypeParameterSuperClass</A>&lt;T&gt;"},

            //Interface generic parameter substitution
            //Signature of subclass that has type parameters.
            {BUG_ID + FS + "pkg" + FS + "TypeParameters.html",
                "<B>All Implemented Interfaces:</B> <DD><A HREF=\"../pkg/SubInterface.html\" title=\"interface in pkg\">SubInterface</A>&lt;E&gt;, <A HREF=\"../pkg/SuperInterface.html\" title=\"interface in pkg\">SuperInterface</A>&lt;E&gt;</DD>"},
            {BUG_ID + FS + "pkg" + FS + "SuperInterface.html",
                "<B>All Known Subinterfaces:</B> <DD><A HREF=\"../pkg/SubInterface.html\" title=\"interface in pkg\">SubInterface</A>&lt;V&gt;</DD>"},
            {BUG_ID + FS + "pkg" + FS + "SubInterface.html",
                "<B>All Superinterfaces:</B> <DD><A HREF=\"../pkg/SuperInterface.html\" title=\"interface in pkg\">SuperInterface</A>&lt;V&gt;</DD>"},

            //=================================
            // VAR ARG TESTING
            //=================================
            {BUG_ID + FS + "pkg" + FS + "VarArgs.html", "(int...&nbsp;i)"},
            {BUG_ID + FS + "pkg" + FS + "VarArgs.html", "(int[][]...&nbsp;i)"},
            {BUG_ID + FS + "pkg" + FS + "VarArgs.html", "(int[]...)"},
            {BUG_ID + FS + "pkg" + FS + "VarArgs.html",
                "<A HREF=\"../pkg/TypeParameters.html\" title=\"class in pkg\">" +
                "TypeParameters</A>...&nbsp;t"},

            //=================================
            // ANNOTATION TYPE TESTING
            //=================================
            //Make sure the summary links are correct.
            {BUG_ID + FS + "pkg" + FS + "AnnotationType.html",
                "SUMMARY:&nbsp;<A HREF=\"#annotation_type_required_element_summary\">" +
                "REQUIRED</A>&nbsp;|&nbsp;<A HREF=\"#annotation_type_optional_element_summary\">" +
                "OPTIONAL</A>"},
            //Make sure the detail links are correct.
            {BUG_ID + FS + "pkg" + FS + "AnnotationType.html",
                "DETAIL:&nbsp;<A HREF=\"#annotation_type_element_detail\">ELEMENT</A>"},
            //Make sure the heading is correct.
            {BUG_ID + FS + "pkg" + FS + "AnnotationType.html",
                "Annotation Type AnnotationType</H2>"},
            //Make sure the signature is correct.
            {BUG_ID + FS + "pkg" + FS + "AnnotationType.html",
                "public @interface <B>AnnotationType</B>"},
            //Make sure member summary headings are correct.
            {BUG_ID + FS + "pkg" + FS + "AnnotationType.html",
                "<B>Required Element Summary</B>"},
            {BUG_ID + FS + "pkg" + FS + "AnnotationType.html",
                "<B>Optional Element Summary</B>"},
            //Make sure element detail heading is correct
            {BUG_ID + FS + "pkg" + FS + "AnnotationType.html",
                "Element Detail"},
            //Make sure default annotation type value is printed when necessary.
            {BUG_ID + FS + "pkg" + FS + "AnnotationType.html",
                "<B>Default:</B><DD>\"unknown\"</DD>"},

            //=================================
            // ANNOTATION TYPE USAGE TESTING
            //=================================

            //PACKAGE
            {BUG_ID + FS + "pkg" + FS + "package-summary.html",
                "<A HREF=\"../pkg/AnnotationType.html\" title=\"annotation in pkg\">@AnnotationType</A>(<A HREF=\"../pkg/AnnotationType.html#optional()\">optional</A>=\"Package Annotation\"," + NL +
                "                <A HREF=\"../pkg/AnnotationType.html#required()\">required</A>=1994)"},

            //CLASS
            {BUG_ID + FS + "pkg" + FS + "AnnotationTypeUsage.html",
                "<FONT SIZE=\"-1\">" +
                "<A HREF=\"../pkg/AnnotationType.html\" title=\"annotation in pkg\">@AnnotationType</A>(<A HREF=\"../pkg/AnnotationType.html#optional()\">optional</A>=\"Class Annotation\","+NL +
                "                <A HREF=\"../pkg/AnnotationType.html#required()\">required</A>=1994)"+NL +
                "</FONT>public class <B>AnnotationTypeUsage</B><DT>extends java.lang.Object</DL>"},

            //FIELD
            {BUG_ID + FS + "pkg" + FS + "AnnotationTypeUsage.html",
                "<FONT SIZE=\"-1\">" +
                "<A HREF=\"../pkg/AnnotationType.html\" title=\"annotation in pkg\">@AnnotationType</A>(<A HREF=\"../pkg/AnnotationType.html#optional()\">optional</A>=\"Field Annotation\","+NL +
                "                <A HREF=\"../pkg/AnnotationType.html#required()\">required</A>=1994)"+NL +
                "</FONT>public int <B>field</B>"},

            //CONSTRUCTOR
            {BUG_ID + FS + "pkg" + FS + "AnnotationTypeUsage.html",
                "<FONT SIZE=\"-1\">" +
                "<A HREF=\"../pkg/AnnotationType.html\" title=\"annotation in pkg\">@AnnotationType</A>(<A HREF=\"../pkg/AnnotationType.html#optional()\">optional</A>=\"Constructor Annotation\","+NL +
                "                <A HREF=\"../pkg/AnnotationType.html#required()\">required</A>=1994)"+NL +
                "</FONT>public <B>AnnotationTypeUsage</B>()"},

            //METHOD
            {BUG_ID + FS + "pkg" + FS + "AnnotationTypeUsage.html",
                "<FONT SIZE=\"-1\">" +
                "<A HREF=\"../pkg/AnnotationType.html\" title=\"annotation in pkg\">@AnnotationType</A>(<A HREF=\"../pkg/AnnotationType.html#optional()\">optional</A>=\"Method Annotation\","+NL +
                "                <A HREF=\"../pkg/AnnotationType.html#required()\">required</A>=1994)"+NL +
                "</FONT>public void <B>method</B>()"},

            //METHOD PARAMS
            {BUG_ID + FS + "pkg" + FS + "AnnotationTypeUsage.html",
                "<PRE>" + NL +
                "public void <B>methodWithParams</B>(<FONT SIZE=\"-1\"><A HREF=\"../pkg/AnnotationType.html\" title=\"annotation in pkg\">@AnnotationType</A>(<A HREF=\"../pkg/AnnotationType.html#optional()\">optional</A>=\"Parameter Annotation\",<A HREF=\"../pkg/AnnotationType.html#required()\">required</A>=1994)</FONT>" + NL +
                "                             int&nbsp;documented," + NL +
                "                             int&nbsp;undocmented)</PRE>"},

            //CONSTRUCTOR PARAMS
            {BUG_ID + FS + "pkg" + FS + "AnnotationTypeUsage.html",
                "<PRE>" + NL +
                                "public <B>AnnotationTypeUsage</B>(<FONT SIZE=\"-1\"><A HREF=\"../pkg/AnnotationType.html\" title=\"annotation in pkg\">@AnnotationType</A>(<A HREF=\"../pkg/AnnotationType.html#optional()\">optional</A>=\"Constructor Param Annotation\",<A HREF=\"../pkg/AnnotationType.html#required()\">required</A>=1994)</FONT>" + NL +
                                "                           int&nbsp;documented," + NL +
                "                           int&nbsp;undocmented)</PRE>"},

            //=================================
            // ANNOTATION TYPE USAGE TESTING (All Different Types).
            //=================================

            //Integer
            {BUG_ID + FS + "pkg1" + FS + "B.html",
                "<A HREF=\"../pkg1/A.html#d()\">d</A>=3.14,"},

            //Double
            {BUG_ID + FS + "pkg1" + FS + "B.html",
                "<A HREF=\"../pkg1/A.html#d()\">d</A>=3.14,"},

            //Boolean
            {BUG_ID + FS + "pkg1" + FS + "B.html",
                "<A HREF=\"../pkg1/A.html#b()\">b</A>=true,"},

            //String
            {BUG_ID + FS + "pkg1" + FS + "B.html",
                "<A HREF=\"../pkg1/A.html#s()\">s</A>=\"sigh\","},

            //Class
            {BUG_ID + FS + "pkg1" + FS + "B.html",
                "<A HREF=\"../pkg1/A.html#c()\">c</A>=<A HREF=\"../pkg2/Foo.html\" title=\"class in pkg2\">Foo.class</A>,"},

            //Bounded Class
            {BUG_ID + FS + "pkg1" + FS + "B.html",
                "<A HREF=\"../pkg1/A.html#w()\">w</A>=<A HREF=\"../pkg/TypeParameterSubClass.html\" title=\"class in pkg\">TypeParameterSubClass.class</A>,"},

            //Enum
            {BUG_ID + FS + "pkg1" + FS + "B.html",
                "<A HREF=\"../pkg1/A.html#e()\">e</A>=<A HREF=\"../pkg/Coin.html#Penny\">Penny</A>,"},

            //Annotation Type
            {BUG_ID + FS + "pkg1" + FS + "B.html",
                "<A HREF=\"../pkg1/A.html#a()\">a</A>=<A HREF=\"../pkg/AnnotationType.html\" title=\"annotation in pkg\">@AnnotationType</A>(<A HREF=\"../pkg/AnnotationType.html#optional()\">optional</A>=\"foo\",<A HREF=\"../pkg/AnnotationType.html#required()\">required</A>=1994),"},

            //String Array
            {BUG_ID + FS + "pkg1" + FS + "B.html",
                "<A HREF=\"../pkg1/A.html#sa()\">sa</A>={\"up\",\"down\"},"},

            //Primitive
            {BUG_ID + FS + "pkg1" + FS + "B.html",
                "<A HREF=\"../pkg1/A.html#primitiveClassTest()\">primitiveClassTest</A>=boolean.class,"},

            //XXX:  Add array test case after this if fixed:
            //5020899: Incorrect internal representation of class-valued annotation elements

            //Make sure that annotations are surrounded by <pre> and </pre>
            {BUG_ID + FS + "pkg1" + FS + "B.html",
                "<PRE><FONT SIZE=\"-1\"><A HREF=\"../pkg1/A.html\" title=\"annotation in pkg1\">@A</A>"},
            {BUG_ID + FS + "pkg1" + FS + "B.html",
                "</FONT>public interface <B>B</B></DL>" + NL +
                    "</PRE>"},


            //==============================================================
            // Handle multiple bounds.
            //==============================================================
            {BUG_ID + FS + "pkg" + FS + "MultiTypeParameters.html",
                "public &lt;T extends java.lang.Number & java.lang.Runnable&gt; T <B>foo</B>(T&nbsp;t)"},

            //==============================================================
            // Test Class-Use Documenation for Type Parameters.
            //==============================================================

            //ClassUseTest1: <T extends Foo & Foo2>
            {BUG_ID + FS + "pkg2" + FS + "class-use" + FS + "Foo.html",
                 "<TH ALIGN=\"left\" COLSPAN=\"2\">Classes in <A HREF=\"../../pkg2/package-summary.html\">pkg2</A> with type parameters of type <A HREF=\"../../pkg2/Foo.html\" title=\"class in pkg2\">Foo</A></FONT></TH>"
            },
            {BUG_ID + FS + "pkg2" + FS + "class-use" + FS + "Foo.html",
                "<TD><CODE><B><A HREF=\"../../pkg2/ClassUseTest1.html\" title=\"class in pkg2\">ClassUseTest1&lt;T extends Foo & Foo2&gt;</A></B></CODE>"
            },
            {BUG_ID + FS + "pkg2" + FS + "class-use" + FS + "Foo.html",
                "<TH ALIGN=\"left\" COLSPAN=\"2\">Methods in <A HREF=\"../../pkg2/package-summary.html\">pkg2</A> with type parameters of type <A HREF=\"../../pkg2/Foo.html\" title=\"class in pkg2\">Foo</A></FONT></TH>"
            },
            {BUG_ID + FS + "pkg2" + FS + "class-use" + FS + "Foo.html",
                "<TD><CODE><B>ClassUseTest1.</B><B><A HREF=\"../../pkg2/ClassUseTest1.html#method(T)\">method</A></B>(T&nbsp;t)</CODE>"
            },
            {BUG_ID + FS + "pkg2" + FS + "class-use" + FS + "Foo.html",
                "<TH ALIGN=\"left\" COLSPAN=\"2\">Fields in <A HREF=\"../../pkg2/package-summary.html\">pkg2</A> with type parameters of type <A HREF=\"../../pkg2/Foo.html\" title=\"class in pkg2\">Foo</A></FONT></TH>"
            },
            {BUG_ID + FS + "pkg2" + FS + "class-use" + FS + "Foo.html",
                "<A HREF=\"../../pkg2/ParamTest.html\" title=\"class in pkg2\">ParamTest</A>&lt;<A HREF=\"../../pkg2/Foo.html\" title=\"class in pkg2\">Foo</A>&gt;</CODE></FONT></TD>"
            },

            {BUG_ID + FS + "pkg2" + FS + "class-use" + FS + "ParamTest.html",
                "<TH ALIGN=\"left\" COLSPAN=\"2\">Fields in <A HREF=\"../../pkg2/package-summary.html\">pkg2</A> declared as <A HREF=\"../../pkg2/ParamTest.html\" title=\"class in pkg2\">ParamTest</A></FONT></TH>"
            },
            {BUG_ID + FS + "pkg2" + FS + "class-use" + FS + "ParamTest.html",
                "<A HREF=\"../../pkg2/ParamTest.html\" title=\"class in pkg2\">ParamTest</A>&lt;<A HREF=\"../../pkg2/Foo.html\" title=\"class in pkg2\">Foo</A>&gt;</CODE></FONT></TD>"
            },

           {BUG_ID + FS + "pkg2" + FS + "class-use" + FS + "Foo2.html",
            "<TH ALIGN=\"left\" COLSPAN=\"2\">Classes in <A HREF=\"../../pkg2/package-summary.html\">pkg2</A> with type parameters of type <A HREF=\"../../pkg2/Foo2.html\" title=\"interface in pkg2\">Foo2</A></FONT></TH>"
           },
           {BUG_ID + FS + "pkg2" + FS + "class-use" + FS + "Foo2.html",
            "<TD><CODE><B><A HREF=\"../../pkg2/ClassUseTest1.html\" title=\"class in pkg2\">ClassUseTest1&lt;T extends Foo & Foo2&gt;</A></B></CODE>"
           },
           {BUG_ID + FS + "pkg2" + FS + "class-use" + FS + "Foo2.html",
               "<TH ALIGN=\"left\" COLSPAN=\"2\">Methods in <A HREF=\"../../pkg2/package-summary.html\">pkg2</A> with type parameters of type <A HREF=\"../../pkg2/Foo2.html\" title=\"interface in pkg2\">Foo2</A></FONT></TH>"
            },
            {BUG_ID + FS + "pkg2" + FS + "class-use" + FS + "Foo2.html",
               "<TD><CODE><B>ClassUseTest1.</B><B><A HREF=\"../../pkg2/ClassUseTest1.html#method(T)\">method</A></B>(T&nbsp;t)</CODE>"
            },

            //ClassUseTest2: <T extends ParamTest<Foo3>>
            {BUG_ID + FS + "pkg2" + FS + "class-use" + FS + "ParamTest.html",
              "<TH ALIGN=\"left\" COLSPAN=\"2\">Classes in <A HREF=\"../../pkg2/package-summary.html\">pkg2</A> with type parameters of type <A HREF=\"../../pkg2/ParamTest.html\" title=\"class in pkg2\">ParamTest</A></FONT></TH>"
            },
            {BUG_ID + FS + "pkg2" + FS + "class-use" + FS + "ParamTest.html",
              "<TD><CODE><B><A HREF=\"../../pkg2/ClassUseTest2.html\" title=\"class in pkg2\">ClassUseTest2&lt;T extends ParamTest&lt;<A HREF=\"../../pkg2/Foo3.html\" title=\"class in pkg2\">Foo3</A>&gt;&gt;</A></B></CODE>"
            },
            {BUG_ID + FS + "pkg2" + FS + "class-use" + FS + "ParamTest.html",
              "<TH ALIGN=\"left\" COLSPAN=\"2\">Methods in <A HREF=\"../../pkg2/package-summary.html\">pkg2</A> with type parameters of type <A HREF=\"../../pkg2/ParamTest.html\" title=\"class in pkg2\">ParamTest</A></FONT></TH>"
            },
            {BUG_ID + FS + "pkg2" + FS + "class-use" + FS + "ParamTest.html",
              "<TD><CODE><B>ClassUseTest2.</B><B><A HREF=\"../../pkg2/ClassUseTest2.html#method(T)\">method</A></B>(T&nbsp;t)</CODE>"
            },
            {BUG_ID + FS + "pkg2" + FS + "class-use" + FS + "ParamTest.html",
              "<TH ALIGN=\"left\" COLSPAN=\"2\">Fields in <A HREF=\"../../pkg2/package-summary.html\">pkg2</A> declared as <A HREF=\"../../pkg2/ParamTest.html\" title=\"class in pkg2\">ParamTest</A></FONT></TH>"
            },
            {BUG_ID + FS + "pkg2" + FS + "class-use" + FS + "ParamTest.html",
              "<A HREF=\"../../pkg2/ParamTest.html\" title=\"class in pkg2\">ParamTest</A>&lt;<A HREF=\"../../pkg2/Foo.html\" title=\"class in pkg2\">Foo</A>&gt;</CODE></FONT></TD>"
            },
            {BUG_ID + FS + "pkg2" + FS + "class-use" + FS + "ParamTest.html",
              "<TH ALIGN=\"left\" COLSPAN=\"2\">Methods in <A HREF=\"../../pkg2/package-summary.html\">pkg2</A> with type parameters of type <A HREF=\"../../pkg2/ParamTest.html\" title=\"class in pkg2\">ParamTest</A></FONT></TH>"
            },
            {BUG_ID + FS + "pkg2" + FS + "class-use" + FS + "ParamTest.html",
              "&lt;T extends <A HREF=\"../../pkg2/ParamTest.html\" title=\"class in pkg2\">ParamTest</A>&lt;<A HREF=\"../../pkg2/Foo3.html\" title=\"class in pkg2\">Foo3</A>&gt;&gt;"
            },

            {BUG_ID + FS + "pkg2" + FS + "class-use" + FS + "Foo3.html",
                "<TH ALIGN=\"left\" COLSPAN=\"2\">Classes in <A HREF=\"../../pkg2/package-summary.html\">pkg2</A> with type parameters of type <A HREF=\"../../pkg2/Foo3.html\" title=\"class in pkg2\">Foo3</A></FONT></TH>"
            },
            {BUG_ID + FS + "pkg2" + FS + "class-use" + FS + "Foo3.html",
                "<TD><CODE><B><A HREF=\"../../pkg2/ClassUseTest2.html\" title=\"class in pkg2\">ClassUseTest2&lt;T extends ParamTest&lt;<A HREF=\"../../pkg2/Foo3.html\" title=\"class in pkg2\">Foo3</A>&gt;&gt;</A></B></CODE>"
            },
            {BUG_ID + FS + "pkg2" + FS + "class-use" + FS + "Foo3.html",
                "<TH ALIGN=\"left\" COLSPAN=\"2\">Methods in <A HREF=\"../../pkg2/package-summary.html\">pkg2</A> with type parameters of type <A HREF=\"../../pkg2/Foo3.html\" title=\"class in pkg2\">Foo3</A></FONT></TH>"
            },
            {BUG_ID + FS + "pkg2" + FS + "class-use" + FS + "Foo3.html",
                "<TD><CODE><B>ClassUseTest2.</B><B><A HREF=\"../../pkg2/ClassUseTest2.html#method(T)\">method</A></B>(T&nbsp;t)</CODE>"
            },
            {BUG_ID + FS + "pkg2" + FS + "class-use" + FS + "Foo3.html",
                "<TH ALIGN=\"left\" COLSPAN=\"2\">Methods in <A HREF=\"../../pkg2/package-summary.html\">pkg2</A> that return types with arguments of type <A HREF=\"../../pkg2/Foo3.html\" title=\"class in pkg2\">Foo3</A></FONT></TH>"
            },
            {BUG_ID + FS + "pkg2" + FS + "class-use" + FS + "Foo3.html",
                "&lt;T extends <A HREF=\"../../pkg2/ParamTest.html\" title=\"class in pkg2\">ParamTest</A>&lt;<A HREF=\"../../pkg2/Foo3.html\" title=\"class in pkg2\">Foo3</A>&gt;&gt;"
            },

            //ClassUseTest3: <T extends ParamTest2<List<? extends Foo4>>>
            {BUG_ID + FS + "pkg2" + FS + "class-use" + FS + "ParamTest2.html",
                "<TH ALIGN=\"left\" COLSPAN=\"2\">Classes in <A HREF=\"../../pkg2/package-summary.html\">pkg2</A> with type parameters of type <A HREF=\"../../pkg2/ParamTest2.html\" title=\"class in pkg2\">ParamTest2</A></FONT></TH>"
            },
            {BUG_ID + FS + "pkg2" + FS + "class-use" + FS + "ParamTest2.html",
                "<TD><CODE><B><A HREF=\"../../pkg2/ClassUseTest3.html\" title=\"class in pkg2\">ClassUseTest3&lt;T extends ParamTest2&lt;java.util.List&lt;? extends Foo4&gt;&gt;&gt;</A></B></CODE>"
            },
            {BUG_ID + FS + "pkg2" + FS + "class-use" + FS + "ParamTest2.html",
                "<TH ALIGN=\"left\" COLSPAN=\"2\">Methods in <A HREF=\"../../pkg2/package-summary.html\">pkg2</A> with type parameters of type <A HREF=\"../../pkg2/ParamTest2.html\" title=\"class in pkg2\">ParamTest2</A></FONT></TH>"
            },
            {BUG_ID + FS + "pkg2" + FS + "class-use" + FS + "ParamTest2.html",
                "<TD><CODE><B>ClassUseTest3.</B><B><A HREF=\"../../pkg2/ClassUseTest3.html#method(T)\">method</A></B>(T&nbsp;t)</CODE>"
            },
            {BUG_ID + FS + "pkg2" + FS + "class-use" + FS + "ParamTest2.html",
                "<TH ALIGN=\"left\" COLSPAN=\"2\">Methods in <A HREF=\"../../pkg2/package-summary.html\">pkg2</A> with type parameters of type <A HREF=\"../../pkg2/ParamTest2.html\" title=\"class in pkg2\">ParamTest2</A></FONT></TH>"
            },
            {BUG_ID + FS + "pkg2" + FS + "class-use" + FS + "ParamTest2.html",
                "&lt;T extends <A HREF=\"../../pkg2/ParamTest2.html\" title=\"class in pkg2\">ParamTest2</A>&lt;java.util.List&lt;? extends <A HREF=\"../../pkg2/Foo4.html\" title=\"class in pkg2\">Foo4</A>&gt;&gt;&gt;"
            },

            {BUG_ID + FS + "pkg2" + FS + "class-use" + FS + "Foo4.html",
                "<TH ALIGN=\"left\" COLSPAN=\"2\">Classes in <A HREF=\"../../pkg2/package-summary.html\">pkg2</A> with type parameters of type <A HREF=\"../../pkg2/Foo4.html\" title=\"class in pkg2\">Foo4</A></FONT></TH>"
            },
            {BUG_ID + FS + "pkg2" + FS + "class-use" + FS + "Foo4.html",
                "<TD><CODE><B><A HREF=\"../../pkg2/ClassUseTest3.html\" title=\"class in pkg2\">ClassUseTest3&lt;T extends ParamTest2&lt;java.util.List&lt;? extends Foo4&gt;&gt;&gt;</A></B></CODE>"
            },
            {BUG_ID + FS + "pkg2" + FS + "class-use" + FS + "Foo4.html",
                "<TH ALIGN=\"left\" COLSPAN=\"2\">Methods in <A HREF=\"../../pkg2/package-summary.html\">pkg2</A> with type parameters of type <A HREF=\"../../pkg2/Foo4.html\" title=\"class in pkg2\">Foo4</A></FONT></TH>"
            },
            {BUG_ID + FS + "pkg2" + FS + "class-use" + FS + "Foo4.html",
                "<TD><CODE><B>ClassUseTest3.</B><B><A HREF=\"../../pkg2/ClassUseTest3.html#method(T)\">method</A></B>(T&nbsp;t)</CODE>"
            },
            {BUG_ID + FS + "pkg2" + FS + "class-use" + FS + "Foo4.html",
                "<TH ALIGN=\"left\" COLSPAN=\"2\">Methods in <A HREF=\"../../pkg2/package-summary.html\">pkg2</A> that return types with arguments of type <A HREF=\"../../pkg2/Foo4.html\" title=\"class in pkg2\">Foo4</A></FONT></TH>"
            },
            {BUG_ID + FS + "pkg2" + FS + "class-use" + FS + "Foo4.html",
                "&lt;T extends <A HREF=\"../../pkg2/ParamTest2.html\" title=\"class in pkg2\">ParamTest2</A>&lt;java.util.List&lt;? extends <A HREF=\"../../pkg2/Foo4.html\" title=\"class in pkg2\">Foo4</A>&gt;&gt;&gt;"
            },

            //Type parameters in constructor and method args
            {BUG_ID + FS + "pkg2" + FS + "class-use" + FS + "Foo4.html",
                "<TH ALIGN=\"left\" COLSPAN=\"2\">Method parameters in <A HREF=\"../../pkg2/package-summary.html\">pkg2</A> with type arguments of type <A HREF=\"../../pkg2/Foo4.html\" title=\"class in pkg2\">Foo4</A></FONT></TH>" + NL +
                "</TR>" + NL +
                "<TR BGCOLOR=\"white\" CLASS=\"TableRowColor\">" + NL +
                "<TD ALIGN=\"right\" VALIGN=\"top\" WIDTH=\"1%\"><FONT SIZE=\"-1\">" + NL +
                "<CODE>&nbsp;void</CODE></FONT></TD>" + NL +
                "<TD><CODE><B>ClassUseTest3.</B><B><A HREF=\"../../pkg2/ClassUseTest3.html#method(java.util.Set)\">method</A></B>(java.util.Set&lt;<A HREF=\"../../pkg2/Foo4.html\" title=\"class in pkg2\">Foo4</A>&gt;&nbsp;p)</CODE>"
            },
            {BUG_ID + FS + "pkg2" + FS + "class-use" + FS + "Foo4.html",
                "<TH ALIGN=\"left\" COLSPAN=\"2\">Constructor parameters in <A HREF=\"../../pkg2/package-summary.html\">pkg2</A> with type arguments of type <A HREF=\"../../pkg2/Foo4.html\" title=\"class in pkg2\">Foo4</A></FONT></TH>" + NL +
                "</TR>" + NL +
                "<TR BGCOLOR=\"white\" CLASS=\"TableRowColor\">" + NL +
                "<TD><CODE><B><A HREF=\"../../pkg2/ClassUseTest3.html#ClassUseTest3(java.util.Set)\">ClassUseTest3</A></B>(java.util.Set&lt;<A HREF=\"../../pkg2/Foo4.html\" title=\"class in pkg2\">Foo4</A>&gt;&nbsp;p)</CODE>"
            },

            //=================================
            // Annotatation Type Usage
            //=================================
            {BUG_ID + FS + "pkg" + FS + "class-use" + FS + "AnnotationType.html",
                "<FONT SIZE=\"+2\">" + NL +
                "Packages with annotations of type <A HREF=\"../../pkg/AnnotationType.html\" title=\"annotation in pkg\">AnnotationType</A></FONT></TH>" + NL +
                "</TR>" + NL +
                "<TR BGCOLOR=\"white\" CLASS=\"TableRowColor\">" + NL +
                "<TD><A HREF=\"../../pkg/package-summary.html\"><B>pkg</B></A></TD>"
            },

            {BUG_ID + FS + "pkg" + FS + "class-use" + FS + "AnnotationType.html",
                "Classes in <A HREF=\"../../pkg/package-summary.html\">pkg</A> with annotations of type <A HREF=\"../../pkg/AnnotationType.html\" title=\"annotation in pkg\">AnnotationType</A></FONT></TH>" + NL +
                "</TR>" + NL +
                "<TR BGCOLOR=\"white\" CLASS=\"TableRowColor\">" + NL +
                "<TD ALIGN=\"right\" VALIGN=\"top\" WIDTH=\"1%\"><FONT SIZE=\"-1\">" + NL +
                "<CODE>&nbsp;class</CODE></FONT></TD>" + NL +
                "<TD><CODE><B><A HREF=\"../../pkg/AnnotationTypeUsage.html\" title=\"class in pkg\">AnnotationTypeUsage</A></B></CODE>"
            },

            {BUG_ID + FS + "pkg" + FS + "class-use" + FS + "AnnotationType.html",
                "Fields in <A HREF=\"../../pkg/package-summary.html\">pkg</A> with annotations of type <A HREF=\"../../pkg/AnnotationType.html\" title=\"annotation in pkg\">AnnotationType</A></FONT></TH>" + NL +
                "</TR>" + NL +
                "<TR BGCOLOR=\"white\" CLASS=\"TableRowColor\">" + NL +
                "<TD ALIGN=\"right\" VALIGN=\"top\" WIDTH=\"1%\"><FONT SIZE=\"-1\">" + NL +
                "<CODE>&nbsp;int</CODE></FONT></TD>" + NL +
                "<TD><CODE><B>AnnotationTypeUsage.</B><B><A HREF=\"../../pkg/AnnotationTypeUsage.html#field\">field</A></B></CODE>"
            },

            {BUG_ID + FS + "pkg" + FS + "class-use" + FS + "AnnotationType.html",
                "Methods in <A HREF=\"../../pkg/package-summary.html\">pkg</A> with annotations of type <A HREF=\"../../pkg/AnnotationType.html\" title=\"annotation in pkg\">AnnotationType</A></FONT></TH>" + NL +
                "</TR>" + NL +
                "<TR BGCOLOR=\"white\" CLASS=\"TableRowColor\">" + NL +
                "<TD ALIGN=\"right\" VALIGN=\"top\" WIDTH=\"1%\"><FONT SIZE=\"-1\">" + NL +
                "<CODE>&nbsp;void</CODE></FONT></TD>" + NL +
                "<TD><CODE><B>AnnotationTypeUsage.</B><B><A HREF=\"../../pkg/AnnotationTypeUsage.html#method()\">method</A></B>()</CODE>"
            },

            {BUG_ID + FS + "pkg" + FS + "class-use" + FS + "AnnotationType.html",
                "Method parameters in <A HREF=\"../../pkg/package-summary.html\">pkg</A> with annotations of type <A HREF=\"../../pkg/AnnotationType.html\" title=\"annotation in pkg\">AnnotationType</A></FONT></TH>" + NL +
                "</TR>" + NL +
                "<TR BGCOLOR=\"white\" CLASS=\"TableRowColor\">" + NL +
                "<TD ALIGN=\"right\" VALIGN=\"top\" WIDTH=\"1%\"><FONT SIZE=\"-1\">" + NL +
                "<CODE>&nbsp;void</CODE></FONT></TD>" + NL +
                "<TD><CODE><B>AnnotationTypeUsage.</B><B><A HREF=\"../../pkg/AnnotationTypeUsage.html#methodWithParams(int, int)\">methodWithParams</A></B>(int&nbsp;documented," + NL +
                "                 int&nbsp;undocmented)</CODE>"
            },

            {BUG_ID + FS + "pkg" + FS + "class-use" + FS + "AnnotationType.html",
                "Constructors in <A HREF=\"../../pkg/package-summary.html\">pkg</A> with annotations of type <A HREF=\"../../pkg/AnnotationType.html\" title=\"annotation in pkg\">AnnotationType</A></FONT></TH>" + NL +
                "</TR>" + NL +
                "<TR BGCOLOR=\"white\" CLASS=\"TableRowColor\">" + NL +
                "<TD><CODE><B><A HREF=\"../../pkg/AnnotationTypeUsage.html#AnnotationTypeUsage()\">AnnotationTypeUsage</A></B>()</CODE>"
            },

            {BUG_ID + FS + "pkg" + FS + "class-use" + FS + "AnnotationType.html",
                "Constructor parameters in <A HREF=\"../../pkg/package-summary.html\">pkg</A> with annotations of type <A HREF=\"../../pkg/AnnotationType.html\" title=\"annotation in pkg\">AnnotationType</A></FONT></TH>" + NL +
                "</TR>" + NL +
                "<TR BGCOLOR=\"white\" CLASS=\"TableRowColor\">" + NL +
                "<TD><CODE><B><A HREF=\"../../pkg/AnnotationTypeUsage.html#AnnotationTypeUsage(int, int)\">AnnotationTypeUsage</A></B>(int&nbsp;documented," + NL +
                "                    int&nbsp;undocmented)</CODE>"
            },

            //=================================
            // TYPE PARAMETER IN INDEX
            //=================================
            {BUG_ID + FS + "index-all.html",
                "<A HREF=\"./pkg2/Foo.html#method(java.util.Vector)\"><B>method(Vector&lt;Object&gt;)</B></A>"
            },
            //=================================
            // TYPE PARAMETER IN INDEX
            //=================================
            {BUG_ID + FS + "index-all.html",
                "<A HREF=\"./pkg2/Foo.html#method(java.util.Vector)\"><B>method(Vector&lt;Object&gt;)</B></A>"
            },
        };
    private static final String[][] NEGATED_TEST = {
        //=================================
        // ENUM TESTING
        //=================================
        //NO constructor section
        {BUG_ID + FS + "pkg" + FS + "Coin.html", "<B>Constructor Summary</B>"},
        //=================================
        // TYPE PARAMETER TESTING
        //=================================
        //No type parameters in class frame.
        {BUG_ID + FS + "allclasses-frame.html",
            "<A HREF=\"../pkg/TypeParameters.html\" title=\"class in pkg\">" +
                    "TypeParameters</A>&lt;<A HREF=\"../pkg/TypeParameters.html\" " +
                    "title=\"type parameter in TypeParameters\">E</A>&gt;"
        },

        //==============================================================
        // ANNOTATION TYPE USAGE TESTING (When @Documented is omitted)
        //===============================================================

        //CLASS
        {BUG_ID + FS + "pkg" + FS + "AnnotationTypeUsage.html",
            "<FONT SIZE=\"-1\">" + NL +
            "<A HREF=\"../pkg/AnnotationTypeUndocumented.html\" title=\"annotation in pkg\">@AnnotationTypeUndocumented</A>(<A HREF=\"../pkg/AnnotationType.html#optional\">optional</A>=\"Class Annotation\"," + NL +
            "                <A HREF=\"../pkg/AnnotationType.html#required\">required</A>=1994)" + NL +
            "</FONT>public class <B>AnnotationTypeUsage</B><DT>extends java.lang.Object</DL>"},

        //FIELD
        {BUG_ID + FS + "pkg" + FS + "AnnotationTypeUsage.html",
            "<FONT SIZE=\"-1\">" + NL +
            "<A HREF=\"../pkg/AnnotationTypeUndocumented.html\" title=\"annotation in pkg\">@AnnotationTypeUndocumented</A>(<A HREF=\"../pkg/AnnotationType.html#optional\">optional</A>=\"Field Annotation\"," + NL +
            "                <A HREF=\"../pkg/AnnotationType.html#required\">required</A>=1994)" + NL +
            "</FONT>public int <B>field</B>"},

        //CONSTRUCTOR
        {BUG_ID + FS + "pkg" + FS + "AnnotationTypeUsage.html",
            "<FONT SIZE=\"-1\">" + NL +
            "<A HREF=\"../pkg/AnnotationTypeUndocumented.html\" title=\"annotation in pkg\">@AnnotationTypeUndocumented</A>(<A HREF=\"../pkg/AnnotationType.html#optional\">optional</A>=\"Constructor Annotation\"," + NL +
            "                <A HREF=\"../pkg/AnnotationType.html#required\">required</A>=1994)" + NL +
            "</FONT>public <B>AnnotationTypeUsage</B>()"},

        //METHOD
        {BUG_ID + FS + "pkg" + FS + "AnnotationTypeUsage.html",
            "<FONT SIZE=\"-1\">" + NL +
            "<A HREF=\"../pkg/AnnotationTypeUndocumented.html\" title=\"annotation in pkg\">@AnnotationTypeUndocumented</A>(<A HREF=\"../pkg/AnnotationType.html#optional\">optional</A>=\"Method Annotation\"," + NL +
            "                <A HREF=\"../pkg/AnnotationType.html#required\">required</A>=1994)" + NL +
            "</FONT>public void <B>method</B>()"},

        //=================================
        // Make sure annotation types do not
        // trigger this warning.
        //=================================
        {WARNING_OUTPUT,
            "Internal error: package sets don't match: [] with: null"
        },
    };

    /**
     * The entry point of the test.
     * @param args the array of command line arguments.
     */
    public static void main(String[] args) {
        TestNewLanguageFeatures tester = new TestNewLanguageFeatures();
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
