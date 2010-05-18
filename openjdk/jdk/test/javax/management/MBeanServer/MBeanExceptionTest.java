/*
 * Copyright 2004 Sun Microsystems, Inc.  All Rights Reserved.
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
 * @bug 5035217
 * @summary Test that MBean's RuntimeException is wrapped in
 * RuntimeMBeanException and (for Standard MBeans) that checked exceptions
 * are wrapped in MBeanException
 * @author Eamonn McManus
 * @compile -source 1.4 MBeanExceptionTest.java
 * @run main MBeanExceptionTest
 */

import javax.management.*;

public class MBeanExceptionTest {
    public static void main(String[] args) throws Exception {
        System.out.println("Test that if an MBean throws RuntimeException " +
                           "it is wrapped in RuntimeMBeanException,");
        System.out.println("and if a Standard MBean throws Exception " +
                           "it is wrapped in MBeanException");
        MBeanServer mbs = MBeanServerFactory.newMBeanServer();
        Object standard = new Except();
        ObjectName standardName = new ObjectName(":name=Standard MBean");
        Object standardMBean =
            new StandardMBean(new Except(), ExceptMBean.class);
        ObjectName standardMBeanName =
            new ObjectName(":name=Instance of StandardMBean");
        Object dynamic = new DynamicExcept();
        ObjectName dynamicName = new ObjectName(":name=Dynamic MBean");
        mbs.registerMBean(standard, standardName);
        mbs.registerMBean(standardMBean, standardMBeanName);
        mbs.registerMBean(dynamic, dynamicName);
        int failures = 0;
        failures += test(mbs, standardName, true);
        failures += test(mbs, standardMBeanName, true);
        failures += test(mbs, dynamicName, false);
        if (failures == 0)
            System.out.println("Test passed");
        else {
            System.out.println("TEST FAILED: " + failures + " failure(s)");
            System.exit(1);
        }
    }

    private static int test(MBeanServer mbs, ObjectName name,
                            boolean testChecked)
            throws Exception {
        System.out.println("--------" + name + "--------");

        int failures = 0;
        final String[] ops = {"getAttribute", "setAttribute", "invoke"};
        final int GET = 0, SET = 1, INVOKE = 2;
        final String[] targets = {"UncheckedException", "CheckedException"};
        final int UNCHECKED = 0, CHECKED = 1;

        for (int i = 0; i < ops.length; i++) {
            for (int j = 0; j < targets.length; j++) {

                if (j == CHECKED && !testChecked)
                    continue;

                String target = targets[j];
                String what = ops[i] + "/" + target;
                System.out.println(what);

                try {
                    switch (i) {
                    case GET:
                        mbs.getAttribute(name, target);
                        break;
                    case SET:
                        mbs.setAttribute(name, new Attribute(target, "x"));
                        break;
                    case INVOKE:
                        mbs.invoke(name, target, null, null);
                        break;
                    default:
                        throw new AssertionError();
                    }
                    System.out.println("failure: " + what + " returned!");
                    failures++;
                } catch (RuntimeMBeanException e) {
                    if (j == CHECKED) {
                        System.out.println("failure: RuntimeMBeanException " +
                                           "when checked expected: " + e);
                        failures++;
                    } else {
                        Throwable cause = e.getCause();
                        if (cause == theUncheckedException)
                            System.out.println("ok: " + what);
                        else {
                            System.out.println("failure: " + what +
                                               " wrapped " + cause);
                            failures++;
                        }
                    }
                } catch (MBeanException e) {
                    if (j == UNCHECKED) {
                        System.out.println("failure: checked exception " +
                                           "when unchecked expected: " + e);
                        failures++;
                    } else {
                        Throwable cause = e.getCause();
                        if (cause == theCheckedException)
                            System.out.println("ok: " + what);
                        else {
                            System.out.println("failure: " + what +
                                               " wrapped " + cause);
                            failures++;
                        }
                    }
                } catch (Throwable t) {
                    System.out.println("failure: " + what + " threw: " + t);
                    while ((t = t.getCause()) != null)
                        System.out.println("  ... " + t);
                    failures++;
                }
            }
        }

        return failures;
    }

    public static interface ExceptMBean {
        public String getUncheckedException();
        public void setUncheckedException(String x);
        public void UncheckedException();
        public String getCheckedException() throws Exception;
        public void setCheckedException(String x) throws Exception;
        public void CheckedException() throws Exception;
    }

    public static class Except implements ExceptMBean {
        public String getUncheckedException() {
            throw theUncheckedException;
        }
        public void setUncheckedException(String x) {
            throw theUncheckedException;
        }
        public void UncheckedException() {
            throw theUncheckedException;
        }
        public String getCheckedException() throws Exception {
            throw theCheckedException;
        }
        public void setCheckedException(String x) throws Exception {
            throw theCheckedException;
        }
        public void CheckedException() throws Exception {
            throw theCheckedException;
        }
    }

    public static class DynamicExcept implements DynamicMBean {
        public Object getAttribute(String attrName)
                throws MBeanException {
            if (attrName.equals("UncheckedException"))
                throw theUncheckedException;
            else
                throw new AssertionError();
        }
        public void setAttribute(Attribute attr)
                throws MBeanException {
            String attrName = attr.getName();
            if (attrName.equals("UncheckedException"))
                throw theUncheckedException;
            else
                throw new AssertionError();
        }
        public Object invoke(String opName, Object[] params, String[] sig)
                throws MBeanException {
            assert params == null && sig == null;
            if (opName.equals("UncheckedException"))
                throw theUncheckedException;
            else
                throw new AssertionError();
        }
        public AttributeList getAttributes(String[] names) {
            assert false;
            return null;
        }
        public AttributeList setAttributes(AttributeList attrs) {
            assert false;
            return null;
        }
        public MBeanInfo getMBeanInfo() {
            try {
                return new StandardMBean(new Except(), ExceptMBean.class)
                    .getMBeanInfo();
            } catch (Exception e) {
                assert false;
                return null;
            }
        }
    }

    private static final Exception theCheckedException =
        new Exception("The checked exception that should be seen");
    private static final RuntimeException theUncheckedException =
        new UnsupportedOperationException("The unchecked exception " +
                                          "that should be seen");
}
