/*
 * Copyright 2002-2009 Sun Microsystems, Inc.  All Rights Reserved.
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
#include "incls/_psPromotionManager.cpp.incl"

PSPromotionManager**         PSPromotionManager::_manager_array = NULL;
OopStarTaskQueueSet*         PSPromotionManager::_stack_array_depth = NULL;
OopTaskQueueSet*             PSPromotionManager::_stack_array_breadth = NULL;
PSOldGen*                    PSPromotionManager::_old_gen = NULL;
MutableSpace*                PSPromotionManager::_young_space = NULL;

void PSPromotionManager::initialize() {
  ParallelScavengeHeap* heap = (ParallelScavengeHeap*)Universe::heap();
  assert(heap->kind() == CollectedHeap::ParallelScavengeHeap, "Sanity");

  _old_gen = heap->old_gen();
  _young_space = heap->young_gen()->to_space();

  assert(_manager_array == NULL, "Attempt to initialize twice");
  _manager_array = NEW_C_HEAP_ARRAY(PSPromotionManager*, ParallelGCThreads+1 );
  guarantee(_manager_array != NULL, "Could not initialize promotion manager");

  if (UseDepthFirstScavengeOrder) {
    _stack_array_depth = new OopStarTaskQueueSet(ParallelGCThreads);
    guarantee(_stack_array_depth != NULL, "Count not initialize promotion manager");
  } else {
    _stack_array_breadth = new OopTaskQueueSet(ParallelGCThreads);
    guarantee(_stack_array_breadth != NULL, "Count not initialize promotion manager");
  }

  // Create and register the PSPromotionManager(s) for the worker threads.
  for(uint i=0; i<ParallelGCThreads; i++) {
    _manager_array[i] = new PSPromotionManager();
    guarantee(_manager_array[i] != NULL, "Could not create PSPromotionManager");
    if (UseDepthFirstScavengeOrder) {
      stack_array_depth()->register_queue(i, _manager_array[i]->claimed_stack_depth());
    } else {
      stack_array_breadth()->register_queue(i, _manager_array[i]->claimed_stack_breadth());
    }
  }

  // The VMThread gets its own PSPromotionManager, which is not available
  // for work stealing.
  _manager_array[ParallelGCThreads] = new PSPromotionManager();
  guarantee(_manager_array[ParallelGCThreads] != NULL, "Could not create PSPromotionManager");
}

PSPromotionManager* PSPromotionManager::gc_thread_promotion_manager(int index) {
  assert(index >= 0 && index < (int)ParallelGCThreads, "index out of range");
  assert(_manager_array != NULL, "Sanity");
  return _manager_array[index];
}

PSPromotionManager* PSPromotionManager::vm_thread_promotion_manager() {
  assert(_manager_array != NULL, "Sanity");
  return _manager_array[ParallelGCThreads];
}

void PSPromotionManager::pre_scavenge() {
  ParallelScavengeHeap* heap = (ParallelScavengeHeap*)Universe::heap();
  assert(heap->kind() == CollectedHeap::ParallelScavengeHeap, "Sanity");

  _young_space = heap->young_gen()->to_space();

  for(uint i=0; i<ParallelGCThreads+1; i++) {
    manager_array(i)->reset();
  }
}

void PSPromotionManager::post_scavenge() {
#if PS_PM_STATS
  print_stats();
#endif // PS_PM_STATS

  for(uint i=0; i<ParallelGCThreads+1; i++) {
    PSPromotionManager* manager = manager_array(i);

    // the guarantees are a bit gratuitous but, if one fires, we'll
    // have a better idea of what went wrong
    if (i < ParallelGCThreads) {
      guarantee((!UseDepthFirstScavengeOrder ||
                 manager->overflow_stack_depth()->length() <= 0),
                "promotion manager overflow stack must be empty");
      guarantee((UseDepthFirstScavengeOrder ||
                 manager->overflow_stack_breadth()->length() <= 0),
                "promotion manager overflow stack must be empty");

      guarantee((!UseDepthFirstScavengeOrder ||
                 manager->claimed_stack_depth()->size() <= 0),
                "promotion manager claimed stack must be empty");
      guarantee((UseDepthFirstScavengeOrder ||
                 manager->claimed_stack_breadth()->size() <= 0),
                "promotion manager claimed stack must be empty");
    } else {
      guarantee((!UseDepthFirstScavengeOrder ||
                 manager->overflow_stack_depth()->length() <= 0),
                "VM Thread promotion manager overflow stack "
                "must be empty");
      guarantee((UseDepthFirstScavengeOrder ||
                 manager->overflow_stack_breadth()->length() <= 0),
                "VM Thread promotion manager overflow stack "
                "must be empty");

      guarantee((!UseDepthFirstScavengeOrder ||
                 manager->claimed_stack_depth()->size() <= 0),
                "VM Thread promotion manager claimed stack "
                "must be empty");
      guarantee((UseDepthFirstScavengeOrder ||
                 manager->claimed_stack_breadth()->size() <= 0),
                "VM Thread promotion manager claimed stack "
                "must be empty");
    }

    manager->flush_labs();
  }
}

#if PS_PM_STATS

void
PSPromotionManager::print_stats(uint i) {
  tty->print_cr("---- GC Worker %2d Stats", i);
  tty->print_cr("    total pushes            %8d", _total_pushes);
  tty->print_cr("    masked pushes           %8d", _masked_pushes);
  tty->print_cr("    overflow pushes         %8d", _overflow_pushes);
  tty->print_cr("    max overflow length     %8d", _max_overflow_length);
  tty->print_cr("");
  tty->print_cr("    arrays chunked          %8d", _arrays_chunked);
  tty->print_cr("    array chunks processed  %8d", _array_chunks_processed);
  tty->print_cr("");
  tty->print_cr("    total steals            %8d", _total_steals);
  tty->print_cr("    masked steals           %8d", _masked_steals);
  tty->print_cr("");
}

void
PSPromotionManager::print_stats() {
  tty->print_cr("== GC Tasks Stats (%s), GC %3d",
                (UseDepthFirstScavengeOrder) ? "Depth-First" : "Breadth-First",
                Universe::heap()->total_collections());

  for (uint i = 0; i < ParallelGCThreads+1; ++i) {
    PSPromotionManager* manager = manager_array(i);
    manager->print_stats(i);
  }
}

#endif // PS_PM_STATS

PSPromotionManager::PSPromotionManager() {
  ParallelScavengeHeap* heap = (ParallelScavengeHeap*)Universe::heap();
  assert(heap->kind() == CollectedHeap::ParallelScavengeHeap, "Sanity");
  _depth_first = UseDepthFirstScavengeOrder;

  // We set the old lab's start array.
  _old_lab.set_start_array(old_gen()->start_array());

  uint queue_size;
  if (depth_first()) {
    claimed_stack_depth()->initialize();
    queue_size = claimed_stack_depth()->max_elems();
    // We want the overflow stack to be permanent
    _overflow_stack_depth = new (ResourceObj::C_HEAP) GrowableArray<StarTask>(10, true);
    _overflow_stack_breadth = NULL;
  } else {
    claimed_stack_breadth()->initialize();
    queue_size = claimed_stack_breadth()->max_elems();
    // We want the overflow stack to be permanent
    _overflow_stack_breadth = new (ResourceObj::C_HEAP) GrowableArray<oop>(10, true);
    _overflow_stack_depth = NULL;
  }

  _totally_drain = (ParallelGCThreads == 1) || (GCDrainStackTargetSize == 0);
  if (_totally_drain) {
    _target_stack_size = 0;
  } else {
    // don't let the target stack size to be more than 1/4 of the entries
    _target_stack_size = (uint) MIN2((uint) GCDrainStackTargetSize,
                                     (uint) (queue_size / 4));
  }

  _array_chunk_size = ParGCArrayScanChunk;
  // let's choose 1.5x the chunk size
  _min_array_size_for_chunking = 3 * _array_chunk_size / 2;

  reset();
}

void PSPromotionManager::reset() {
  assert(claimed_stack_empty(), "reset of non-empty claimed stack");
  assert(overflow_stack_empty(), "reset of non-empty overflow stack");

  // We need to get an assert in here to make sure the labs are always flushed.

  ParallelScavengeHeap* heap = (ParallelScavengeHeap*)Universe::heap();
  assert(heap->kind() == CollectedHeap::ParallelScavengeHeap, "Sanity");

  // Do not prefill the LAB's, save heap wastage!
  HeapWord* lab_base = young_space()->top();
  _young_lab.initialize(MemRegion(lab_base, (size_t)0));
  _young_gen_is_full = false;

  lab_base = old_gen()->object_space()->top();
  _old_lab.initialize(MemRegion(lab_base, (size_t)0));
  _old_gen_is_full = false;

  _prefetch_queue.clear();

#if PS_PM_STATS
  _total_pushes = 0;
  _masked_pushes = 0;
  _overflow_pushes = 0;
  _max_overflow_length = 0;
  _arrays_chunked = 0;
  _array_chunks_processed = 0;
  _total_steals = 0;
  _masked_steals = 0;
#endif // PS_PM_STATS
}


void PSPromotionManager::drain_stacks_depth(bool totally_drain) {
  assert(depth_first(), "invariant");
  assert(overflow_stack_depth() != NULL, "invariant");
  totally_drain = totally_drain || _totally_drain;

#ifdef ASSERT
  ParallelScavengeHeap* heap = (ParallelScavengeHeap*)Universe::heap();
  assert(heap->kind() == CollectedHeap::ParallelScavengeHeap, "Sanity");
  MutableSpace* to_space = heap->young_gen()->to_space();
  MutableSpace* old_space = heap->old_gen()->object_space();
  MutableSpace* perm_space = heap->perm_gen()->object_space();
#endif /* ASSERT */

  do {
    StarTask p;

    // Drain overflow stack first, so other threads can steal from
    // claimed stack while we work.
    while(!overflow_stack_depth()->is_empty()) {
      // linux compiler wants different overloaded operator= in taskqueue to
      // assign to p that the other compilers don't like.
      StarTask ptr = overflow_stack_depth()->pop();
      process_popped_location_depth(ptr);
    }

    if (totally_drain) {
      while (claimed_stack_depth()->pop_local(p)) {
        process_popped_location_depth(p);
      }
    } else {
      while (claimed_stack_depth()->size() > _target_stack_size &&
             claimed_stack_depth()->pop_local(p)) {
        process_popped_location_depth(p);
      }
    }
  } while( (totally_drain && claimed_stack_depth()->size() > 0) ||
           (overflow_stack_depth()->length() > 0) );

  assert(!totally_drain || claimed_stack_empty(), "Sanity");
  assert(totally_drain ||
         claimed_stack_depth()->size() <= _target_stack_size,
         "Sanity");
  assert(overflow_stack_empty(), "Sanity");
}

void PSPromotionManager::drain_stacks_breadth(bool totally_drain) {
  assert(!depth_first(), "invariant");
  assert(overflow_stack_breadth() != NULL, "invariant");
  totally_drain = totally_drain || _totally_drain;

#ifdef ASSERT
  ParallelScavengeHeap* heap = (ParallelScavengeHeap*)Universe::heap();
  assert(heap->kind() == CollectedHeap::ParallelScavengeHeap, "Sanity");
  MutableSpace* to_space = heap->young_gen()->to_space();
  MutableSpace* old_space = heap->old_gen()->object_space();
  MutableSpace* perm_space = heap->perm_gen()->object_space();
#endif /* ASSERT */

  do {
    oop obj;

    // Drain overflow stack first, so other threads can steal from
    // claimed stack while we work.
    while(!overflow_stack_breadth()->is_empty()) {
      obj = overflow_stack_breadth()->pop();
      obj->copy_contents(this);
    }

    if (totally_drain) {
      // obj is a reference!!!
      while (claimed_stack_breadth()->pop_local(obj)) {
        // It would be nice to assert about the type of objects we might
        // pop, but they can come from anywhere, unfortunately.
        obj->copy_contents(this);
      }
    } else {
      // obj is a reference!!!
      while (claimed_stack_breadth()->size() > _target_stack_size &&
             claimed_stack_breadth()->pop_local(obj)) {
        // It would be nice to assert about the type of objects we might
        // pop, but they can come from anywhere, unfortunately.
        obj->copy_contents(this);
      }
    }

    // If we could not find any other work, flush the prefetch queue
    if (claimed_stack_breadth()->size() == 0 &&
        (overflow_stack_breadth()->length() == 0)) {
      flush_prefetch_queue();
    }
  } while((totally_drain && claimed_stack_breadth()->size() > 0) ||
          (overflow_stack_breadth()->length() > 0));

  assert(!totally_drain || claimed_stack_empty(), "Sanity");
  assert(totally_drain ||
         claimed_stack_breadth()->size() <= _target_stack_size,
         "Sanity");
  assert(overflow_stack_empty(), "Sanity");
}

void PSPromotionManager::flush_labs() {
  assert(claimed_stack_empty(), "Attempt to flush lab with live stack");
  assert(overflow_stack_empty(), "Attempt to flush lab with live overflow stack");

  // If either promotion lab fills up, we can flush the
  // lab but not refill it, so check first.
  assert(!_young_lab.is_flushed() || _young_gen_is_full, "Sanity");
  if (!_young_lab.is_flushed())
    _young_lab.flush();

  assert(!_old_lab.is_flushed() || _old_gen_is_full, "Sanity");
  if (!_old_lab.is_flushed())
    _old_lab.flush();

  // Let PSScavenge know if we overflowed
  if (_young_gen_is_full) {
    PSScavenge::set_survivor_overflow(true);
  }
}

//
// This method is pretty bulky. It would be nice to split it up
// into smaller submethods, but we need to be careful not to hurt
// performance.
//

oop PSPromotionManager::copy_to_survivor_space(oop o, bool depth_first) {
  assert(PSScavenge::should_scavenge(&o), "Sanity");

  oop new_obj = NULL;

  // NOTE! We must be very careful with any methods that access the mark
  // in o. There may be multiple threads racing on it, and it may be forwarded
  // at any time. Do not use oop methods for accessing the mark!
  markOop test_mark = o->mark();

  // The same test as "o->is_forwarded()"
  if (!test_mark->is_marked()) {
    bool new_obj_is_tenured = false;
    size_t new_obj_size = o->size();

    // Find the objects age, MT safe.
    int age = (test_mark->has_displaced_mark_helper() /* o->has_displaced_mark() */) ?
      test_mark->displaced_mark_helper()->age() : test_mark->age();

    // Try allocating obj in to-space (unless too old)
    if (age < PSScavenge::tenuring_threshold()) {
      new_obj = (oop) _young_lab.allocate(new_obj_size);
      if (new_obj == NULL && !_young_gen_is_full) {
        // Do we allocate directly, or flush and refill?
        if (new_obj_size > (YoungPLABSize / 2)) {
          // Allocate this object directly
          new_obj = (oop)young_space()->cas_allocate(new_obj_size);
        } else {
          // Flush and fill
          _young_lab.flush();

          HeapWord* lab_base = young_space()->cas_allocate(YoungPLABSize);
          if (lab_base != NULL) {
            _young_lab.initialize(MemRegion(lab_base, YoungPLABSize));
            // Try the young lab allocation again.
            new_obj = (oop) _young_lab.allocate(new_obj_size);
          } else {
            _young_gen_is_full = true;
          }
        }
      }
    }

    // Otherwise try allocating obj tenured
    if (new_obj == NULL) {
#ifndef PRODUCT
      if (Universe::heap()->promotion_should_fail()) {
        return oop_promotion_failed(o, test_mark);
      }
#endif  // #ifndef PRODUCT

      new_obj = (oop) _old_lab.allocate(new_obj_size);
      new_obj_is_tenured = true;

      if (new_obj == NULL) {
        if (!_old_gen_is_full) {
          // Do we allocate directly, or flush and refill?
          if (new_obj_size > (OldPLABSize / 2)) {
            // Allocate this object directly
            new_obj = (oop)old_gen()->cas_allocate(new_obj_size);
          } else {
            // Flush and fill
            _old_lab.flush();

            HeapWord* lab_base = old_gen()->cas_allocate(OldPLABSize);
            if(lab_base != NULL) {
              _old_lab.initialize(MemRegion(lab_base, OldPLABSize));
              // Try the old lab allocation again.
              new_obj = (oop) _old_lab.allocate(new_obj_size);
            }
          }
        }

        // This is the promotion failed test, and code handling.
        // The code belongs here for two reasons. It is slightly
        // different thatn the code below, and cannot share the
        // CAS testing code. Keeping the code here also minimizes
        // the impact on the common case fast path code.

        if (new_obj == NULL) {
          _old_gen_is_full = true;
          return oop_promotion_failed(o, test_mark);
        }
      }
    }

    assert(new_obj != NULL, "allocation should have succeeded");

    // Copy obj
    Copy::aligned_disjoint_words((HeapWord*)o, (HeapWord*)new_obj, new_obj_size);

    // Now we have to CAS in the header.
    if (o->cas_forward_to(new_obj, test_mark)) {
      // We won any races, we "own" this object.
      assert(new_obj == o->forwardee(), "Sanity");

      // Increment age if obj still in new generation. Now that
      // we're dealing with a markOop that cannot change, it is
      // okay to use the non mt safe oop methods.
      if (!new_obj_is_tenured) {
        new_obj->incr_age();
        assert(young_space()->contains(new_obj), "Attempt to push non-promoted obj");
      }

      if (depth_first) {
        // Do the size comparison first with new_obj_size, which we
        // already have. Hopefully, only a few objects are larger than
        // _min_array_size_for_chunking, and most of them will be arrays.
        // So, the is->objArray() test would be very infrequent.
        if (new_obj_size > _min_array_size_for_chunking &&
            new_obj->is_objArray() &&
            PSChunkLargeArrays) {
          // we'll chunk it
#if PS_PM_STATS
          ++_arrays_chunked;
#endif // PS_PM_STATS
          oop* const masked_o = mask_chunked_array_oop(o);
          push_depth(masked_o);
#if PS_PM_STATS
          ++_masked_pushes;
#endif // PS_PM_STATS
        } else {
          // we'll just push its contents
          new_obj->push_contents(this);
        }
      } else {
        push_breadth(new_obj);
      }
    }  else {
      // We lost, someone else "owns" this object
      guarantee(o->is_forwarded(), "Object must be forwarded if the cas failed.");

      // Try to deallocate the space.  If it was directly allocated we cannot
      // deallocate it, so we have to test.  If the deallocation fails,
      // overwrite with a filler object.
      if (new_obj_is_tenured) {
        if (!_old_lab.unallocate_object(new_obj)) {
          CollectedHeap::fill_with_object((HeapWord*) new_obj, new_obj_size);
        }
      } else if (!_young_lab.unallocate_object(new_obj)) {
        CollectedHeap::fill_with_object((HeapWord*) new_obj, new_obj_size);
      }

      // don't update this before the unallocation!
      new_obj = o->forwardee();
    }
  } else {
    assert(o->is_forwarded(), "Sanity");
    new_obj = o->forwardee();
  }

#ifdef DEBUG
  // This code must come after the CAS test, or it will print incorrect
  // information.
  if (TraceScavenge) {
    gclog_or_tty->print_cr("{%s %s " PTR_FORMAT " -> " PTR_FORMAT " (" SIZE_FORMAT ")}",
       PSScavenge::should_scavenge(&new_obj) ? "copying" : "tenuring",
       new_obj->blueprint()->internal_name(), o, new_obj, new_obj->size());
  }
#endif

  return new_obj;
}

template <class T> void PSPromotionManager::process_array_chunk_work(
                                                 oop obj,
                                                 int start, int end) {
  assert(start < end, "invariant");
  T* const base      = (T*)objArrayOop(obj)->base();
  T* p               = base + start;
  T* const chunk_end = base + end;
  while (p < chunk_end) {
    if (PSScavenge::should_scavenge(p)) {
      claim_or_forward_depth(p);
    }
    ++p;
  }
}

void PSPromotionManager::process_array_chunk(oop old) {
  assert(PSChunkLargeArrays, "invariant");
  assert(old->is_objArray(), "invariant");
  assert(old->is_forwarded(), "invariant");

#if PS_PM_STATS
  ++_array_chunks_processed;
#endif // PS_PM_STATS

  oop const obj = old->forwardee();

  int start;
  int const end = arrayOop(old)->length();
  if (end > (int) _min_array_size_for_chunking) {
    // we'll chunk more
    start = end - _array_chunk_size;
    assert(start > 0, "invariant");
    arrayOop(old)->set_length(start);
    push_depth(mask_chunked_array_oop(old));
#if PS_PM_STATS
    ++_masked_pushes;
#endif // PS_PM_STATS
  } else {
    // this is the final chunk for this array
    start = 0;
    int const actual_length = arrayOop(obj)->length();
    arrayOop(old)->set_length(actual_length);
  }

  if (UseCompressedOops) {
    process_array_chunk_work<narrowOop>(obj, start, end);
  } else {
    process_array_chunk_work<oop>(obj, start, end);
  }
}

oop PSPromotionManager::oop_promotion_failed(oop obj, markOop obj_mark) {
  assert(_old_gen_is_full || PromotionFailureALot, "Sanity");

  // Attempt to CAS in the header.
  // This tests if the header is still the same as when
  // this started.  If it is the same (i.e., no forwarding
  // pointer has been installed), then this thread owns
  // it.
  if (obj->cas_forward_to(obj, obj_mark)) {
    // We won any races, we "own" this object.
    assert(obj == obj->forwardee(), "Sanity");

    if (depth_first()) {
      obj->push_contents(this);
    } else {
      // Don't bother incrementing the age, just push
      // onto the claimed_stack..
      push_breadth(obj);
    }

    // Save the mark if needed
    PSScavenge::oop_promotion_failed(obj, obj_mark);
  }  else {
    // We lost, someone else "owns" this object
    guarantee(obj->is_forwarded(), "Object must be forwarded if the cas failed.");

    // No unallocation to worry about.
    obj = obj->forwardee();
  }

#ifdef DEBUG
  if (TraceScavenge) {
    gclog_or_tty->print_cr("{%s %s 0x%x (%d)}",
                           "promotion-failure",
                           obj->blueprint()->internal_name(),
                           obj, obj->size());

  }
#endif

  return obj;
}
