/*
 * Copyright (c) 1997, 2010, Oracle and/or its affiliates. All rights reserved.
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
 * Please contact Oracle, 500 Oracle Parkway, Redwood Shores, CA 94065 USA
 * or visit www.oracle.com if you need additional information or have any
 * questions.
 *
 */

#include "precompiled.hpp"
#include "classfile/altHashing.hpp"
#include "classfile/javaClasses.hpp"
#include "classfile/symbolTable.hpp"
#include "classfile/systemDictionary.hpp"
#include "gc_interface/collectedHeap.inline.hpp"
#include "memory/filemap.hpp"
#include "memory/gcLocker.inline.hpp"
#include "oops/oop.inline.hpp"
#include "oops/oop.inline2.hpp"
#include "oops/symbolKlass.hpp"
#include "runtime/mutexLocker.hpp"
#include "utilities/hashtable.inline.hpp"
#include "utilities/numberSeq.hpp"

// --------------------------------------------------------------------------

SymbolTable* SymbolTable::_the_table = NULL;
bool SymbolTable::_needs_rehashing = false;

// Create a new table and using alternate hash code, populate the new table
// with the existing strings.   Set flag to use the alternate hash code afterwards.
void SymbolTable::rehash_table() {

  assert(SafepointSynchronize::is_at_safepoint(), "must be at safepoint");
  // This should never happen with -Xshare:dump but it might in testing mode.
  if (DumpSharedSpaces) return;
  // Create a new symbol table
  SymbolTable* new_table = new SymbolTable();

  the_table()->move_to(new_table);

  // Delete the table and buckets (entries are reused in new table).
  delete _the_table;
  // Don't check if we need rehashing until the table gets unbalanced again.
  // Then rehash with a new global seed.
  _needs_rehashing = false;
  _the_table = new_table;
}

// Lookup a symbol in a bucket.

symbolOop SymbolTable::lookup(int index, const char* name,
                              int len, unsigned int hash) {
  int count = 0;
  for (HashtableEntry* e = bucket(index); e != NULL; e = e->next()) {
    count++;
    if (e->hash() == hash) {
      symbolOop sym = symbolOop(e->literal());
      if (sym->equals(name, len)) {
        return sym;
      }
    }
  }
  // If the bucket size is too deep check if this hash code is insufficient.
  if (count >= BasicHashtable::rehash_count && !needs_rehashing()) {
    _needs_rehashing = check_rehash_table(count);
  }
  return NULL;
}

// Pick hashing algorithm.
unsigned int SymbolTable::hash_symbol(const char* s, int len) {
  return the_table()->use_alternate_hashcode() ?
           AltHashing::murmur3_32(the_table()->seed(), (const jbyte*)s, len) :
           java_lang_String::to_hash(s, len);
}


// We take care not to be blocking while holding the
// SymbolTable_lock. Otherwise, the system might deadlock, since the
// symboltable is used during compilation (VM_thread) The lock free
// synchronization is simplified by the fact that we do not delete
// entries in the symbol table during normal execution (only during
// safepoints).

symbolOop SymbolTable::lookup(const char* name, int len, TRAPS) {
  unsigned int hashValue = hash_symbol(name, len);
  int index = the_table()->hash_to_index(hashValue);

  symbolOop s = the_table()->lookup(index, name, len, hashValue);

  // Found
  if (s != NULL) return s;

  // We assume that lookup() has been called already, that it failed,
  // and symbol was not found.  We create the symbol here.
  symbolKlass* sk  = (symbolKlass*) Universe::symbolKlassObj()->klass_part();
  symbolOop s_oop = sk->allocate_symbol((u1*)name, len, CHECK_NULL);
  symbolHandle sym (THREAD, s_oop);

  // Allocation must be done before grabbing the SymbolTable_lock lock
  MutexLocker ml(SymbolTable_lock, THREAD);

  // Otherwise, add to symbol to table
  return the_table()->basic_add(sym, index, (u1*)name, len, hashValue, CHECK_NULL);
}

symbolOop SymbolTable::lookup(symbolHandle sym, int begin, int end, TRAPS) {
  char* buffer;
  int index, len;
  unsigned int hashValue;
  char* name;
  {
    debug_only(No_Safepoint_Verifier nsv;)

    name = (char*)sym->base() + begin;
    len = end - begin;
    hashValue = hash_symbol(name, len);
    index = the_table()->hash_to_index(hashValue);
    symbolOop s = the_table()->lookup(index, name, len, hashValue);

    // Found
    if (s != NULL) return s;
  }

  // Otherwise, add to symbol to table. Copy to a C string first.
  char stack_buf[128];
  ResourceMark rm(THREAD);
  if (len <= 128) {
    buffer = stack_buf;
  } else {
    buffer = NEW_RESOURCE_ARRAY_IN_THREAD(THREAD, char, len);
  }
  for (int i=0; i<len; i++) {
    buffer[i] = name[i];
  }
  // Make sure there is no safepoint in the code above since name can't move.
  // We can't include the code in No_Safepoint_Verifier because of the
  // ResourceMark.

  // We assume that lookup() has been called already, that it failed,
  // and symbol was not found.  We create the symbol here.
  symbolKlass* sk  = (symbolKlass*) Universe::symbolKlassObj()->klass_part();
  symbolOop s_oop = sk->allocate_symbol((u1*)buffer, len, CHECK_NULL);
  symbolHandle newsym (THREAD, s_oop);

  // Allocation must be done before grabbing the SymbolTable_lock lock
  MutexLocker ml(SymbolTable_lock, THREAD);

  return the_table()->basic_add(newsym, index, (u1*)buffer, len, hashValue, CHECK_NULL);
}

symbolOop SymbolTable::lookup_only(const char* name, int len,
                                   unsigned int& hash) {
  hash = hash_symbol(name, len);
  int index = the_table()->hash_to_index(hash);

  return the_table()->lookup(index, name, len, hash);
}

// Suggestion: Push unicode-based lookup all the way into the hashing
// and probing logic, so there is no need for convert_to_utf8 until
// an actual new symbolOop is created.
symbolOop SymbolTable::lookup_unicode(const jchar* name, int utf16_length, TRAPS) {
  int utf8_length = UNICODE::utf8_length((jchar*) name, utf16_length);
  char stack_buf[128];
  if (utf8_length < (int) sizeof(stack_buf)) {
    char* chars = stack_buf;
    UNICODE::convert_to_utf8(name, utf16_length, chars);
    return lookup(chars, utf8_length, THREAD);
  } else {
    ResourceMark rm(THREAD);
    char* chars = NEW_RESOURCE_ARRAY(char, utf8_length + 1);;
    UNICODE::convert_to_utf8(name, utf16_length, chars);
    return lookup(chars, utf8_length, THREAD);
  }
}

symbolOop SymbolTable::lookup_only_unicode(const jchar* name, int utf16_length,
                                           unsigned int& hash) {
  int utf8_length = UNICODE::utf8_length((jchar*) name, utf16_length);
  char stack_buf[128];
  if (utf8_length < (int) sizeof(stack_buf)) {
    char* chars = stack_buf;
    UNICODE::convert_to_utf8(name, utf16_length, chars);
    return lookup_only(chars, utf8_length, hash);
  } else {
    ResourceMark rm;
    char* chars = NEW_RESOURCE_ARRAY(char, utf8_length + 1);;
    UNICODE::convert_to_utf8(name, utf16_length, chars);
    return lookup_only(chars, utf8_length, hash);
  }
}

void SymbolTable::add(constantPoolHandle cp, int names_count,
                      const char** names, int* lengths, int* cp_indices,
                      unsigned int* hashValues, TRAPS) {

  symbolKlass* sk  = (symbolKlass*) Universe::symbolKlassObj()->klass_part();
  symbolOop sym_oops[symbol_alloc_batch_size];
  bool allocated = sk->allocate_symbols(names_count, names, lengths,
                                        sym_oops, CHECK);
  if (!allocated) {
    // do it the hard way
    for (int i=0; i<names_count; i++) {
      assert(!Universe::heap()->is_in_reserved(names[i]) || GC_locker::is_active(),
         "proposed name of symbol must be stable");

      // We assume that lookup() has been called already, that it failed,
      // and symbol was not found.  We create the symbol here.
      symbolKlass* sk  = (symbolKlass*) Universe::symbolKlassObj()->klass_part();
      symbolOop s_oop = sk->allocate_symbol((u1*)names[i], lengths[i], CHECK);
      symbolHandle sym (THREAD, s_oop);

      // Allocation must be done before grabbing the SymbolTable_lock lock
      MutexLocker ml(SymbolTable_lock, THREAD);

      SymbolTable* table = the_table();
      int index = table->hash_to_index(hashValues[i]);
      symbolOop s = table->basic_add(sym, index, (u1*)names[i], lengths[i],
                                       hashValues[i], CHECK);
      cp->symbol_at_put(cp_indices[i], s);
    }
    return;
  }

  symbolHandle syms[symbol_alloc_batch_size];
  for (int i=0; i<names_count; i++) {
    syms[i] = symbolHandle(THREAD, sym_oops[i]);
  }

  // Allocation must be done before grabbing the SymbolTable_lock lock
  MutexLocker ml(SymbolTable_lock, THREAD);

  SymbolTable* table = the_table();
  bool added = table->basic_add(syms, cp, names_count, names, lengths,
                                cp_indices, hashValues, CHECK);
  assert(added, "should always return true");
}

symbolOop SymbolTable::basic_add(symbolHandle sym, int index_arg, u1 *name, int len,
                                 unsigned int hashValue_arg, TRAPS) {
  // Cannot hit a safepoint in this function because the "this" pointer can move.
  No_Safepoint_Verifier nsv;

  assert(sym->equals((char*)name, len), "symbol must be properly initialized");

  // Check if the symbol table has been rehashed, if so, need to recalculate
  // the hash value and index.
  unsigned int hashValue;
  int index;
  if (use_alternate_hashcode()) {
    hashValue = hash_symbol((const char*)name, len);
    index = hash_to_index(hashValue);
  } else {
    hashValue = hashValue_arg;
    index = index_arg;
  }

  // Since look-up was done lock-free, we need to check if another
  // thread beat us in the race to insert the symbol.

  symbolOop test = lookup(index, (char*)name, len, hashValue);
  if (test != NULL) {
    // A race occurred and another thread introduced the symbol, this one
    // will be dropped and collected.
    return test;
  }

  HashtableEntry* entry = new_entry(hashValue, sym());
  add_entry(index, entry);
  return sym();
}

bool SymbolTable::basic_add(symbolHandle* syms,
                            constantPoolHandle cp, int names_count,
                            const char** names, int* lengths,
                            int* cp_indices, unsigned int* hashValues,
                            TRAPS) {
  // Cannot hit a safepoint in this function because the "this" pointer can move.
  No_Safepoint_Verifier nsv;

  for (int i=0; i<names_count; i++) {
    assert(syms[i]->equals(names[i], lengths[i]), "symbol must be properly initialized");
    // Check if the symbol table has been rehashed, if so, need to recalculate
    // the hash value.
    unsigned int hashValue;
    if (use_alternate_hashcode()) {
      hashValue = hash_symbol(names[i], lengths[i]);
    } else {
      hashValue = hashValues[i];
    }
    // Since look-up was done lock-free, we need to check if another
    // thread beat us in the race to insert the symbol.
    int index = hash_to_index(hashValue);
    symbolOop test = lookup(index, names[i], lengths[i], hashValue);
    if (test != NULL) {
      // A race occurred and another thread introduced the symbol, this one
      // will be dropped and collected. Use test instead.
      cp->symbol_at_put(cp_indices[i], test);
    } else {
      symbolOop sym = syms[i]();
      HashtableEntry* entry = new_entry(hashValue, sym);
      add_entry(index, entry);
      cp->symbol_at_put(cp_indices[i], sym);
    }
  }
  return true;  // always returns true
}

void SymbolTable::verify() {
  for (int i = 0; i < the_table()->table_size(); ++i) {
    HashtableEntry* p = the_table()->bucket(i);
    for ( ; p != NULL; p = p->next()) {
      symbolOop s = symbolOop(p->literal());
      guarantee(s != NULL, "symbol is NULL");
      s->verify();
      guarantee(s->is_perm(), "symbol not in permspace");
      unsigned int h = hash_symbol((const char*)s->bytes(), s->utf8_length());
      guarantee(p->hash() == h, "broken hash in symbol table entry");
      guarantee(the_table()->hash_to_index(h) == i,
                "wrong index in symbol table");
    }
  }
}

void SymbolTable::dump(outputStream* st) {
  NumberSeq summary;
  for (int i = 0; i < the_table()->table_size(); ++i) {
    int count = 0;
    for (HashtableEntry* e = the_table()->bucket(i);
       e != NULL; e = e->next()) {
      count++;
    }
    summary.add((double)count);
  }
  st->print_cr("SymbolTable statistics:");
  st->print_cr("Number of buckets       : %7d", summary.num());
  st->print_cr("Average bucket size     : %7.0f", summary.avg());
  st->print_cr("Variance of bucket size : %7.0f", summary.variance());
  st->print_cr("Std. dev. of bucket size: %7.0f", summary.sd());
  st->print_cr("Maximum bucket size     : %7.0f", summary.maximum());
}

//---------------------------------------------------------------------------
// Non-product code

#ifndef PRODUCT

void SymbolTable::print_histogram() {
  MutexLocker ml(SymbolTable_lock);
  const int results_length = 100;
  int results[results_length];
  int i,j;

  // initialize results to zero
  for (j = 0; j < results_length; j++) {
    results[j] = 0;
  }

  int total = 0;
  int max_symbols = 0;
  int out_of_range = 0;
  for (i = 0; i < the_table()->table_size(); i++) {
    HashtableEntry* p = the_table()->bucket(i);
    for ( ; p != NULL; p = p->next()) {
      int counter = symbolOop(p->literal())->utf8_length();
      total += counter;
      if (counter < results_length) {
        results[counter]++;
      } else {
        out_of_range++;
      }
      max_symbols = MAX2(max_symbols, counter);
    }
  }
  tty->print_cr("Symbol Table:");
  tty->print_cr("%8s %5d", "Total  ", total);
  tty->print_cr("%8s %5d", "Maximum", max_symbols);
  tty->print_cr("%8s %3.2f", "Average",
          ((float) total / (float) the_table()->table_size()));
  tty->print_cr("%s", "Histogram:");
  tty->print_cr(" %s %29s", "Length", "Number chains that length");
  for (i = 0; i < results_length; i++) {
    if (results[i] > 0) {
      tty->print_cr("%6d %10d", i, results[i]);
    }
  }
  int line_length = 70;
  tty->print_cr("%s %30s", " Length", "Number chains that length");
  for (i = 0; i < results_length; i++) {
    if (results[i] > 0) {
      tty->print("%4d", i);
      for (j = 0; (j < results[i]) && (j < line_length);  j++) {
        tty->print("%1s", "*");
      }
      if (j == line_length) {
        tty->print("%1s", "+");
      }
      tty->cr();
    }
  }
  tty->print_cr(" %s %d: %d\n", "Number chains longer than",
                    results_length, out_of_range);
}
#endif // PRODUCT

// --------------------------------------------------------------------------

#ifdef ASSERT
class StableMemoryChecker : public StackObj {
  enum { _bufsize = wordSize*4 };

  address _region;
  jint    _size;
  u1      _save_buf[_bufsize];

  int sample(u1* save_buf) {
    if (_size <= _bufsize) {
      memcpy(save_buf, _region, _size);
      return _size;
    } else {
      // copy head and tail
      memcpy(&save_buf[0],          _region,                      _bufsize/2);
      memcpy(&save_buf[_bufsize/2], _region + _size - _bufsize/2, _bufsize/2);
      return (_bufsize/2)*2;
    }
  }

 public:
  StableMemoryChecker(const void* region, jint size) {
    _region = (address) region;
    _size   = size;
    sample(_save_buf);
  }

  bool verify() {
    u1 check_buf[sizeof(_save_buf)];
    int check_size = sample(check_buf);
    return (0 == memcmp(_save_buf, check_buf, check_size));
  }

  void set_region(const void* region) { _region = (address) region; }
};
#endif


// --------------------------------------------------------------------------


StringTable* StringTable::_the_table = NULL;

bool StringTable::_needs_rehashing = false;

// Pick hashing algorithm
unsigned int StringTable::hash_string(const jchar* s, int len) {
  return the_table()->use_alternate_hashcode() ? AltHashing::murmur3_32(the_table()->seed(), s, len) :
                                    java_lang_String::to_hash(s, len);
}

oop StringTable::lookup(int index, jchar* name,
                        int len, unsigned int hash) {
  int count = 0;
  for (HashtableEntry* l = bucket(index); l != NULL; l = l->next()) {
    count++;
    if (l->hash() == hash) {
      if (java_lang_String::equals(l->literal(), name, len)) {
        return l->literal();
      }
    }
  }
  // If the bucket size is too deep check if this hash code is insufficient.
  if (count >= BasicHashtable::rehash_count && !needs_rehashing()) {
    _needs_rehashing = check_rehash_table(count);
  }
  return NULL;
}


oop StringTable::basic_add(int index_arg, Handle string, jchar* name,
                           int len, unsigned int hashValue_arg, TRAPS) {

  // Cannot hit a safepoint in this function because the "this" pointer can move.
  No_Safepoint_Verifier nsv;

  assert(java_lang_String::equals(string(), name, len),
         "string must be properly initialized");

  // Check if the symbol table has been rehashed, if so, need to recalculate
  // the hash value and index before second lookup.
  unsigned int hashValue;
  int index;
  if (use_alternate_hashcode()) {
    hashValue = hash_string(name, len);
    index = hash_to_index(hashValue);
  } else {
    hashValue = hashValue_arg;
    index = index_arg;
  }

  // Since look-up was done lock-free, we need to check if another
  // thread beat us in the race to insert the symbol.

  oop test = lookup(index, name, len, hashValue); // calls lookup(u1*, int)
  if (test != NULL) {
    // Entry already added
    return test;
  }

  HashtableEntry* entry = new_entry(hashValue, string());
  add_entry(index, entry);
  return string();
}


oop StringTable::lookup(symbolOop symbol) {
  ResourceMark rm;
  int length;
  jchar* chars = symbol->as_unicode(length);
  unsigned int hashValue = hash_string(chars, length);
  int index = the_table()->hash_to_index(hashValue);
  return the_table()->lookup(index, chars, length, hashValue);
}


oop StringTable::intern(Handle string_or_null, jchar* name,
                        int len, TRAPS) {
  unsigned int hashValue = hash_string(name, len);
  int index = the_table()->hash_to_index(hashValue);
  oop found_string = the_table()->lookup(index, name, len, hashValue);

  // Found
  if (found_string != NULL) return found_string;

  debug_only(StableMemoryChecker smc(name, len * sizeof(name[0])));
  assert(!Universe::heap()->is_in_reserved(name) || GC_locker::is_active(),
         "proposed name of symbol must be stable");

  Handle string;
  // try to reuse the string if possible
  if (!string_or_null.is_null() && string_or_null()->is_perm()) {
    string = string_or_null;
  } else {
    string = java_lang_String::create_tenured_from_unicode(name, len, CHECK_NULL);
  }

  // Allocation must be done before grabbing the StringTable_lock lock
  MutexLocker ml(StringTable_lock, THREAD);

  // Otherwise, add to symbol to table
  return the_table()->basic_add(index, string, name, len,
                                hashValue, CHECK_NULL);
}

oop StringTable::intern(symbolOop symbol, TRAPS) {
  if (symbol == NULL) return NULL;
  ResourceMark rm(THREAD);
  int length;
  jchar* chars = symbol->as_unicode(length);
  Handle string;
  oop result = intern(string, chars, length, CHECK_NULL);
  return result;
}


oop StringTable::intern(oop string, TRAPS)
{
  if (string == NULL) return NULL;
  ResourceMark rm(THREAD);
  int length;
  Handle h_string (THREAD, string);
  jchar* chars = java_lang_String::as_unicode_string(string, length);
  oop result = intern(h_string, chars, length, CHECK_NULL);
  return result;
}


oop StringTable::intern(const char* utf8_string, TRAPS) {
  if (utf8_string == NULL) return NULL;
  ResourceMark rm(THREAD);
  int length = UTF8::unicode_length(utf8_string);
  jchar* chars = NEW_RESOURCE_ARRAY(jchar, length);
  UTF8::convert_to_unicode(utf8_string, chars, length);
  Handle string;
  oop result = intern(string, chars, length, CHECK_NULL);
  return result;
}

void StringTable::verify() {
  for (int i = 0; i < the_table()->table_size(); ++i) {
    HashtableEntry* p = the_table()->bucket(i);
    for ( ; p != NULL; p = p->next()) {
      oop s = p->literal();
      guarantee(s != NULL, "interned string is NULL");
      guarantee(s->is_perm(), "interned string not in permspace");

      int length;
      jchar* chars = java_lang_String::as_unicode_string(s, length);
      unsigned int h = hash_string(chars, length);
      guarantee(p->hash() == h, "broken hash in string table entry");
      guarantee(the_table()->hash_to_index(h) == i,
                "wrong index in string table");
    }
  }
}

void StringTable::dump(outputStream* st) {
  NumberSeq summary;
  for (int i = 0; i < the_table()->table_size(); ++i) {
    HashtableEntry* p = the_table()->bucket(i);
    int count = 0;
    for ( ; p != NULL; p = p->next()) {
      count++;
    }
    summary.add((double)count);
  }
  st->print_cr("StringTable statistics:");
  st->print_cr("Number of buckets       : %7d", summary.num());
  st->print_cr("Average bucket size     : %7.0f", summary.avg());
  st->print_cr("Variance of bucket size : %7.0f", summary.variance());
  st->print_cr("Std. dev. of bucket size: %7.0f", summary.sd());
  st->print_cr("Maximum bucket size     : %7.0f", summary.maximum());
}


// Create a new table and using alternate hash code, populate the new table
// with the existing strings.   Set flag to use the alternate hash code afterwards.
void StringTable::rehash_table() {
  assert(SafepointSynchronize::is_at_safepoint(), "must be at safepoint");
  // This should never happen with -Xshare:dump but it might in testing mode.
  if (DumpSharedSpaces) return;
  StringTable* new_table = new StringTable();

  // Rehash the table
  the_table()->move_to(new_table);

  // Delete the table and buckets (entries are reused in new table).
  delete _the_table;
  // Don't check if we need rehashing until the table gets unbalanced again.
  // Then rehash with a new global seed.
  _needs_rehashing = false;
  _the_table = new_table;
}
