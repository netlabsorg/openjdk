/*
 * Copyright 2001-2009 Sun Microsystems, Inc.  All Rights Reserved.
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
#include "incls/_concurrentG1Refine.cpp.incl"

// Possible sizes for the card counts cache: odd primes that roughly double in size.
// (See jvmtiTagMap.cpp).
int ConcurrentG1Refine::_cc_cache_sizes[] = {
        16381,    32771,    76831,    150001,   307261,
       614563,  1228891,  2457733,   4915219,  9830479,
     19660831, 39321619, 78643219, 157286461,       -1
  };

ConcurrentG1Refine::ConcurrentG1Refine() :
  _card_counts(NULL), _card_epochs(NULL),
  _n_card_counts(0), _max_n_card_counts(0),
  _cache_size_index(0), _expand_card_counts(false),
  _hot_cache(NULL),
  _def_use_cache(false), _use_cache(false),
  _n_periods(0),
  _threads(NULL), _n_threads(0)
{
  if (G1ConcRefine) {
    _n_threads = (int)thread_num();
    if (_n_threads > 0) {
      _threads = NEW_C_HEAP_ARRAY(ConcurrentG1RefineThread*, _n_threads);
      int worker_id_offset = (int)DirtyCardQueueSet::num_par_ids();
      ConcurrentG1RefineThread *next = NULL;
      for (int i = _n_threads - 1; i >= 0; i--) {
        ConcurrentG1RefineThread* t = new ConcurrentG1RefineThread(this, next, worker_id_offset, i);
        assert(t != NULL, "Conc refine should have been created");
        assert(t->cg1r() == this, "Conc refine thread should refer to this");
        _threads[i] = t;
        next = t;
      }
    }
  }
}

size_t ConcurrentG1Refine::thread_num() {
  if (G1ConcRefine) {
    return (G1ParallelRSetThreads > 0) ? G1ParallelRSetThreads : ParallelGCThreads;
  }
  return 0;
}

void ConcurrentG1Refine::init() {
  if (G1ConcRSLogCacheSize > 0) {
    _g1h = G1CollectedHeap::heap();
    _max_n_card_counts =
      (unsigned) (_g1h->g1_reserved_obj_bytes() >> CardTableModRefBS::card_shift);

    size_t max_card_num = ((size_t)1 << (sizeof(unsigned)*BitsPerByte-1)) - 1;
    guarantee(_max_n_card_counts < max_card_num, "card_num representation");

    int desired = _max_n_card_counts / InitialCacheFraction;
    for (_cache_size_index = 0;
              _cc_cache_sizes[_cache_size_index] >= 0; _cache_size_index++) {
      if (_cc_cache_sizes[_cache_size_index] >= desired) break;
    }
    _cache_size_index = MAX2(0, (_cache_size_index - 1));

    int initial_size = _cc_cache_sizes[_cache_size_index];
    if (initial_size < 0) initial_size = _max_n_card_counts;

    // Make sure we don't go bigger than we will ever need
    _n_card_counts = MIN2((unsigned) initial_size, _max_n_card_counts);

    _card_counts = NEW_C_HEAP_ARRAY(CardCountCacheEntry, _n_card_counts);
    _card_epochs = NEW_C_HEAP_ARRAY(CardEpochCacheEntry, _n_card_counts);

    Copy::fill_to_bytes(&_card_counts[0],
                        _n_card_counts * sizeof(CardCountCacheEntry));
    Copy::fill_to_bytes(&_card_epochs[0], _n_card_counts * sizeof(CardEpochCacheEntry));

    ModRefBarrierSet* bs = _g1h->mr_bs();
    guarantee(bs->is_a(BarrierSet::CardTableModRef), "Precondition");
    _ct_bs = (CardTableModRefBS*)bs;
    _ct_bot = _ct_bs->byte_for_const(_g1h->reserved_region().start());

    _def_use_cache = true;
    _use_cache = true;
    _hot_cache_size = (1 << G1ConcRSLogCacheSize);
    _hot_cache = NEW_C_HEAP_ARRAY(jbyte*, _hot_cache_size);
    _n_hot = 0;
    _hot_cache_idx = 0;

    // For refining the cards in the hot cache in parallel
    int n_workers = (ParallelGCThreads > 0 ?
                        _g1h->workers()->total_workers() : 1);
    _hot_cache_par_chunk_size = MAX2(1, _hot_cache_size / n_workers);
    _hot_cache_par_claimed_idx = 0;
  }
}

void ConcurrentG1Refine::stop() {
  if (_threads != NULL) {
    for (int i = 0; i < _n_threads; i++) {
      _threads[i]->stop();
    }
  }
}

ConcurrentG1Refine::~ConcurrentG1Refine() {
  if (G1ConcRSLogCacheSize > 0) {
    assert(_card_counts != NULL, "Logic");
    FREE_C_HEAP_ARRAY(CardCountCacheEntry, _card_counts);
    assert(_card_epochs != NULL, "Logic");
    FREE_C_HEAP_ARRAY(CardEpochCacheEntry, _card_epochs);
    assert(_hot_cache != NULL, "Logic");
    FREE_C_HEAP_ARRAY(jbyte*, _hot_cache);
  }
  if (_threads != NULL) {
    for (int i = 0; i < _n_threads; i++) {
      delete _threads[i];
    }
    FREE_C_HEAP_ARRAY(ConcurrentG1RefineThread*, _threads);
  }
}

void ConcurrentG1Refine::threads_do(ThreadClosure *tc) {
  if (_threads != NULL) {
    for (int i = 0; i < _n_threads; i++) {
      tc->do_thread(_threads[i]);
    }
  }
}

bool ConcurrentG1Refine::is_young_card(jbyte* card_ptr) {
  HeapWord* start = _ct_bs->addr_for(card_ptr);
  HeapRegion* r = _g1h->heap_region_containing(start);
  if (r != NULL && r->is_young()) {
    return true;
  }
  // This card is not associated with a heap region
  // so can't be young.
  return false;
}

jbyte* ConcurrentG1Refine::add_card_count(jbyte* card_ptr, int* count, bool* defer) {
  unsigned new_card_num = ptr_2_card_num(card_ptr);
  unsigned bucket = hash(new_card_num);
  assert(0 <= bucket && bucket < _n_card_counts, "Bounds");

  CardCountCacheEntry* count_ptr = &_card_counts[bucket];
  CardEpochCacheEntry* epoch_ptr = &_card_epochs[bucket];

  // We have to construct a new entry if we haven't updated the counts
  // during the current period, or if the count was updated for a
  // different card number.
  unsigned int new_epoch = (unsigned int) _n_periods;
  julong new_epoch_entry = make_epoch_entry(new_card_num, new_epoch);

  while (true) {
    // Fetch the previous epoch value
    julong prev_epoch_entry = epoch_ptr->_value;
    julong cas_res;

    if (extract_epoch(prev_epoch_entry) != new_epoch) {
      // This entry has not yet been updated during this period.
      // Note: we update the epoch value atomically to ensure
      // that there is only one winner that updates the cached
      // card_ptr value even though all the refine threads share
      // the same epoch value.

      cas_res = (julong) Atomic::cmpxchg((jlong) new_epoch_entry,
                                         (volatile jlong*)&epoch_ptr->_value,
                                         (jlong) prev_epoch_entry);

      if (cas_res == prev_epoch_entry) {
        // We have successfully won the race to update the
        // epoch and card_num value. Make it look like the
        // count and eviction count were previously cleared.
        count_ptr->_count = 1;
        count_ptr->_evict_count = 0;
        *count = 0;
        // We can defer the processing of card_ptr
        *defer = true;
        return card_ptr;
      }
      // We did not win the race to update the epoch field, so some other
      // thread must have done it. The value that gets returned by CAS
      // should be the new epoch value.
      assert(extract_epoch(cas_res) == new_epoch, "unexpected epoch");
      // We could 'continue' here or just re-read the previous epoch value
      prev_epoch_entry = epoch_ptr->_value;
    }

    // The epoch entry for card_ptr has been updated during this period.
    unsigned old_card_num = extract_card_num(prev_epoch_entry);

    // The card count that will be returned to caller
    *count = count_ptr->_count;

    // Are we updating the count for the same card?
    if (new_card_num == old_card_num) {
      // Same card - just update the count. We could have more than one
      // thread racing to update count for the current card. It should be
      // OK not to use a CAS as the only penalty should be some missed
      // increments of the count which delays identifying the card as "hot".

      if (*count < max_jubyte) count_ptr->_count++;
      // We can defer the processing of card_ptr
      *defer = true;
      return card_ptr;
    }

    // Different card - evict old card info
    if (count_ptr->_evict_count < max_jubyte) count_ptr->_evict_count++;
    if (count_ptr->_evict_count > G1CardCountCacheExpandThreshold) {
      // Trigger a resize the next time we clear
      _expand_card_counts = true;
    }

    cas_res = (julong) Atomic::cmpxchg((jlong) new_epoch_entry,
                                       (volatile jlong*)&epoch_ptr->_value,
                                       (jlong) prev_epoch_entry);

    if (cas_res == prev_epoch_entry) {
      // We successfully updated the card num value in the epoch entry
      count_ptr->_count = 0; // initialize counter for new card num

      // Even though the region containg the card at old_card_num was not
      // in the young list when old_card_num was recorded in the epoch
      // cache it could have been added to the free list and subsequently
      // added to the young list in the intervening time. If the evicted
      // card is in a young region just return the card_ptr and the evicted
      // card will not be cleaned. See CR 6817995.

      jbyte* old_card_ptr = card_num_2_ptr(old_card_num);
      if (is_young_card(old_card_ptr)) {
        *count = 0;
        // We can defer the processing of card_ptr
        *defer = true;
        return card_ptr;
      }

      // We do not want to defer processing of card_ptr in this case
      // (we need to refine old_card_ptr and card_ptr)
      *defer = false;
      return old_card_ptr;
    }
    // Someone else beat us - try again.
  }
}

jbyte* ConcurrentG1Refine::cache_insert(jbyte* card_ptr, bool* defer) {
  int count;
  jbyte* cached_ptr = add_card_count(card_ptr, &count, defer);
  assert(cached_ptr != NULL, "bad cached card ptr");
  assert(!is_young_card(cached_ptr), "shouldn't get a card in young region");

  // The card pointer we obtained from card count cache is not hot
  // so do not store it in the cache; return it for immediate
  // refining.
  if (count < G1ConcRSHotCardLimit) {
    return cached_ptr;
  }

  // Otherwise, the pointer we got from the _card_counts is hot.
  jbyte* res = NULL;
  MutexLockerEx x(HotCardCache_lock, Mutex::_no_safepoint_check_flag);
  if (_n_hot == _hot_cache_size) {
    res = _hot_cache[_hot_cache_idx];
    _n_hot--;
  }
  // Now _n_hot < _hot_cache_size, and we can insert at _hot_cache_idx.
  _hot_cache[_hot_cache_idx] = cached_ptr;
  _hot_cache_idx++;
  if (_hot_cache_idx == _hot_cache_size) _hot_cache_idx = 0;
  _n_hot++;

  if (res != NULL) {
    // Even though the region containg res was not in the young list
    // when it was recorded in the hot cache it could have been added
    // to the free list and subsequently added to the young list in
    // the intervening time. If res is in a young region, return NULL
    // so that res is not cleaned. See CR 6817995.

    if (is_young_card(res)) {
      res = NULL;
    }
  }

  return res;
}

void ConcurrentG1Refine::clean_up_cache(int worker_i, G1RemSet* g1rs) {
  assert(!use_cache(), "cache should be disabled");
  int start_idx;

  while ((start_idx = _hot_cache_par_claimed_idx) < _n_hot) { // read once
    int end_idx = start_idx + _hot_cache_par_chunk_size;

    if (start_idx ==
        Atomic::cmpxchg(end_idx, &_hot_cache_par_claimed_idx, start_idx)) {
      // The current worker has successfully claimed the chunk [start_idx..end_idx)
      end_idx = MIN2(end_idx, _n_hot);
      for (int i = start_idx; i < end_idx; i++) {
        jbyte* entry = _hot_cache[i];
        if (entry != NULL) {
          g1rs->concurrentRefineOneCard(entry, worker_i);
        }
      }
    }
  }
}

void ConcurrentG1Refine::expand_card_count_cache() {
  if (_n_card_counts < _max_n_card_counts) {
    int new_idx = _cache_size_index+1;
    int new_size = _cc_cache_sizes[new_idx];
    if (new_size < 0) new_size = _max_n_card_counts;

    // Make sure we don't go bigger than we will ever need
    new_size = MIN2((unsigned) new_size, _max_n_card_counts);

    // Expand the card count and card epoch tables
    if (new_size > (int)_n_card_counts) {
      // We can just free and allocate a new array as we're
      // not interested in preserving the contents
      assert(_card_counts != NULL, "Logic!");
      assert(_card_epochs != NULL, "Logic!");
      FREE_C_HEAP_ARRAY(CardCountCacheEntry, _card_counts);
      FREE_C_HEAP_ARRAY(CardEpochCacheEntry, _card_epochs);
      _n_card_counts = new_size;
      _card_counts = NEW_C_HEAP_ARRAY(CardCountCacheEntry, _n_card_counts);
      _card_epochs = NEW_C_HEAP_ARRAY(CardEpochCacheEntry, _n_card_counts);
      _cache_size_index = new_idx;
    }
  }
}

void ConcurrentG1Refine::clear_and_record_card_counts() {
  if (G1ConcRSLogCacheSize == 0) return;

#ifndef PRODUCT
  double start = os::elapsedTime();
#endif

  if (_expand_card_counts) {
    expand_card_count_cache();
    _expand_card_counts = false;
    // Only need to clear the epochs.
    Copy::fill_to_bytes(&_card_epochs[0], _n_card_counts * sizeof(CardEpochCacheEntry));
  }

  int this_epoch = (int) _n_periods;
  assert((this_epoch+1) <= max_jint, "to many periods");
  // Update epoch
  _n_periods++;

#ifndef PRODUCT
  double elapsed = os::elapsedTime() - start;
  _g1h->g1_policy()->record_cc_clear_time(elapsed * 1000.0);
#endif
}

void ConcurrentG1Refine::print_worker_threads_on(outputStream* st) const {
  for (int i = 0; i < _n_threads; ++i) {
    _threads[i]->print_on(st);
    st->cr();
  }
}

