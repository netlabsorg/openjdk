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
 *
 */

import java.text.*;
import java.util.*;
import sun.util.*;
import sun.util.resources.*;

public class NumberFormatProviderTest extends ProviderTest {

    com.foo.NumberFormatProviderImpl nfp = new com.foo.NumberFormatProviderImpl();
    List<Locale> availloc = Arrays.asList(NumberFormat.getAvailableLocales());
    List<Locale> providerloc = Arrays.asList(nfp.getAvailableLocales());
    List<Locale> jreloc = Arrays.asList(LocaleData.getAvailableLocales());

    public static void main(String[] s) {
        new NumberFormatProviderTest();
    }

    NumberFormatProviderTest() {
        availableLocalesTest();
        objectValidityTest();
    }

    void availableLocalesTest() {
        Set<Locale> localesFromAPI = new HashSet<Locale>(availloc);
        Set<Locale> localesExpected = new HashSet<Locale>(jreloc);
        localesExpected.addAll(providerloc);
        if (localesFromAPI.equals(localesExpected)) {
            System.out.println("availableLocalesTest passed.");
        } else {
            throw new RuntimeException("availableLocalesTest failed");
        }
    }

    void objectValidityTest() {

        for (Locale target: availloc) {
            // pure JRE implementation
            ResourceBundle rb = LocaleData.getNumberFormatData(target);
            boolean jreSupportsLocale = jreloc.contains(target);

            // JRE string arrays
            String[] jreNumberPatterns = null;
            if (jreSupportsLocale) {
                try {
                    jreNumberPatterns = rb.getStringArray("NumberPatterns");
                } catch (MissingResourceException mre) {}
            }

            // result object
            String resultCur =
                ((DecimalFormat)NumberFormat.getCurrencyInstance(target)).toPattern();
            String resultInt =
                ((DecimalFormat)NumberFormat.getIntegerInstance(target)).toPattern();
            String resultNum =
                ((DecimalFormat)NumberFormat.getNumberInstance(target)).toPattern();
            String resultPer =
                ((DecimalFormat)NumberFormat.getPercentInstance(target)).toPattern();

            // provider's object (if any)
            String providersCur = null;
            String providersInt = null;
            String providersNum = null;
            String providersPer = null;
            if (providerloc.contains(target)) {
                DecimalFormat dfCur = (DecimalFormat)nfp.getCurrencyInstance(target);
                if (dfCur != null) {
                    providersCur = dfCur.toPattern();
                }
                DecimalFormat dfInt = (DecimalFormat)nfp.getIntegerInstance(target);
                if (dfInt != null) {
                    providersInt = dfInt.toPattern();
                }
                DecimalFormat dfNum = (DecimalFormat)nfp.getNumberInstance(target);
                if (dfNum != null) {
                    providersNum = dfNum.toPattern();
                }
                DecimalFormat dfPer = (DecimalFormat)nfp.getPercentInstance(target);
                if (dfPer != null) {
                    providersPer = dfPer.toPattern();
                }
            }

            // JRE's object (if any)
            // note that this totally depends on the current implementation
            String jresCur = null;
            String jresInt = null;
            String jresNum = null;
            String jresPer = null;
            if (jreSupportsLocale) {
                DecimalFormat dfCur = new DecimalFormat(jreNumberPatterns[1],
                    DecimalFormatSymbols.getInstance(target));
                if (dfCur != null) {
                    adjustForCurrencyDefaultFractionDigits(dfCur);
                    jresCur = dfCur.toPattern();
                }
                DecimalFormat dfInt = new DecimalFormat(jreNumberPatterns[0],
                    DecimalFormatSymbols.getInstance(target));
                if (dfInt != null) {
                    dfInt.setMaximumFractionDigits(0);
                    dfInt.setDecimalSeparatorAlwaysShown(false);
                    dfInt.setParseIntegerOnly(true);
                    jresInt = dfInt.toPattern();
                }
                DecimalFormat dfNum = new DecimalFormat(jreNumberPatterns[0],
                    DecimalFormatSymbols.getInstance(target));
                if (dfNum != null) {
                    jresNum = dfNum.toPattern();
                }
                DecimalFormat dfPer = new DecimalFormat(jreNumberPatterns[2],
                    DecimalFormatSymbols.getInstance(target));
                if (dfPer != null) {
                    jresPer = dfPer.toPattern();
                }
            }

            checkValidity(target, jresCur, providersCur, resultCur, jreSupportsLocale);
            checkValidity(target, jresInt, providersInt, resultInt, jreSupportsLocale);
            checkValidity(target, jresNum, providersNum, resultNum, jreSupportsLocale);
            checkValidity(target, jresPer, providersPer, resultPer, jreSupportsLocale);
        }
    }

    /**
     * Adjusts the minimum and maximum fraction digits to values that
     * are reasonable for the currency's default fraction digits.
     */
    void adjustForCurrencyDefaultFractionDigits(DecimalFormat df) {
        DecimalFormatSymbols dfs = df.getDecimalFormatSymbols();
        Currency currency = dfs.getCurrency();
        if (currency == null) {
            try {
                currency = Currency.getInstance(dfs.getInternationalCurrencySymbol());
            } catch (IllegalArgumentException e) {
            }
        }
        if (currency != null) {
            int digits = currency.getDefaultFractionDigits();
            if (digits != -1) {
                int oldMinDigits = df.getMinimumFractionDigits();
                // Common patterns are "#.##", "#.00", "#".
                // Try to adjust all of them in a reasonable way.
                if (oldMinDigits == df.getMaximumFractionDigits()) {
                    df.setMinimumFractionDigits(digits);
                    df.setMaximumFractionDigits(digits);
                } else {
                    df.setMinimumFractionDigits(Math.min(digits, oldMinDigits));
                    df.setMaximumFractionDigits(digits);
                }
            }
        }
    }
}
