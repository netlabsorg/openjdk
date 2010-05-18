/*
 * Copyright 1999-2007 Sun Microsystems, Inc.  All Rights Reserved.
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

#include "incls/_precompiled.incl"
#include "incls/_ciSymbol.cpp.incl"

// ------------------------------------------------------------------
// ciSymbol::ciSymbol
//
// Preallocated handle variant.  Used with handles from vmSymboHandles.
ciSymbol::ciSymbol(symbolHandle h_s) : ciObject(h_s) {
}

// ciSymbol
//
// This class represents a symbolOop in the HotSpot virtual
// machine.

// ------------------------------------------------------------------
// ciSymbol::as_utf8
//
// The text of the symbol as a null-terminated C string.
const char* ciSymbol::as_utf8() {
  VM_QUICK_ENTRY_MARK;
  symbolOop s = get_symbolOop();
  return s->as_utf8();
}

// ------------------------------------------------------------------
// ciSymbol::base
jbyte* ciSymbol::base() {
  GUARDED_VM_ENTRY(return get_symbolOop()->base();)
}

// ------------------------------------------------------------------
// ciSymbol::byte_at
int ciSymbol::byte_at(int i) {
  GUARDED_VM_ENTRY(return get_symbolOop()->byte_at(i);)
}

// ------------------------------------------------------------------
// ciSymbol::utf8_length
int ciSymbol::utf8_length() {
  GUARDED_VM_ENTRY(return get_symbolOop()->utf8_length();)
}

// ------------------------------------------------------------------
// ciSymbol::print_impl
//
// Implementation of the print method
void ciSymbol::print_impl(outputStream* st) {
  st->print(" value=");
  print_symbol_on(st);
}

// ------------------------------------------------------------------
// ciSymbol::print_symbol_on
//
// Print the value of this symbol on an outputStream
void ciSymbol::print_symbol_on(outputStream *st) {
  GUARDED_VM_ENTRY(get_symbolOop()->print_symbol_on(st);)
}

// ------------------------------------------------------------------
// ciSymbol::make_impl
//
// Make a ciSymbol from a C string (implementation).
ciSymbol* ciSymbol::make_impl(const char* s) {
  EXCEPTION_CONTEXT;
  // For some reason, oopFactory::new_symbol doesn't declare its
  // char* argument as const.
  symbolOop sym = oopFactory::new_symbol((char*)s, (int)strlen(s), THREAD);
  if (HAS_PENDING_EXCEPTION) {
    CLEAR_PENDING_EXCEPTION;
    CURRENT_THREAD_ENV->record_out_of_memory_failure();
    return ciEnv::_unloaded_cisymbol;
  }
  return CURRENT_THREAD_ENV->get_object(sym)->as_symbol();
}

// ------------------------------------------------------------------
// ciSymbol::make
//
// Make a ciSymbol from a C string.
ciSymbol* ciSymbol::make(const char* s) {
  GUARDED_VM_ENTRY(return make_impl(s);)
}
