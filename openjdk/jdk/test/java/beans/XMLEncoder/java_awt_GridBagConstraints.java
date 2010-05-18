/*
 * Copyright 2006-2007 Sun Microsystems, Inc.  All Rights Reserved.
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
 * @bug 6402062
 * @summary Tests GridBagConstraints encoding
 * @author Sergey Malenkov
 */

import java.awt.GridBagConstraints;

public final class java_awt_GridBagConstraints extends AbstractTest<GridBagConstraints> {
    public static void main(String[] args) {
        new java_awt_GridBagConstraints().test(true);
    }

    protected GridBagConstraints getObject() {
        GridBagConstraints gbc = new GridBagConstraints();
        gbc.gridx = 1;
        gbc.gridy = 2;
        gbc.gridwidth = 3;
        gbc.gridheight = 4;
        gbc.weightx = 0.1;
        gbc.weighty = 0.2;
        gbc.anchor = GridBagConstraints.NORTH;
        gbc.fill = GridBagConstraints.VERTICAL;
        gbc.insets.top = 1;
        gbc.insets.left = 2;
        gbc.insets.right = 3;
        gbc.insets.bottom = 4;
        gbc.ipadx = -1;
        gbc.ipady = -2;
        return gbc;
    }

    protected GridBagConstraints getAnotherObject() {
        return null; // TODO: could not update property
        // return new GridBagConstraints();
    }
}
