/*
 * Copyright 2003 Sun Microsystems, Inc.  All Rights Reserved.
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

#ifndef _SYMTAB_H_
#define _SYMTAB_H_

#include <stdint.h>

// interface to manage ELF symbol tables

struct symtab;

// build symbol table for a given ELF file descriptor
struct symtab* build_symtab(int fd);

// destroy the symbol table
void destroy_symtab(struct symtab* symtab);

// search for symbol in the given symbol table. Adds offset
// to the base uintptr_t supplied. Returns NULL if not found.
uintptr_t search_symbol(struct symtab* symtab, uintptr_t base,
                      const char *sym_name, int *sym_size);

// look for nearest symbol for a given offset (not address - base
// subtraction done by caller
const char* nearest_symbol(struct symtab* symtab, uintptr_t offset,
                      uintptr_t* poffset);

#endif /*_SYMTAB_H_*/
