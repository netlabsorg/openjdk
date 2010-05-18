/*
 * Copyright 2002 Sun Microsystems, Inc.  All Rights Reserved.
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

///////////// Locking verification specific to CMS //////////////
// Much like "assert_lock_strong()", except
// that it relaxes the assertion somewhat for the parallel GC case, where
// main GC thread or the CMS thread might hold the lock on behalf of
// the parallel threads.
class CMSLockVerifier: AllStatic {
 public:
  static void assert_locked(const Mutex* lock, const Mutex* p_lock)
    PRODUCT_RETURN;
  static void assert_locked(const Mutex* lock) {
    assert_locked(lock, NULL);
  }
};
