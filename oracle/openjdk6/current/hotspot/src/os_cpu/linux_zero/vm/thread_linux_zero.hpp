/*
 * Copyright 2000-2007 Sun Microsystems, Inc.  All Rights Reserved.
 * Copyright 2007, 2008, 2009 Red Hat, Inc.
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

 private:
  ZeroStack  _zero_stack;
  ZeroFrame* _top_zero_frame;

  void pd_initialize() {
    _top_zero_frame = NULL;
  }

 public:
  ZeroStack *zero_stack() {
    return &_zero_stack;
  }

 public:
  ZeroFrame *top_zero_frame() {
    return _top_zero_frame;
  }
  void push_zero_frame(ZeroFrame *frame) {
    *(ZeroFrame **) frame = _top_zero_frame;
    _top_zero_frame = frame;
  }
  void pop_zero_frame() {
    zero_stack()->set_sp((intptr_t *) _top_zero_frame + 1);
    _top_zero_frame = *(ZeroFrame **) _top_zero_frame;
  }

 public:
  static ByteSize zero_stack_offset() {
    return byte_offset_of(JavaThread, _zero_stack);
  }
  static ByteSize top_zero_frame_offset() {
    return byte_offset_of(JavaThread, _top_zero_frame);
  }

 public:
  void record_base_of_stack_pointer() {
    assert(top_zero_frame() == NULL, "junk on stack prior to Java call");
  }
  void set_base_of_stack_pointer(intptr_t* base_sp) {
    assert(base_sp == NULL, "should be");
    assert(top_zero_frame() == NULL, "junk on stack after Java call");
  }

 public:
  void set_last_Java_frame() {
    JavaFrameAnchor *jfa = frame_anchor();
    jfa->set_last_Java_sp((intptr_t *) top_zero_frame());
  }
  void reset_last_Java_frame() {
    JavaFrameAnchor *jfa = frame_anchor();
    jfa->set_last_Java_sp(NULL);
  }

 private:
  frame pd_last_frame() {
    assert(has_last_Java_frame(), "must have last_Java_sp() when suspended");
    return frame(last_Java_sp(), zero_stack()->sp());
  }

 public:
  // Check for pending suspend requests and pending asynchronous
  // exceptions.  There are separate accessors for these, but
  // _suspend_flags is volatile so using them would be unsafe.
  bool has_special_condition_for_native_trans() {
    return _suspend_flags != 0;
  }

 public:
  bool pd_get_top_frame_for_signal_handler(frame* fr_addr,
                                           void* ucontext,
                                           bool isInJava) {
    ShouldNotCallThis();
  }

  // These routines are only used on cpu architectures that
  // have separate register stacks (Itanium).
  static bool register_stack_overflow() { return false; }
  static void enable_register_stack_guard() {}
  static void disable_register_stack_guard() {}
