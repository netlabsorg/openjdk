/*
 * Copyright 2008 Sun Microsystems, Inc.  All Rights Reserved.
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
 *
 */
package com.sun.hotspot.igv.data;

import java.io.Serializable;

/**
 *
 * @author Thomas Wuerthinger
 */
public class Property implements Serializable {

    public static final long serialVersionUID = 1L;

    private String name;
    private String value;

    private Property() {
        this(null, null);
    }

    private Property(Property p) {
        this(p.getName(), p.getValue());
    }

    private Property(String name) {
        this(name, null);
    }

    public Property(String name, String value) {
        this.name = name;
        this.value = value;
    }

    public String getName() {
        return name;
    }

    public String getValue() {
        return value;
    }

    @Override
    public String toString() {
        return name + " = " + value + "; ";
    }

    @Override
    public boolean equals(Object o) {
        if (!(o instanceof Property)) return false;
        Property p2 = (Property)o;
        return name.equals(p2.name) && value.equals(p2.value);
    }
    @Override
    public int hashCode() {
        return name.hashCode() + value == null ? 0 : value.hashCode();
    }
}
