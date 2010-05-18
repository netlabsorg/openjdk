/*
 * Copyright 2001-2008 Sun Microsystems, Inc.  All Rights Reserved.
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

class AllocationStats VALUE_OBJ_CLASS_SPEC {
  // A duration threshold (in ms) used to filter
  // possibly unreliable samples.
  static float _threshold;

  // We measure the demand between the end of the previous sweep and
  // beginning of this sweep:
  //   Count(end_last_sweep) - Count(start_this_sweep)
  //     + splitBirths(between) - splitDeaths(between)
  // The above number divided by the time since the start [END???] of the
  // previous sweep gives us a time rate of demand for blocks
  // of this size. We compute a padded average of this rate as
  // our current estimate for the time rate of demand for blocks
  // of this size. Similarly, we keep a padded average for the time
  // between sweeps. Our current estimate for demand for blocks of
  // this size is then simply computed as the product of these two
  // estimates.
  AdaptivePaddedAverage _demand_rate_estimate;

  ssize_t     _desired;          // Estimate computed as described above
  ssize_t     _coalDesired;     // desired +/- small-percent for tuning coalescing

  ssize_t     _surplus;         // count - (desired +/- small-percent),
                                // used to tune splitting in best fit
  ssize_t     _bfrSurp;         // surplus at start of current sweep
  ssize_t     _prevSweep;       // count from end of previous sweep
  ssize_t     _beforeSweep;     // count from before current sweep
  ssize_t     _coalBirths;      // additional chunks from coalescing
  ssize_t     _coalDeaths;      // loss from coalescing
  ssize_t     _splitBirths;     // additional chunks from splitting
  ssize_t     _splitDeaths;     // loss from splitting
  size_t     _returnedBytes;    // number of bytes returned to list.
 public:
  void initialize() {
    AdaptivePaddedAverage* dummy =
      new (&_demand_rate_estimate) AdaptivePaddedAverage(CMS_FLSWeight,
                                                         CMS_FLSPadding);
    _desired = 0;
    _coalDesired = 0;
    _surplus = 0;
    _bfrSurp = 0;
    _prevSweep = 0;
    _beforeSweep = 0;
    _coalBirths = 0;
    _coalDeaths = 0;
    _splitBirths = 0;
    _splitDeaths = 0;
    _returnedBytes = 0;
  }

  AllocationStats() {
    initialize();
  }
  // The rate estimate is in blocks per second.
  void compute_desired(size_t count,
                       float inter_sweep_current,
                       float inter_sweep_estimate) {
    // If the latest inter-sweep time is below our granularity
    // of measurement, we may call in here with
    // inter_sweep_current == 0. However, even for suitably small
    // but non-zero inter-sweep durations, we may not trust the accuracy
    // of accumulated data, since it has not been "integrated"
    // (read "low-pass-filtered") long enough, and would be
    // vulnerable to noisy glitches. In such cases, we
    // ignore the current sample and use currently available
    // historical estimates.
    if (inter_sweep_current > _threshold) {
      ssize_t demand = prevSweep() - count + splitBirths() - splitDeaths();
      float rate = ((float)demand)/inter_sweep_current;
      _demand_rate_estimate.sample(rate);
      _desired = (ssize_t)(_demand_rate_estimate.padded_average()
                           *inter_sweep_estimate);
    }
  }

  ssize_t desired() const { return _desired; }
  void set_desired(ssize_t v) { _desired = v; }

  ssize_t coalDesired() const { return _coalDesired; }
  void set_coalDesired(ssize_t v) { _coalDesired = v; }

  ssize_t surplus() const { return _surplus; }
  void set_surplus(ssize_t v) { _surplus = v; }
  void increment_surplus() { _surplus++; }
  void decrement_surplus() { _surplus--; }

  ssize_t bfrSurp() const { return _bfrSurp; }
  void set_bfrSurp(ssize_t v) { _bfrSurp = v; }
  ssize_t prevSweep() const { return _prevSweep; }
  void set_prevSweep(ssize_t v) { _prevSweep = v; }
  ssize_t beforeSweep() const { return _beforeSweep; }
  void set_beforeSweep(ssize_t v) { _beforeSweep = v; }

  ssize_t coalBirths() const { return _coalBirths; }
  void set_coalBirths(ssize_t v) { _coalBirths = v; }
  void increment_coalBirths() { _coalBirths++; }

  ssize_t coalDeaths() const { return _coalDeaths; }
  void set_coalDeaths(ssize_t v) { _coalDeaths = v; }
  void increment_coalDeaths() { _coalDeaths++; }

  ssize_t splitBirths() const { return _splitBirths; }
  void set_splitBirths(ssize_t v) { _splitBirths = v; }
  void increment_splitBirths() { _splitBirths++; }

  ssize_t splitDeaths() const { return _splitDeaths; }
  void set_splitDeaths(ssize_t v) { _splitDeaths = v; }
  void increment_splitDeaths() { _splitDeaths++; }

  NOT_PRODUCT(
    size_t returnedBytes() const { return _returnedBytes; }
    void set_returnedBytes(size_t v) { _returnedBytes = v; }
  )
};
