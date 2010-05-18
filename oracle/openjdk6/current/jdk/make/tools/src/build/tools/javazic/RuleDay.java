/*
 * Copyright 2000-2004 Sun Microsystems, Inc.  All Rights Reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * This code is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 only, as
 * published by the Free Software Foundation.  Sun designates this
 * particular file as subject to the "Classpath" exception as provided
 * by Sun in the LICENSE file that accompanied this code.
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

package build.tools.javazic;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

/**
 * RuleDay class represents the value of the "ON" field.  The day of
 * week values start from 1 following the {@link java.util.Calendar}
 * convention.
 *
 * @since 1.4
 */
class RuleDay {
    private static final Map<String,DayOfWeek> abbreviations = new HashMap<String,DayOfWeek>(7);
    static {
        for (DayOfWeek day : DayOfWeek.values()) {
            abbreviations.put(day.getAbbr(), day);
        }
    }

    private String dayName = null;
    private DayOfWeek dow;
    private boolean lastOne = false;
    private int soonerOrLater = 0;
    private int thanDayOfMonth; // day of month (e.g., 8 for "Sun>=8")

    RuleDay() {
    }

    RuleDay(int day) {
        thanDayOfMonth = day;
    }

    int getDay() {
        return thanDayOfMonth;
    }

    /**
     * @return the day of week value (1-based)
     */
    int getDayOfWeekNum() {
        return dow.value();
    }

    /**
     * @return true if this rule day represents the last day of
     * week. (e.g., lastSun).
     */
    boolean isLast() {
        return lastOne;
    }

    /**
     * @return true if this rule day represents the day of week on or
     * later than (after) the {@link #getDay}. (e.g., Sun>=1)
     */
    boolean isLater() {
        return soonerOrLater > 0;
    }

    /**
     * @return true if this rule day represents the day of week on or
     * earlier than (before) the {@link #getDay}. (e.g., Sun<=15)
     */
    boolean isEarlier() {
        return soonerOrLater < 0;
    }

    /**
     * @return true if this rule day represents an exact day.
     */
    boolean isExact() {
        return soonerOrLater == 0;
    }

    /**
     * Parses the "ON" field and constructs a RuleDay.
     * @param day an "ON" field string (e.g., "Sun>=1")
     * @return a RuleDay representing the given "ON" field
     */
    static RuleDay parse(String day) {
        RuleDay d = new RuleDay();
        if (day.startsWith("last")) {
            d.lastOne = true;
            d.dayName = day.substring(4);
            d.dow = getDOW(d.dayName);
        } else {
            int index;
            if ((index = day.indexOf(">=")) != -1) {
                d.dayName = day.substring(0, index);
                d.dow = getDOW(d.dayName);
                d.soonerOrLater = 1; // greater or equal
                d.thanDayOfMonth = Integer.parseInt(day.substring(index+2));
            } else if ((index = day.indexOf("<=")) != -1) {
                d.dayName = day.substring(0, index);
                d.dow = getDOW(d.dayName);
                d.soonerOrLater = -1; // less or equal
                d.thanDayOfMonth = Integer.parseInt(day.substring(index+2));
            } else {
                // it should be an integer value.
                d.thanDayOfMonth = Integer.parseInt(day);
            }
        }
        return d;
    }

    /**
     * Converts this RuleDay to the SimpleTimeZone day rule.
     * @return the converted SimpleTimeZone day rule
     */
    int getDayForSimpleTimeZone() {
        if (isLast()) {
            return -1;
        }
        return getDay();
    }

    /**
     * Converts this RuleDay to the SimpleTimeZone day-of-week rule.
     * @return the SimpleTimeZone day-of-week rule value
     */
    int getDayOfWeekForSimpleTimeZoneInt() {
        if (!isLater() && !isEarlier() && !isLast()) {
            return 0;
        }
        if (isLater()) {
            return -getDayOfWeekNum();
        }
        return getDayOfWeekNum();
    }

    /**
     * @return the string representation of the {@link
     * #getDayOfWeekForSimpleTimeZoneInt} value
     */
    String getDayOfWeekForSimpleTimeZone() {
        int d = getDayOfWeekForSimpleTimeZoneInt();
        if (d == 0) {
            return "0";
        }
        String sign = "";
        if (d < 0) {
            sign = "-";
            d = -d;
        }
        return sign + toString(d);
    }

    private static DayOfWeek getDOW(String abbr) {
        return abbreviations.get(abbr);
    }

    /**
     * Converts the specified day of week value to the day-of-week
     * name defined in {@link java.util.Calenda}.
     * @param dow 1-based day of week value
     * @return the Calendar day of week name with "Calendar." prefix.
     * @throws IllegalArgumentException if the specified dow value is out of range.
     */
    static String toString(int dow) {
        if (dow >= DayOfWeek.SUNDAY.value() && dow <= DayOfWeek.SATURDAY.value()) {
            return "Calendar." + DayOfWeek.values()[dow - 1];
        }
        throw new IllegalArgumentException("wrong Day_of_Week number: " + dow);
    }
}
