
/* Copyright 2023 NVIDIA Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "realm/ucx/mpool.h"
#include "realm/ucx/ucp_utils.h"
#include "realm/logging.h"

#include <string>
#include <cmath>

namespace Realm {
  Logger log_ucp_mp("ucpmp");

namespace UCP {

////////////////////////////////////////////////////////////////////////
//
// class MPool
//
void *MPool::chunk_elems(const Chunk *chunk)
{
  void *ptr_to_align =
    (char*)(chunk + 1) + sizeof(Elem) + alignment_offset;

  uintptr_t padding  =
    PADDING_TO_ALIGN((uintptr_t)ptr_to_align, alignment);

  return (char*)(chunk + 1) + padding;
}

inline MPool::Elem *MPool::chunk_ith_elem(const Chunk *chunk, size_t i)
{
  return (MPool::Elem*)((char*)chunk->elems + (i * obj_alloc_size));
}

inline void MPool::free_list_add(Elem *elem)
{
  elem->next = free_list;
  free_list  = elem;
}

bool MPool::validate_config()
{
  bool ok = true;

  if (obj_size == 0) {
    log_ucp_mp.error() << "mpool object size 0";
    ok = false;
  }

  if (!IS_POW2(alignment)) {
    log_ucp_mp.error() << "mpool alignment not power of 2";
    ok = false;
  }
  if (alignment_offset > obj_size) {
    log_ucp_mp.error() << "mpool alignment offset greater than object size";
    ok = false;
  }
  if (init_num_objs > max_objs) {
    log_ucp_mp.error() << "mpool maximum objects less than"
                   << " initial number of objects";
    ok = false;
  }
  if (objs_per_chunk == 0) {
    log_ucp_mp.error() << "mpool objects per chunk zero";
    ok = false;
  }
  if (obj_size > max_chunk_size) {
    log_ucp_mp.error()
      << "mpool max chunk size " << max_chunk_size
      << " less than object size " << obj_size;
    ok = false;
  }
  if (expand_factor < 1.0) {
    log_ucp_mp.error() << "mpool expand factor cannot be less than 1.0.";
    ok = false;
  }

  return ok;
}
MPool::MPool(std::string name_, bool leak_check_, size_t obj_size_,
    size_t alignment_, size_t alignment_offset_,
    size_t objs_per_chunk_, size_t init_num_objs_,
    size_t max_objs_, size_t max_chunk_size_,
    double expand_factor_,
    chunk_alloc_t chunk_alloc_, void *chunk_alloc_arg_,
    chunk_release_t chunk_release_, void *chunk_release_arg_,
    obj_init_t obj_init_, void *obj_init_arg_,
    obj_cleanup_t obj_cleanup_, void *obj_cleanup_arg_)
  : name(name_)
  , leak_check(leak_check_)
  , obj_size(obj_size_)
  , alignment(alignment_)
  , alignment_offset(alignment_offset_)
  , objs_per_chunk(objs_per_chunk_)
  , init_num_objs(init_num_objs_)
  , max_objs(max_objs_)
  , max_chunk_size(max_chunk_size_)
  , expand_factor(expand_factor_)
  , chunk_alloc(chunk_alloc_)
  , chunk_alloc_arg(chunk_alloc_arg_)
  , chunk_release(chunk_release_)
  , chunk_release_arg(chunk_release_arg_)
  , obj_init(obj_init_)
  , obj_init_arg(obj_init_arg_)
  , obj_cleanup(obj_cleanup_)
  , obj_cleanup_arg(obj_cleanup_arg_)
{
  if (!validate_config()) abort();

  obj_alloc_size = ALIGN_UP_POW2(sizeof(Elem) + obj_size, alignment);

  // revise objs_per_chunk based on max_chunk_size
  size_t chunk_obj_size = objs_per_chunk * obj_size;
  chunk_obj_size        = std::min(chunk_obj_size, max_chunk_size);
  objs_per_chunk = chunk_obj_size / obj_size;

  if (!expand(init_num_objs)) {
    log_ucp_mp.error() << "mpool initial expansion to "
                       << init_num_objs << " objects failed";
    abort();
  }

  log_ucp_mp.info()
    << "mpool " << this << " " << name << " initialized"
    << " obj_size "            << obj_size
    << ", alignment "          << alignment
    << ", alignment_offset "   << alignment_offset
    << ", objs_per_chunk "     << objs_per_chunk
    << ", init_num_objs "      << init_num_objs
    << ", max_objs "           << max_objs
    << ", max_chunk_size "     << max_chunk_size
    << ", expand_factor "      << expand_factor;
}

MPool::~MPool()
{
  Elem *elem, *elem_next;
  Chunk *chunk;

  log_ucp_mp.info() << "destructing mpool " << name
                    << " num_objs "         << num_objs;

  // cleanup objects first
  if (obj_cleanup) {
    for (elem = free_list; elem != nullptr; elem = elem->next) {
      obj_cleanup(elem + 1, obj_cleanup_arg);
    }
  }

  if (leak_check) {
    elem = free_list;
    while (elem != nullptr) {
      elem_next = elem->next;
      elem->mp = nullptr;
      elem = elem_next;
    }
    for (chunk = chunks; chunk != nullptr; chunk = chunk->next) {
      for (size_t i = 0; i < chunk->num_objs; i++) {
        elem = chunk_ith_elem(chunk, i);
        if (elem->mp != nullptr) {
          log_ucp_mp.warning() << "mpool object not returned"
                               << " mp " << this << " " << name
                               << " obj " << (void*)(elem + 1);
        }
      }
    }
  }

  // release all chunks
  chunk = chunks;
  while (chunk) {
    Chunk *next_chunk = chunk->next;
    chunk_release(chunk, chunk_release_arg);
    chunk = next_chunk;
  }

  log_ucp_mp.debug() << "mpool " << this << " " << name << " destroyed";
}

bool MPool::expand(size_t ext_objs)
{
  if (ext_objs == 0) return true;
  if (num_objs == max_objs) {
    log_ucp_mp.error() << "mpool expand failed."
                       << " Maximum number of objects reached "
                       << max_objs;
    return false;
  }
  if (ext_objs > max_objs ||
      num_objs > max_objs - ext_objs) {
    ext_objs = max_objs - num_objs;
  }

  size_t ext_chunks    = std::ceil(ext_objs / (double)objs_per_chunk);
  size_t chunk_objs    = objs_per_chunk;
  size_t objs_rem      = ext_objs % objs_per_chunk;
  for (size_t i = 0; i < ext_chunks; i++) {
    // last chunk might have fewer objects
    if (i == ext_chunks - 1 && objs_rem != 0) {
      chunk_objs = objs_rem;
    }
    size_t chunk_alloc_size =
      sizeof(Chunk) + (chunk_objs * obj_alloc_size) + alignment;
    Chunk *chunk = (Chunk*)chunk_alloc(chunk_alloc_size, chunk_alloc_arg);
    if (!chunk) {
      log_ucp_mp.error() << "mpool chunk allocation failed";
      return false;
    }
    chunk->elems    = chunk_elems(chunk);
    chunk->num_objs = chunk_objs;
    chunk->next     = chunks;
    chunks          = chunk;

    for (size_t j = 0; j < chunk->num_objs; j++) {
      Elem *elem  = chunk_ith_elem(chunk, j);
      if (obj_init) {
        obj_init(elem + 1, obj_init_arg);
      }
      free_list_add(elem);
    }
  }

  num_objs   += ext_objs;
  num_chunks += ext_chunks;

  return true;
}

void *MPool::get()
{
  if (free_list == nullptr) {
    size_t ext_objs = (expand_factor - 1) * num_objs;
    if (!ext_objs) ext_objs = 1;
    if (!expand(ext_objs)) {
      log_ucp_mp.error() << "mpool expand failed";
      return nullptr;
    }
  }

  Elem *elem = free_list;
  free_list  = elem->next;
  elem->mp   = this;

  return elem + 1;
}

void MPool::put(void *obj)
{
  Elem *elem = (Elem*)obj - 1;
  elem->mp->free_list_add(elem);
}

bool MPool::has(bool with_expand)
{
  return (free_list || (with_expand && num_objs < max_objs));
}

////////////////////////////////////////////////////////////////////////
//
// class VMPool
//
VMPool::VMPool(std::string name_,
    bool leak_check_, size_t max_obj_size_,
    size_t alignment_, size_t objs_per_chunk_,
    size_t init_num_objs_, size_t max_objs_,
    size_t max_chunk_size_, double expand_factor_,
    chunk_alloc_t chunk_alloc_, void *chunk_alloc_arg_,
    chunk_release_t chunk_release_, void *chunk_release_arg_,
    obj_init_t obj_init_, void *obj_init_arg_,
    obj_cleanup_t obj_cleanup_, void *obj_cleanup_arg_)
  : max_obj_size(max_obj_size_)
  , alignment(alignment_)
{
  // the object address to which the returned buffer belongs
  // is stored right before the returned buffer address. That
  // requires sizeof(uintptr_t) bytes. For alignment's sake,
  // we need an additional alignof(uintptr_t) bytes too.
  // we also need to prepend "this" pointer.

  mpool_max_obj_size = metadata_size + alignment_ + max_obj_size;

  mp = new MPool(name_, leak_check_, mpool_max_obj_size,
      alignof(Elem), 0, objs_per_chunk_, init_num_objs_,
      max_objs_, max_chunk_size_, expand_factor_,
      chunk_alloc_, chunk_alloc_arg_, chunk_release_, chunk_release_arg_,
      obj_init_, obj_init_arg_, obj_cleanup_, obj_cleanup_arg_);
}

VMPool::~VMPool()
{
  delete mp;
}

void *VMPool::get(size_t size)
{
  assert(size <= max_obj_size);

  // we use a very simple (far from optimal) approach:
  // only keep track of one mpool object and sub-allocate buffers from it
  // if it has enough tail space. Otherwise, get a new object from mpool.

  if (!obj_cached) {
    obj_cached = addr_cached = (uintptr_t)(mp->get());
    if (!obj_cached) return nullptr;
  }

  uintptr_t parent_obj, limit;
  uintptr_t limit_cached = obj_cached + mpool_max_obj_size;
  uintptr_t addr_aligned = ALIGN_UP_POW2(
      addr_cached + metadata_size, alignment);

  if (addr_aligned + size <= limit_cached) {
    addr_cached   = addr_aligned + size;
    parent_obj    = obj_cached;
  } else {
    parent_obj    = (uintptr_t)(mp->get());
    if (!parent_obj) return nullptr;
    limit         = parent_obj + mpool_max_obj_size;
    addr_aligned  = ALIGN_UP_POW2(
        parent_obj + metadata_size, alignment);
    if ((limit - (addr_aligned + size)) >= (limit_cached - addr_cached)) {
      // the new object has more spare room than the cached one
      obj_cached  = parent_obj;
      addr_cached = addr_aligned + size;
    }
  }

  // increase parent obj refcount
  objs_map[parent_obj]++;

  // prepend "this" and the parent object info
  Elem *elem       = (Elem*)(void*)addr_aligned - 1;
  elem             = (Elem*)(void*)ALIGN_DOWN_POW2(
      (uintptr_t)(void*)elem, alignof(Elem));
  elem->vmp        = this;
  elem->parent_obj = parent_obj;

  return (void*)addr_aligned;
}

void VMPool::put(void *buf)
{
  // 1. find "this" pointer
  // 2. find the parent object to which the address belongs
  // 3. decrement the refcount of the parent object
  // 4. if the refcout reaches 0 put the object back to mpool
  Elem *elem           = (Elem*)buf - 1;
  elem                 = (Elem*)(void*)ALIGN_DOWN_POW2(
      (uintptr_t)(void*)elem, alignof(Elem));
  VMPool *vmp          = elem->vmp;
  uintptr_t parent_obj = elem->parent_obj;

  auto iter = vmp->objs_map.find(parent_obj);
  assert(iter != vmp->objs_map.end());
  if (--iter->second == 0) {
    MPool::put((void*)parent_obj);
    vmp->objs_map.erase(iter);
    if (parent_obj == vmp->obj_cached) {
      vmp->obj_cached = 0;
    }
  }
}

bool VMPool::expand(size_t ext_objs)
{
  return mp->expand(ext_objs);
}

}; // namespace UCP

}; // namespace Realm
