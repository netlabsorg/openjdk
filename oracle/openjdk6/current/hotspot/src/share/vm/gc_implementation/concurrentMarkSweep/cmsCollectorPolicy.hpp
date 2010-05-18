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
 *
 */

class ConcurrentMarkSweepPolicy : public TwoGenerationCollectorPolicy {
 protected:
  void initialize_generations();

 public:
  ConcurrentMarkSweepPolicy();

  ConcurrentMarkSweepPolicy* as_concurrent_mark_sweep_policy() { return this; }

  void initialize_gc_policy_counters();
#if 1
  virtual void initialize_size_policy(size_t init_eden_size,
                                      size_t init_promo_size,
                                      size_t init_survivor_size);
#endif

  // Returns true if the incremental mode is enabled.
  virtual bool has_soft_ended_eden();
};

class ASConcurrentMarkSweepPolicy : public ConcurrentMarkSweepPolicy {
 public:

  // Initialize the jstat counters.  This method requires a
  // size policy.  The size policy is expected to be created
  // after the generations are fully initialized so the
  // initialization of the counters need to be done post
  // the initialization of the generations.
  void initialize_gc_policy_counters();

  virtual CollectorPolicy::Name kind() {
    return CollectorPolicy::ASConcurrentMarkSweepPolicyKind;
  }
};
