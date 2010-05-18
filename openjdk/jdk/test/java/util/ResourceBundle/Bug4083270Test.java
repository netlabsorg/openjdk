/*
 * Copyright (c) 2007 Sun Microsystems, Inc.  All Rights Reserved.
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
    @test
    @summary test Bug 4083270
    @run main Bug4083270Test
    @bug 4083270
*/
/*
 *
 *
 * (C) Copyright IBM Corp. 1999 - All Rights Reserved
 *
 * Portions Copyright 2007 by Sun Microsystems, Inc.,
 * 901 San Antonio Road, Palo Alto, California, 94303, U.S.A.
 * All rights reserved.
 *
 * This software is the confidential and proprietary information
 * of Sun Microsystems, Inc. ("Confidential Information").  You
 * shall not disclose such Confidential Information and shall use
 * it only in accordance with the terms of the license agreement
 * you entered into with Sun.
 *
 * The original version of this source code and documentation is
 * copyrighted and owned by IBM. These materials are provided
 * under terms of a License Agreement between IBM and Sun.
 * This technology is protected by multiple US and International
 * patents. This notice and attribution to IBM may not be removed.
 *
 */

import java.util.*;

/*
 * Bug: ResourceBundle.geBundle does not check for subclass of ResourceBundle.
 * This test tries to load a properties file that has a class file with the
 * same name that isn't a subclass of ResourceBundle.  If the properties
 * file is found, that means that getBundle ignored the class file because
 * it wasn't a subclass of ResourceBundle.
 */
public class Bug4083270Test extends RBTestFmwk {
    public static void main(String[] args) throws Exception {
        new Bug4083270Test(true).run(args);
    }

    public Bug4083270Test(boolean dummy) {
    }

    public Bug4083270Test() throws Exception {
        //If we get here, it means getBundle tried to instantiate this
        //class.  It shouldn't since this class does not subclass
        //ResourceBundle.
        errln("ResourceBundle loaded a non-ResourceBundle class");
    }

    public void testRecursiveResourceLoads() throws Exception {
        final String className = getClass().getName();
        try {
            ResourceBundle bundle = ResourceBundle.getBundle(className, Locale.getDefault());
            if (bundle == null) {
                errln("ResourceBundle did not find properties file");
            } else if (!(bundle instanceof PropertyResourceBundle)) {
                errln("ResourceBundle loaded a non-ResourceBundle class");
            }
        } catch (MissingResourceException e) {
            errln("ResourceBundle threw a MissingResourceException");
        }
    }
}
