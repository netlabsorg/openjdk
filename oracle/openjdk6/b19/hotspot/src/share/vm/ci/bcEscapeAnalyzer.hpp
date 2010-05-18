/*
 * Copyright 2005-2008 Sun Microsystems, Inc.  All Rights Reserved.
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

define_array(ciObjectArray, ciObject*);
define_stack(ciObjectList, ciObjectArray);

// This class implements a fast, conservative analysis of effect of methods
// on the escape state of their arguments.  The analysis is at the bytecode
// level.

class  ciMethodBlocks;
class  ciBlock;

class BCEscapeAnalyzer : public ResourceObj {
 private:
  bool              _conservative; // If true, return maximally
                                   // conservative results.
  ciMethod*         _method;
  ciMethodData*     _methodData;
  int               _arg_size;

  intStack          _stack;

  BitMap            _arg_local;
  BitMap            _arg_stack;
  BitMap            _arg_returned;
  BitMap            _dirty;
  enum{ ARG_OFFSET_MAX = 31};
  uint              *_arg_modified;

  bool              _return_local;
  bool              _return_allocated;
  bool              _allocated_escapes;
  bool              _unknown_modified;

  ciObjectList     _dependencies;

  ciMethodBlocks   *_methodBlocks;

  BCEscapeAnalyzer* _parent;
  int               _level;

  class  ArgumentMap;
  class  StateInfo;

  // helper functions
  bool is_argument(int i)    { return i >= 0 && i < _arg_size; }

  void raw_push(int i)       { _stack.push(i); }
  int  raw_pop()             { return _stack.is_empty() ? -1 : _stack.pop(); }
  void apush(int i)          { raw_push(i); }
  void spush()               { raw_push(-1); }
  void lpush()               { spush(); spush(); }
  int  apop()                { return raw_pop(); }
  void spop()                { assert(_stack.is_empty() || _stack.top() == -1, ""); raw_pop(); }
  void lpop()                { spop(); spop(); }

  void set_returned(ArgumentMap vars);
  bool is_argument(ArgumentMap vars);
  bool is_arg_stack(ArgumentMap vars);
  void clear_bits(ArgumentMap vars, BitMap &bs);
  void set_method_escape(ArgumentMap vars);
  void set_global_escape(ArgumentMap vars);
  void set_dirty(ArgumentMap vars);
  void set_modified(ArgumentMap vars, int offs, int size);

  bool is_recursive_call(ciMethod* callee);
  void add_dependence(ciKlass *klass, ciMethod *meth);
  void propagate_dependencies(ciMethod *meth);
  void invoke(StateInfo &state, Bytecodes::Code code, ciMethod* target, ciKlass* holder);

  void iterate_one_block(ciBlock *blk, StateInfo &state, GrowableArray<ciBlock *> &successors);
  void iterate_blocks(Arena *);
  void merge_block_states(StateInfo *blockstates, ciBlock *dest, StateInfo *s_state);

  // analysis
  void initialize();
  void clear_escape_info();
  void compute_escape_info();
  vmIntrinsics::ID known_intrinsic();
  bool compute_escape_for_intrinsic(vmIntrinsics::ID iid);
  bool do_analysis();

  void read_escape_info();

  bool contains(uint arg_set1, uint arg_set2);

 public:
  BCEscapeAnalyzer(ciMethod* method, BCEscapeAnalyzer* parent = NULL);

  // accessors
  ciMethod*         method() const               { return _method; }
  ciMethodData*     methodData() const           { return _methodData; }
  BCEscapeAnalyzer* parent() const               { return _parent; }
  int               level() const                { return _level; }
  ciObjectList*     dependencies()               { return &_dependencies; }
  bool              has_dependencies() const     { return !_dependencies.is_empty(); }

  // retrieval of interprocedural escape information

  // The given argument does not escape the callee.
  bool is_arg_local(int i) const {
    return !_conservative && _arg_local.at(i);
  }

  // The given argument escapes the callee, but does not become globally
  // reachable.
  bool is_arg_stack(int i) const {
    return !_conservative && _arg_stack.at(i);
  }

  // The given argument does not escape globally, and may be returned.
  bool is_arg_returned(int i) const {
    return !_conservative && _arg_returned.at(i); }

  // True iff only input arguments are returned.
  bool is_return_local() const {
    return !_conservative && _return_local;
  }

  // True iff only newly allocated unescaped objects are returned.
  bool is_return_allocated() const {
    return !_conservative && _return_allocated && !_allocated_escapes;
  }

  // Tracking of argument modification

  enum {OFFSET_ANY = -1};
  bool is_arg_modified(int arg, int offset, int size_in_bytes);
  void set_arg_modified(int arg, int offset, int size_in_bytes);
  bool has_non_arg_side_affects()    { return _unknown_modified; }

  // Copy dependencies from this analysis into "deps"
  void copy_dependencies(Dependencies *deps);

#ifndef PRODUCT
  // dump escape information
  void dump();
#endif
};
