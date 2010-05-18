/*
 * Copyright 1997-2005 Sun Microsystems, Inc.  All Rights Reserved.
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

// PcDescs map a physical PC (given as offset from start of nmethod) to
// the corresponding source scope and byte code index.

class nmethod;

class PcDesc VALUE_OBJ_CLASS_SPEC {
  friend class VMStructs;
 private:
  int _pc_offset;           // offset from start of nmethod
  int _scope_decode_offset; // offset for scope in nmethod
  int _obj_decode_offset;

 public:
  int pc_offset() const           { return _pc_offset;   }
  int scope_decode_offset() const { return _scope_decode_offset; }
  int obj_decode_offset() const   { return _obj_decode_offset; }

  void set_pc_offset(int x)           { _pc_offset           = x; }
  void set_scope_decode_offset(int x) { _scope_decode_offset = x; }
  void set_obj_decode_offset(int x)   { _obj_decode_offset   = x; }

  // Constructor (only used for static in nmethod.cpp)
  // Also used by ScopeDesc::sender()]
  PcDesc(int pc_offset, int scope_decode_offset, int obj_decode_offset);

  enum {
    // upper and lower exclusive limits real offsets:
    lower_offset_limit = -1,
    upper_offset_limit = (unsigned int)-1 >> 1
  };

  // Returns the real pc
  address real_pc(const nmethod* code) const;

  void print(nmethod* code);
  bool verify(nmethod* code);
};
