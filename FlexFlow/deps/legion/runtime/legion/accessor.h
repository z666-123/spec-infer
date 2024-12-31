/* Copyright 2023 Stanford University, NVIDIA Corporation
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

// include this _before_ anything below to deal with weird ordering 
//  constraints w.r.t. Legion's other definitions
// (it will actually turn around and include this file when it's ready)
#include "legion/legion_types.h"

#ifndef RUNTIME_ACCESSOR_H
#define RUNTIME_ACCESSOR_H

#if defined (__CUDACC__) || defined (__HIPCC__)
#define CUDAPREFIX __host__ __device__
#else
#define CUDAPREFIX
#endif

#include "legion/arrays.h"
#include "realm/instance.h"
#include "legion/legion_domain.h"

// for fprintf
#include <stdio.h>

// for off_t
#include <sys/types.h>

// Imported ptr_t definition from old common.h
struct ptr_t
{
public:
#if defined (__CUDACC__) || defined (__HIPCC__)
  __host__ __device__
#endif
  ptr_t(void) : value(0) { }
#if defined (__CUDACC__) || defined (__HIPCC__)
  __host__ __device__
#endif
  ptr_t(const ptr_t &p) : value(p.value) { }
#if defined (__CUDACC__) || defined (__HIPCC__)
  __host__ __device__
#endif
  ptr_t(long long int v) : value(v) { }
public:
  long long int value;
public:
#if defined (__CUDACC__) || defined (__HIPCC__)
  __host__ __device__
#endif
  inline ptr_t& operator=(const ptr_t &ptr) { value = ptr.value; return *this; }
#if defined (__CUDACC__) || defined (__HIPCC__)
  __host__ __device__
#endif
  inline bool operator==(const ptr_t &ptr) const { return (ptr.value == this->value); }
#if defined (__CUDACC__) || defined (__HIPCC__)
  __host__ __device__
#endif
  inline bool operator!=(const ptr_t &ptr) const { return (ptr.value != this->value); }
#if defined (__CUDACC__) || defined (__HIPCC__)
  __host__ __device__
#endif
  inline bool operator< (const ptr_t &ptr) const { return (ptr.value <  this->value); }
#if defined (__CUDACC__) || defined (__HIPCC__)
  __host__ __device__
#endif
  inline operator bool(void) const { return (value != -1LL); }
#if defined (__CUDACC__) || defined (__HIPCC__)
  __host__ __device__
#endif
  inline bool operator!(void) const { return (value == -1LL); }

  // Addition operation on pointers
#if defined (__CUDACC__) || defined (__HIPCC__)
  __host__ __device__
#endif
  inline ptr_t operator+(const ptr_t &ptr) const { return ptr_t(value + ptr.value); }
#if defined (__CUDACC__) || defined (__HIPCC__)
  __host__ __device__
#endif
  inline ptr_t operator+(unsigned offset) const { return ptr_t(value + offset); }
#if defined (__CUDACC__) || defined (__HIPCC__)
  __host__ __device__
#endif
  inline ptr_t operator+(int offset) const { return ptr_t(value + offset); }

  // Subtraction operation on pointers
#if defined (__CUDACC__) || defined (__HIPCC__)
  __host__ __device__
#endif
  inline ptr_t operator-(const ptr_t &ptr) const { return ptr_t(value - ptr.value); }
#if defined (__CUDACC__) || defined (__HIPCC__)
  __host__ __device__
#endif
  inline ptr_t operator-(unsigned offset) const { return ptr_t(value - offset); }
#if defined (__CUDACC__) || defined (__HIPCC__)
  __host__ __device__
#endif
  inline ptr_t operator-(int offset) const { return ptr_t(value - offset); }

#if defined (__CUDACC__) || defined (__HIPCC__)
  __host__ __device__
#endif
  inline ptr_t& operator++(void) { value++; return *this; }
#if defined (__CUDACC__) || defined (__HIPCC__)
  __host__ __device__
#endif
  inline ptr_t operator++(int) { value++; return *this; }
#if defined (__CUDACC__) || defined (__HIPCC__)
  __host__ __device__
#endif
  inline ptr_t& operator--(void) { value--; return *this; }
#if defined (__CUDACC__) || defined (__HIPCC__)
  __host__ __device__
#endif
  inline ptr_t operator--(int) { value--; return *this; }

  // Thank you Eric for type cast operators!
#if defined (__CUDACC__) || defined (__HIPCC__)
  __host__ __device__
#endif
  inline operator long long int(void) const { return value; }

#if defined (__CUDACC__) || defined (__HIPCC__)
  __host__ __device__
#endif
  inline bool is_null(void) const { return (value == -1LL); }

#if defined (__CUDACC__) || defined (__HIPCC__)
  __host__ __device__
#endif
  static inline ptr_t nil(void) { ptr_t p; p.value = -1LL; return p; }
};

namespace LegionRuntime {
#ifdef LEGION_PRIVILEGE_CHECKS
  enum AccessorPrivilege {
    ACCESSOR_NONE   = 0x00000000,
    ACCESSOR_READ   = 0x00000001,
    ACCESSOR_WRITE  = 0x00000002,
    ACCESSOR_REDUCE = 0x00000004,
    ACCESSOR_ALL    = 0x00000007,
  };
#endif
  namespace Accessor {

    class ByteOffset {
    public:
      CUDAPREFIX ByteOffset(void) : offset(0) {}
      CUDAPREFIX explicit ByteOffset(off_t _offset) : offset(_offset) { assert(offset == _offset); }
      CUDAPREFIX explicit ByteOffset(int _offset) : offset(_offset) {}

      template <typename T1, typename T2>
      CUDAPREFIX ByteOffset(T1 *p1, T2 *p2) : offset(((char *)p1) - ((char *)p2)) {}

      template <typename T>
      CUDAPREFIX T *add_to_pointer(T *ptr) const
      {
	return (T*)(((char *)ptr) + offset);
      }

      CUDAPREFIX bool operator==(const ByteOffset rhs) const { return offset == rhs.offset; }
      CUDAPREFIX bool operator!=(const ByteOffset rhs) const { return offset != rhs.offset; }

      CUDAPREFIX ByteOffset& operator+=(ByteOffset rhs) { offset += rhs.offset; return *this; }
      CUDAPREFIX ByteOffset& operator*=(int scale) { offset *= scale; return *this; }
      
      CUDAPREFIX ByteOffset operator+(const ByteOffset rhs) const { return ByteOffset(offset + rhs.offset); }
      CUDAPREFIX ByteOffset operator*(int scale) const { return ByteOffset(offset * scale); }
      
      int offset;
    };

    CUDAPREFIX inline ByteOffset operator*(int scale, const ByteOffset rhs) { return ByteOffset(scale * rhs.offset); }

    template <typename T>
    CUDAPREFIX inline T* operator+(T *ptr, const ByteOffset offset) { return offset.add_to_pointer(ptr); }

    template <typename T>
    CUDAPREFIX inline T*& operator+=(T *&ptr, const ByteOffset offset) { ptr = offset.add_to_pointer(ptr); return ptr; }

    namespace TemplateFu {
      // bool IsAStruct<T>::value == true if T is a class/struct, else false
      template <typename T> 
      struct IsAStruct {
	typedef char yes[2];
	typedef char no[1];

	template <typename T2> static yes& test_for_struct(int T2:: *x);
	template <typename T2> static no& test_for_struct(...);

	static const bool value = sizeof(test_for_struct<T>(0)) == sizeof(yes);
      };
    };

#ifndef REGION_ACCESSOR_ALREADY_PROTOTYPED
    template <typename AT, typename ET = void, typename PT = ET> struct RegionAccessor;
#endif

    template <typename AT> struct RegionAccessor<AT, void, void> : public AT::Untyped {
      CUDAPREFIX
      RegionAccessor(void)
        : AT::Untyped() {}
      CUDAPREFIX
      RegionAccessor(const typename AT::Untyped& to_copy)
	: AT::Untyped(to_copy) {}
    };

    // Helper functions for extracting an accessor from a region
    template<typename AT, typename ET, typename RT>
    inline RegionAccessor<AT,ET> extract_accessor(const RT &region) 
    {
      return region.get_accessor().template typeify<ET>().template convert<AT>();
    }

    template<typename AT, typename ET, typename RT>
    inline RegionAccessor<AT,ET> extract_accessor(const RT &region, unsigned fid) 
    {
      return region.get_field_accessor(fid).template typeify<ET>().template convert<AT>();
    }

    namespace DebugHooks {
      // these are calls that can be implemented by a higher level (e.g. Legion) to
      //  perform privilege/bounds checks on accessor reference and produce more useful
      //  information for debug

      extern void (*check_bounds_ptr)(void *region, ptr_t ptr);
      extern void (*check_bounds_dpoint)(void *region, const Legion::DomainPoint &dp);

      // single entry point calls the right one (if present) using overloading
      inline void check_bounds(void *region, ptr_t ptr)
      {
	if(check_bounds_ptr)
	  (check_bounds_ptr)(region, ptr);
      }

      inline void check_bounds(void *region, const Legion::DomainPoint &dp)
      {
	if(check_bounds_dpoint)
	  (check_bounds_dpoint)(region, dp);
      }

      extern const char *(*find_privilege_task_name)(void *region);
    };

    namespace AccessorType {
      template <typename T, off_t val> 
      struct Const {
      public:
        CUDAPREFIX
        Const(void) {}
        CUDAPREFIX
        Const(T _value) { assert(_value == val); }
	static const T value = val;
      };

      template <typename T> 
      struct Const<T, 0> {
      public:
        CUDAPREFIX
        Const(void) : value(0) {}
        CUDAPREFIX
        Const(T _value) : value(_value) {}
	/*const*/ T value;
      };

      template <size_t STRIDE> struct AOS;
      template <size_t STRIDE> struct SOA;
      template <size_t STRIDE, size_t BLOCK_SIZE, size_t BLOCK_STRIDE> struct HybridSOA;
      template <unsigned DIM> struct Affine;

      template <typename REDOP> struct ReductionFold;
      template <typename REDOP> struct ReductionList;

#ifdef LEGION_PRIVILEGE_CHECKS
      // TODO: Make these functions work for GPUs
      static inline const char* get_privilege_string(AccessorPrivilege priv)
      {
        switch (priv)
        {
          case ACCESSOR_NONE:
            return "NONE";
          case ACCESSOR_READ:
            return "READ-ONLY";
          case ACCESSOR_WRITE:
            return "WRITE-DISCARD";
          case ACCESSOR_REDUCE:
            return "REDUCE";
          case ACCESSOR_ALL:
            return "READ-WRITE";
          default:
            assert(false);
        }
        return NULL;
      }

      template<AccessorPrivilege REQUESTED>
      static inline void check_privileges(AccessorPrivilege held, void *region)
      {
        if (!(held & REQUESTED))
        {
          const char *held_string = get_privilege_string(held);
          const char *req_string = get_privilege_string(REQUESTED);
          const char *task_name = (DebugHooks::find_privilege_task_name ?
				   (DebugHooks::find_privilege_task_name)(region) : "(unknown)");
          fprintf(stderr,"PRIVILEGE CHECK ERROR IN TASK %s: Need %s privileges but "
                         "only hold %s privileges\n", task_name, req_string, held_string);
          assert(false);
        }
      }
#endif

      struct Generic {
	struct Untyped {
	  Untyped() 
	    : inst(Realm::RegionInstance::NO_INST)
	    , field_id(0)
#if defined(LEGION_PRIVILEGE_CHECKS) || defined(LEGION_BOUNDS_CHECKS) 
	    , region(0)
#endif
	  {}

	  Untyped(Realm::RegionInstance _inst, Realm::FieldID _field_id = 0)
	    : inst(_inst)
	    , field_id(_field_id)
#if defined(LEGION_PRIVILEGE_CHECKS) || defined(LEGION_BOUNDS_CHECKS) 
	    , region(0)
#endif
	  {}

	  template <typename ET>
	  RegionAccessor<Generic, ET, ET> typeify(void) const {
	    RegionAccessor<Generic, ET, ET> result(Typed<ET, ET>(inst, field_id));
#if defined(LEGION_PRIVILEGE_CHECKS) || defined(LEGION_BOUNDS_CHECKS)
            result.set_region(region);
#endif
#ifdef LEGION_PRIVILEGE_CHECKS
            result.set_privileges(priv);
#endif
            return result;
	  }
#ifdef __OPTIMIZE__
          inline void issue_performance_warning(void) const
          {
            // Disable these warnings for now
#if 0
            for (int i = 0; i < 1024; i++)
              fprintf(stdout,"WARNING: STOP USING GENERIC ACCESSORS IN "
                      "OPTIMIZED CODE!\n");
#endif
          }
#endif
	  void read_untyped(ptr_t ptr, void *dst, size_t bytes, off_t offset = 0) const
	  {
	    typedef Realm::GenericAccessor<char, 1, Arrays::coord_t> AT;
	    assert(AT::is_compatible(inst, field_id));
	    AT acc(inst, field_id);
	    size_t start = acc.get_offset(Realm::Point<1, Arrays::coord_t>(ptr.value));
	    inst.read_untyped(start + offset, dst, bytes);
	  }
	  
	  void write_untyped(ptr_t ptr, const void *src, size_t bytes, off_t offset = 0) const
	  {
	    typedef Realm::GenericAccessor<char, 1, Arrays::coord_t> AT;
	    assert(AT::is_compatible(inst, field_id));
	    AT acc(inst, field_id);
	    size_t start = acc.get_offset(Realm::Point<1, Arrays::coord_t>(ptr.value));
	    inst.write_untyped(start + offset, src, bytes);
	  }

	  void read_untyped(const Legion::DomainPoint& dp, void *dst, size_t bytes, off_t offset = 0) const
          {
	    size_t start = 0;
	    switch(dp.get_dim()) {
	    case 0: {
	      typedef Realm::GenericAccessor<char, 1, Arrays::coord_t> AT;
	      assert(AT::is_compatible(inst, field_id));
	      AT acc(inst, field_id);
	      start = acc.get_offset(Realm::Point<1, Arrays::coord_t>(dp.get_index()));
	      break;
	    }
	    case 1: {
	      typedef Realm::GenericAccessor<char, 1, Arrays::coord_t> AT;
	      assert(AT::is_compatible(inst, field_id));
	      AT acc(inst, field_id);
	      Arrays::Point<1> p = dp.get_point<1>();
	      start = acc.get_offset(Realm::Point<1, Arrays::coord_t>(p[0]));
	      break;
	    }
#if LEGION_MAX_DIM >= 2
	    case 2: {
	      typedef Realm::GenericAccessor<char, 2, Arrays::coord_t> AT;
	      assert(AT::is_compatible(inst, field_id));
	      AT acc(inst, field_id);
	      Arrays::Point<2> p = dp.get_point<2>();
	      start = acc.get_offset(Realm::Point<2, Arrays::coord_t>(p[0], p[1]));
	      break;
	    }
#endif
#if LEGION_MAX_DIM >= 3
	    case 3: {
	      typedef Realm::GenericAccessor<char, 3, Arrays::coord_t> AT;
	      assert(AT::is_compatible(inst, field_id));
	      AT acc(inst, field_id);
	      Arrays::Point<3> p = dp.get_point<3>();
	      start = acc.get_offset(Realm::Point<3, Arrays::coord_t>(p[0], p[1], p[2]));
	      break;
	    }
#endif
#if LEGION_MAX_DIM >= 4
	    case 4: {
	      typedef Realm::GenericAccessor<char, 4, Arrays::coord_t> AT;
	      assert(AT::is_compatible(inst, field_id));
	      AT acc(inst, field_id);
	      Arrays::Point<4> p = dp.get_point<4>();
	      start = acc.get_offset(Realm::Point<4, Arrays::coord_t>(p[0], p[1], p[2], p[3]));
	      break;
	    }
#endif
#if LEGION_MAX_DIM >= 5
	    case 5: {
	      typedef Realm::GenericAccessor<char, 5, Arrays::coord_t> AT;
	      assert(AT::is_compatible(inst, field_id));
	      AT acc(inst, field_id);
	      Arrays::Point<5> p = dp.get_point<5>();
              const Legion::coord_t vals[5] = { p[0], p[1], p[2], p[3], p[4] };
	      start = acc.get_offset(Realm::Point<5, Arrays::coord_t>(vals));
	      break;
	    }
#endif
#if LEGION_MAX_DIM >= 6
	    case 6: {
	      typedef Realm::GenericAccessor<char, 6, Arrays::coord_t> AT;
	      assert(AT::is_compatible(inst, field_id));
	      AT acc(inst, field_id);
	      Arrays::Point<6> p = dp.get_point<6>();
              const Legion::coord_t vals[6] = { p[0], p[1], p[2], p[3], p[4], p[5] };
	      start = acc.get_offset(Realm::Point<6, Arrays::coord_t>(vals));
	      break;
	    }
#endif
#if LEGION_MAX_DIM >= 7
	    case 7: {
	      typedef Realm::GenericAccessor<char, 7, Arrays::coord_t> AT;
	      assert(AT::is_compatible(inst, field_id));
	      AT acc(inst, field_id);
	      Arrays::Point<7> p = dp.get_point<7>();
              const Legion::coord_t vals[7] = { p[0], p[1], p[2], p[3], p[4], p[5], p[6] };
	      start = acc.get_offset(Realm::Point<7, Arrays::coord_t>(vals));
	      break;
	    }
#endif
#if LEGION_MAX_DIM >= 8
	    case 8: {
	      typedef Realm::GenericAccessor<char, 8, Arrays::coord_t> AT;
	      assert(AT::is_compatible(inst, field_id));
	      AT acc(inst, field_id);
	      Arrays::Point<8> p = dp.get_point<8>();
              const Legion::coord_t vals[8] = { p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7] };
	      start = acc.get_offset(Realm::Point<8, Arrays::coord_t>(vals));
	      break;
	    }
#endif
#if LEGION_MAX_DIM >= 9
	    case 9: {
	      typedef Realm::GenericAccessor<char, 9, Arrays::coord_t> AT;
	      assert(AT::is_compatible(inst, field_id));
	      AT acc(inst, field_id);
	      Arrays::Point<9> p = dp.get_point<9>();
              const Legion::coord_t vals[9] = { p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], p[8] };
	      start = acc.get_offset(Realm::Point<9, Arrays::coord_t>(vals));
	      break;
	    }
#endif
	    default: assert(0);
	    }
	    inst.read_untyped(start + offset, dst, bytes);
	  }

	  void write_untyped(const Legion::DomainPoint& dp, const void *src, size_t bytes, off_t offset = 0) const
	  {
	    size_t start = 0;
	    switch(dp.get_dim()) {
	    case 0: {
	      typedef Realm::GenericAccessor<char, 1, Arrays::coord_t> AT;
	      assert(AT::is_compatible(inst, field_id));
	      AT acc(inst, field_id);
	      start = acc.get_offset(Realm::Point<1, Arrays::coord_t>(dp.get_index()));
	      break;
	    }
	    case 1: {
	      typedef Realm::GenericAccessor<char, 1, Arrays::coord_t> AT;
	      assert(AT::is_compatible(inst, field_id));
	      AT acc(inst, field_id);
	      Arrays::Point<1> p = dp.get_point<1>();
	      start = acc.get_offset(Realm::Point<1, Arrays::coord_t>(p[0]));
	      break;
	    }
#if LEGION_MAX_DIM >= 2
	    case 2: {
	      typedef Realm::GenericAccessor<char, 2, Arrays::coord_t> AT;
	      assert(AT::is_compatible(inst, field_id));
	      AT acc(inst, field_id);
	      Arrays::Point<2> p = dp.get_point<2>();
	      start = acc.get_offset(Realm::Point<2, Arrays::coord_t>(p[0], p[1]));
	      break;
	    }
#endif
#if LEGION_MAX_DIM >= 3
	    case 3: {
	      typedef Realm::GenericAccessor<char, 3, Arrays::coord_t> AT;
	      assert(AT::is_compatible(inst, field_id));
	      AT acc(inst, field_id);
	      Arrays::Point<3> p = dp.get_point<3>();
	      start = acc.get_offset(Realm::Point<3, Arrays::coord_t>(p[0], p[1], p[2]));
	      break;
	    }
#endif
#if LEGION_MAX_DIM >= 4
	    case 4: {
	      typedef Realm::GenericAccessor<char, 4, Arrays::coord_t> AT;
	      assert(AT::is_compatible(inst, field_id));
	      AT acc(inst, field_id);
	      Arrays::Point<4> p = dp.get_point<4>();
	      start = acc.get_offset(Realm::Point<4, Arrays::coord_t>(p[0], p[1], p[2], p[3]));
	      break;
	    }
#endif
#if LEGION_MAX_DIM >= 5
	    case 5: {
	      typedef Realm::GenericAccessor<char, 5, Arrays::coord_t> AT;
	      assert(AT::is_compatible(inst, field_id));
	      AT acc(inst, field_id);
	      Arrays::Point<5> p = dp.get_point<5>();
              const Legion::coord_t vals[5] = { p[0], p[1], p[2], p[3], p[4] };
	      start = acc.get_offset(Realm::Point<5, Arrays::coord_t>(vals));
	      break;
	    }
#endif
#if LEGION_MAX_DIM >= 6
	    case 6: {
	      typedef Realm::GenericAccessor<char, 6, Arrays::coord_t> AT;
	      assert(AT::is_compatible(inst, field_id));
	      AT acc(inst, field_id);
	      Arrays::Point<6> p = dp.get_point<6>();
              const Legion::coord_t vals[6] = { p[0], p[1], p[2], p[3], p[4], p[5] };
	      start = acc.get_offset(Realm::Point<6, Arrays::coord_t>(vals));
	      break;
	    }
#endif
#if LEGION_MAX_DIM >= 7
	    case 7: {
	      typedef Realm::GenericAccessor<char, 7, Arrays::coord_t> AT;
	      assert(AT::is_compatible(inst, field_id));
	      AT acc(inst, field_id);
	      Arrays::Point<7> p = dp.get_point<7>();
              const Legion::coord_t vals[7] = { p[0], p[1], p[2], p[3], p[4], p[5], p[6] };
	      start = acc.get_offset(Realm::Point<7, Arrays::coord_t>(vals));
	      break;
	    }
#endif
#if LEGION_MAX_DIM >= 8
	    case 8: {
	      typedef Realm::GenericAccessor<char, 8, Arrays::coord_t> AT;
	      assert(AT::is_compatible(inst, field_id));
	      AT acc(inst, field_id);
	      Arrays::Point<8> p = dp.get_point<8>();
              const Legion::coord_t vals[8] = { p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7] };
	      start = acc.get_offset(Realm::Point<8, Arrays::coord_t>(vals));
	      break;
	    }
#endif
#if LEGION_MAX_DIM >= 9
	    case 9: {
	      typedef Realm::GenericAccessor<char, 9, Arrays::coord_t> AT;
	      assert(AT::is_compatible(inst, field_id));
	      AT acc(inst, field_id);
	      Arrays::Point<9> p = dp.get_point<9>();
              const Legion::coord_t vals[9] = { p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], p[8] };
	      start = acc.get_offset(Realm::Point<9, Arrays::coord_t>(vals));
	      break;
	    }
#endif
	    default: assert(0);
	    }
	    inst.write_untyped(start + offset, src, bytes);
	  }

	  void report_fault(ptr_t ptr, size_t bytes, off_t offset = 0) const;
	  void report_fault(const Legion::DomainPoint& dp, size_t bytes, off_t offset = 0) const;

	  RegionAccessor<Generic, void, void> get_untyped_field_accessor(off_t _field_id, size_t _field_size)
	  {
	    return RegionAccessor<Generic, void, void>(Untyped(inst, _field_id));
	  }

	  void *raw_span_ptr(ptr_t ptr, size_t req_count, size_t& act_count, ByteOffset& stride) const
	  {
	    typedef Realm::AffineAccessor<char, 1, Arrays::coord_t> AT;
	    Realm::Rect<1, Arrays::coord_t> r;
	    r.lo.x = ptr.value;
	    r.hi.x = ptr.value + req_count - 1;
	    assert(AT::is_compatible(inst, field_id, r));
	    AT acc(inst, field_id, r);
	    char *dst = acc.ptr(r.lo);
	    act_count = req_count;
	    stride = ByteOffset(off_t(acc.strides[0]));
	    return dst;
	  }

	  // for whole instance - fails if not completely affine
	  template <int DIM>
	  void *raw_rect_ptr(ByteOffset *offsets) const
	  {
	    switch(DIM) {
	    case 1: {
	      typedef Realm::AffineAccessor<char, 1, Arrays::coord_t> AT;
	      assert(AT::is_compatible(inst, field_id));
	      AT acc(inst, field_id);
	      char *dst = acc.ptr(Realm::Point<1, Arrays::coord_t>(0));
	      offsets[0] = ByteOffset(off_t(acc.strides[0]));
	      return dst;
	    }
#if LEGION_MAX_DIM >= 2
	    case 2: {
	      typedef Realm::AffineAccessor<char, 2, Arrays::coord_t> AT;
	      assert(AT::is_compatible(inst, field_id));
	      AT acc(inst, field_id);
	      char *dst = acc.ptr(Realm::Point<2, Arrays::coord_t>(0, 0));
	      offsets[0] = ByteOffset(off_t(acc.strides[0]));
	      offsets[1] = ByteOffset(off_t(acc.strides[1]));
	      return dst;
	    }
#endif
#if LEGION_MAX_DIM >= 3
	    case 3: {
	      typedef Realm::AffineAccessor<char, 3, Arrays::coord_t> AT;
	      assert(AT::is_compatible(inst, field_id));
	      AT acc(inst, field_id);
	      char *dst = acc.ptr(Realm::Point<3, Arrays::coord_t>(0, 0, 0));
	      offsets[0] = ByteOffset(off_t(acc.strides[0]));
	      offsets[1] = ByteOffset(off_t(acc.strides[1]));
	      offsets[2] = ByteOffset(off_t(acc.strides[2]));
	      return dst;
	    }
#endif
#if LEGION_MAX_DIM >= 4
	    case 4: {
	      typedef Realm::AffineAccessor<char, 4, Arrays::coord_t> AT;
	      assert(AT::is_compatible(inst, field_id));
	      AT acc(inst, field_id);
	      char *dst = acc.ptr(Realm::Point<4, Arrays::coord_t>(0, 0, 0, 0));
	      offsets[0] = ByteOffset(off_t(acc.strides[0]));
	      offsets[1] = ByteOffset(off_t(acc.strides[1]));
              offsets[2] = ByteOffset(off_t(acc.strides[2]));
	      offsets[3] = ByteOffset(off_t(acc.strides[3]));
	      return dst;
	    }
#endif
#if LEGION_MAX_DIM >= 5
	    case 5: {
	      typedef Realm::AffineAccessor<char, 5, Arrays::coord_t> AT;
	      assert(AT::is_compatible(inst, field_id));
	      AT acc(inst, field_id);
              const Legion::coord_t zeros[5] = { 0, 0, 0, 0, 0 };
	      char *dst = acc.ptr(Realm::Point<5, Arrays::coord_t>(zeros));
	      offsets[0] = ByteOffset(off_t(acc.strides[0]));
	      offsets[1] = ByteOffset(off_t(acc.strides[1]));
	      offsets[2] = ByteOffset(off_t(acc.strides[2]));
              offsets[3] = ByteOffset(off_t(acc.strides[3]));
	      offsets[4] = ByteOffset(off_t(acc.strides[4]));
	      return dst;
	    }
#endif
#if LEGION_MAX_DIM >= 6
	    case 6: {
	      typedef Realm::AffineAccessor<char, 6, Arrays::coord_t> AT;
	      assert(AT::is_compatible(inst, field_id));
	      AT acc(inst, field_id);
              const Legion::coord_t zeros[6] = { 0, 0, 0, 0, 0, 0 };
	      char *dst = acc.ptr(Realm::Point<6, Arrays::coord_t>(zeros));
	      offsets[0] = ByteOffset(off_t(acc.strides[0]));
	      offsets[1] = ByteOffset(off_t(acc.strides[1]));
	      offsets[2] = ByteOffset(off_t(acc.strides[2]));
              offsets[3] = ByteOffset(off_t(acc.strides[3]));
	      offsets[4] = ByteOffset(off_t(acc.strides[4]));
              offsets[5] = ByteOffset(off_t(acc.strides[5]));
	      return dst;
	    }
#endif
#if LEGION_MAX_DIM >= 7
	    case 7: {
	      typedef Realm::AffineAccessor<char, 7, Arrays::coord_t> AT;
	      assert(AT::is_compatible(inst, field_id));
	      AT acc(inst, field_id);
              const Legion::coord_t zeros[7] = { 0, 0, 0, 0, 0, 0, 0 };
	      char *dst = acc.ptr(Realm::Point<7, Arrays::coord_t>(zeros));
	      offsets[0] = ByteOffset(off_t(acc.strides[0]));
	      offsets[1] = ByteOffset(off_t(acc.strides[1]));
	      offsets[2] = ByteOffset(off_t(acc.strides[2]));
              offsets[3] = ByteOffset(off_t(acc.strides[3]));
	      offsets[4] = ByteOffset(off_t(acc.strides[4]));
              offsets[5] = ByteOffset(off_t(acc.strides[5]));
              offsets[6] = ByteOffset(off_t(acc.strides[6]));
	      return dst;
	    }
#endif
#if LEGION_MAX_DIM >= 8
	    case 8: {
	      typedef Realm::AffineAccessor<char, 8, Arrays::coord_t> AT;
	      assert(AT::is_compatible(inst, field_id));
	      AT acc(inst, field_id);
              const Legion::coord_t zeros[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
	      char *dst = acc.ptr(Realm::Point<8, Arrays::coord_t>(zeros));
	      offsets[0] = ByteOffset(off_t(acc.strides[0]));
	      offsets[1] = ByteOffset(off_t(acc.strides[1]));
	      offsets[2] = ByteOffset(off_t(acc.strides[2]));
              offsets[3] = ByteOffset(off_t(acc.strides[3]));
	      offsets[4] = ByteOffset(off_t(acc.strides[4]));
              offsets[5] = ByteOffset(off_t(acc.strides[5]));
              offsets[6] = ByteOffset(off_t(acc.strides[6]));
              offsets[7] = ByteOffset(off_t(acc.strides[7]));
	      return dst;
	    }
#endif
#if LEGION_MAX_DIM >= 9
	    case 9: {
	      typedef Realm::AffineAccessor<char, 9, Arrays::coord_t> AT;
	      assert(AT::is_compatible(inst, field_id));
	      AT acc(inst, field_id);
              const Legion::coord_t zeros[9] = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	      char *dst = acc.ptr(Realm::Point<9, Arrays::coord_t>(zeros));
	      offsets[0] = ByteOffset(off_t(acc.strides[0]));
	      offsets[1] = ByteOffset(off_t(acc.strides[1]));
	      offsets[2] = ByteOffset(off_t(acc.strides[2]));
              offsets[3] = ByteOffset(off_t(acc.strides[3]));
	      offsets[4] = ByteOffset(off_t(acc.strides[4]));
              offsets[5] = ByteOffset(off_t(acc.strides[5]));
              offsets[6] = ByteOffset(off_t(acc.strides[6]));
              offsets[7] = ByteOffset(off_t(acc.strides[7]));
              offsets[8] = ByteOffset(off_t(acc.strides[8]));
	      return dst;
	    }
#endif
	    default: assert(0);
	    }
	    return 0;
	  }

	  template <int DIM>
	  void *raw_rect_ptr(const LegionRuntime::Arrays::Rect<DIM>& r, LegionRuntime::Arrays::Rect<DIM> &subrect, ByteOffset *offsets) const
	  {
	    switch(DIM) {
	    case 1: {
	      typedef Realm::AffineAccessor<char, 1, Arrays::coord_t> AT;
	      Realm::Rect<1, Arrays::coord_t> rr;
	      rr.lo = Realm::Point<1, Arrays::coord_t>(r.lo.x[0]);
	      rr.hi = Realm::Point<1, Arrays::coord_t>(r.hi.x[0]);
	      assert(AT::is_compatible(inst, field_id, rr));
	      AT acc(inst, field_id, rr);
	      char *dst = acc.ptr(rr.lo);
	      offsets[0] = ByteOffset(off_t(acc.strides[0]));
	      subrect = r;
	      return dst;
	    }
#if LEGION_MAX_DIM >= 2
	    case 2: {
	      typedef Realm::AffineAccessor<char, 2, Arrays::coord_t> AT;
	      Realm::Rect<2, Arrays::coord_t> rr;
	      rr.lo = Realm::Point<2, Arrays::coord_t>(r.lo.x[0], r.lo.x[1]);
	      rr.hi = Realm::Point<2, Arrays::coord_t>(r.hi.x[0], r.hi.x[1]);
	      assert(AT::is_compatible(inst, field_id, rr));
	      AT acc(inst, field_id, rr);
	      char *dst = acc.ptr(rr.lo);
	      offsets[0] = ByteOffset(off_t(acc.strides[0]));
	      offsets[1] = ByteOffset(off_t(acc.strides[1]));
	      subrect = r;
	      return dst;
	    }
#endif
#if LEGION_MAX_DIM >= 3
	    case 3: {
	      typedef Realm::AffineAccessor<char, 3, Arrays::coord_t> AT;
	      Realm::Rect<3, Arrays::coord_t> rr;
	      rr.lo = Realm::Point<3, Arrays::coord_t>(r.lo.x[0], r.lo.x[1], r.lo.x[2]);
	      rr.hi = Realm::Point<3, Arrays::coord_t>(r.hi.x[0], r.hi.x[1], r.hi.x[2]);
	      assert(AT::is_compatible(inst, field_id, rr));
	      AT acc(inst, field_id, rr);
	      char *dst = acc.ptr(rr.lo);
	      offsets[0] = ByteOffset(off_t(acc.strides[0]));
	      offsets[1] = ByteOffset(off_t(acc.strides[1]));
	      offsets[2] = ByteOffset(off_t(acc.strides[2]));
	      subrect = r;
	      return dst;
	    }
#endif
#if LEGION_MAX_DIM >= 4
	    case 4: {
	      typedef Realm::AffineAccessor<char, 4, Arrays::coord_t> AT;
	      Realm::Rect<4, Arrays::coord_t> rr;
	      rr.lo = Realm::Point<4, Arrays::coord_t>(r.lo.x[0], r.lo.x[1], r.lo.x[2], r.lo.x[3]);
	      rr.hi = Realm::Point<4, Arrays::coord_t>(r.hi.x[0], r.hi.x[1], r.hi.x[2], r.hi.x[3]);
	      assert(AT::is_compatible(inst, field_id, rr));
	      AT acc(inst, field_id, rr);
	      char *dst = acc.ptr(rr.lo);
	      offsets[0] = ByteOffset(off_t(acc.strides[0]));
	      offsets[1] = ByteOffset(off_t(acc.strides[1]));
	      offsets[2] = ByteOffset(off_t(acc.strides[2]));
              offsets[3] = ByteOffset(off_t(acc.strides[3]));
	      subrect = r;
	      return dst;
	    }
#endif
#if LEGION_MAX_DIM >= 5
	    case 5: {
	      typedef Realm::AffineAccessor<char, 5, Arrays::coord_t> AT;
	      Realm::Rect<5, Arrays::coord_t> rr;
	      rr.lo = Realm::Point<5, Arrays::coord_t>(r.lo.x);
	      rr.hi = Realm::Point<5, Arrays::coord_t>(r.hi.x);
	      assert(AT::is_compatible(inst, field_id, rr));
	      AT acc(inst, field_id, rr);
	      char *dst = acc.ptr(rr.lo);
	      offsets[0] = ByteOffset(off_t(acc.strides[0]));
	      offsets[1] = ByteOffset(off_t(acc.strides[1]));
	      offsets[2] = ByteOffset(off_t(acc.strides[2]));
              offsets[3] = ByteOffset(off_t(acc.strides[3]));
              offsets[4] = ByteOffset(off_t(acc.strides[4]));
	      subrect = r;
	      return dst;
	    }
#endif
#if LEGION_MAX_DIM >= 6
	    case 6: {
	      typedef Realm::AffineAccessor<char, 6, Arrays::coord_t> AT;
	      Realm::Rect<6, Arrays::coord_t> rr;
	      rr.lo = Realm::Point<6, Arrays::coord_t>(r.lo.x);
	      rr.hi = Realm::Point<6, Arrays::coord_t>(r.hi.x);
	      assert(AT::is_compatible(inst, field_id, rr));
	      AT acc(inst, field_id, rr);
	      char *dst = acc.ptr(rr.lo);
	      offsets[0] = ByteOffset(off_t(acc.strides[0]));
	      offsets[1] = ByteOffset(off_t(acc.strides[1]));
	      offsets[2] = ByteOffset(off_t(acc.strides[2]));
              offsets[3] = ByteOffset(off_t(acc.strides[3]));
              offsets[4] = ByteOffset(off_t(acc.strides[4]));
              offsets[5] = ByteOffset(off_t(acc.strides[5]));
	      subrect = r;
	      return dst;
	    }
#endif
#if LEGION_MAX_DIM >= 7
	    case 7: {
	      typedef Realm::AffineAccessor<char, 7, Arrays::coord_t> AT;
	      Realm::Rect<7, Arrays::coord_t> rr;
	      rr.lo = Realm::Point<7, Arrays::coord_t>(r.lo.x);
	      rr.hi = Realm::Point<7, Arrays::coord_t>(r.hi.x);
	      assert(AT::is_compatible(inst, field_id, rr));
	      AT acc(inst, field_id, rr);
	      char *dst = acc.ptr(rr.lo);
	      offsets[0] = ByteOffset(off_t(acc.strides[0]));
	      offsets[1] = ByteOffset(off_t(acc.strides[1]));
	      offsets[2] = ByteOffset(off_t(acc.strides[2]));
              offsets[3] = ByteOffset(off_t(acc.strides[3]));
              offsets[4] = ByteOffset(off_t(acc.strides[4]));
              offsets[5] = ByteOffset(off_t(acc.strides[5]));
              offsets[6] = ByteOffset(off_t(acc.strides[6]));
	      subrect = r;
	      return dst;
	    }
#endif
#if LEGION_MAX_DIM >= 8
	    case 8: {
	      typedef Realm::AffineAccessor<char, 8, Arrays::coord_t> AT;
	      Realm::Rect<8, Arrays::coord_t> rr;
	      rr.lo = Realm::Point<8, Arrays::coord_t>(r.lo.x);
	      rr.hi = Realm::Point<8, Arrays::coord_t>(r.hi.x);
	      assert(AT::is_compatible(inst, field_id, rr));
	      AT acc(inst, field_id, rr);
	      char *dst = acc.ptr(rr.lo);
	      offsets[0] = ByteOffset(off_t(acc.strides[0]));
	      offsets[1] = ByteOffset(off_t(acc.strides[1]));
	      offsets[2] = ByteOffset(off_t(acc.strides[2]));
              offsets[3] = ByteOffset(off_t(acc.strides[3]));
              offsets[4] = ByteOffset(off_t(acc.strides[4]));
              offsets[5] = ByteOffset(off_t(acc.strides[5]));
              offsets[6] = ByteOffset(off_t(acc.strides[6]));
              offsets[7] = ByteOffset(off_t(acc.strides[7]));
	      subrect = r;
	      return dst;
	    }
#endif
#if LEGION_MAX_DIM >= 9
	    case 9: {
	      typedef Realm::AffineAccessor<char, 9, Arrays::coord_t> AT;
	      Realm::Rect<9, Arrays::coord_t> rr;
	      rr.lo = Realm::Point<9, Arrays::coord_t>(r.lo.x);
	      rr.hi = Realm::Point<9, Arrays::coord_t>(r.hi.x);
	      assert(AT::is_compatible(inst, field_id, rr));
	      AT acc(inst, field_id, rr);
	      char *dst = acc.ptr(rr.lo);
	      offsets[0] = ByteOffset(off_t(acc.strides[0]));
	      offsets[1] = ByteOffset(off_t(acc.strides[1]));
	      offsets[2] = ByteOffset(off_t(acc.strides[2]));
              offsets[3] = ByteOffset(off_t(acc.strides[3]));
              offsets[4] = ByteOffset(off_t(acc.strides[4]));
              offsets[5] = ByteOffset(off_t(acc.strides[5]));
              offsets[6] = ByteOffset(off_t(acc.strides[6]));
              offsets[7] = ByteOffset(off_t(acc.strides[7]));
              offsets[8] = ByteOffset(off_t(acc.strides[8]));
	      subrect = r;
	      return dst;
	    }
#endif
	    default: assert(0);
	    }
	    return 0;
	  }

	  template <int DIM>
	  void *raw_rect_ptr(const LegionRuntime::Arrays::Rect<DIM>& r, LegionRuntime::Arrays::Rect<DIM> &subrect, ByteOffset *offsets,
			     const std::vector<off_t> &field_offsets, ByteOffset &field_stride) const
	  {
	    if(field_offsets.empty()) return 0;
	    void *ptr = RegionAccessor<Generic, void, void>(Generic::Untyped(inst, field_offsets[0])).raw_rect_ptr<DIM>(r, subrect, offsets);
	    if(field_offsets.size() == 1) {
	      field_stride.offset = 0;
	    } else {
	      for(size_t i = 1; i < field_offsets.size(); i++) {
		Arrays::Rect<DIM> subrect2;
		ByteOffset offsets2[DIM];
		void *ptr2 = RegionAccessor<Generic, void, void>(Generic::Untyped(inst, field_offsets[i])).raw_rect_ptr<DIM>(r, subrect2, offsets2);
		assert(ptr2 != 0);
		assert(subrect2 == subrect);
		for(int j = 0; j < DIM; j++)
		  assert(offsets2[j] == offsets[j]);
		ByteOffset stride(ptr2, ptr);
		if(i == 1) {
		  field_stride = stride;
		} else {
		  assert(stride.offset == (field_stride.offset * i));
		}
	      }
	    }
	    return ptr;
	  }

	  template <int DIM>
	  void *raw_dense_ptr(const LegionRuntime::Arrays::Rect<DIM>& r, LegionRuntime::Arrays::Rect<DIM> &subrect, ByteOffset &elem_stride) const
	  {
	    ByteOffset strides[DIM];
	    void *ptr = raw_rect_ptr<DIM>(r, subrect, strides);
	    elem_stride = strides[0];
	    for(int i = 1; i < DIM; i++)
	      assert(strides[i].offset == (strides[i - 1].offset *
					   (subrect.hi.x[i] - subrect.lo.x[i] + 1)));
	    return ptr;
	  }

	  template <int DIM>
	  void *raw_dense_ptr(const LegionRuntime::Arrays::Rect<DIM>& r, LegionRuntime::Arrays::Rect<DIM> &subrect, ByteOffset &elem_stride,
			      const std::vector<off_t> &field_offsets, ByteOffset &field_stride) const
	  {
	    ByteOffset strides[DIM];
	    void *ptr = raw_dense_ptr<DIM>(r, subrect, strides, field_offsets, field_stride);
	    elem_stride = strides[0];
	    for(int i = 1; i < DIM; i++)
	      assert(strides[i].offset == (strides[i - 1].offset *
					   (subrect.hi.x[i] - subrect.lo.x[i] + 1)));
	    return ptr;
	  }

	  Realm::RegionInstance inst;
	  Realm::FieldID field_id;
#if defined(LEGION_PRIVILEGE_CHECKS) || defined(LEGION_BOUNDS_CHECKS) 
        protected:
          void *region;
        public:
          inline void set_region_untyped(void *r) { region = r; }
#endif
#ifdef LEGION_PRIVILEGE_CHECKS
        protected:
          AccessorPrivilege priv;
        public:
          inline void set_privileges_untyped(AccessorPrivilege p) { priv = p; }
#endif
	public:
	  bool get_aos_parameters(void *& base, size_t& stride) const
	  {
	    return false;
	  }

	  bool get_soa_parameters(void *& base, size_t& stride) const
	  {
	    ByteOffset offset;
	    base = raw_rect_ptr<1>(&offset);
	    if(!base) return false;
	    if(stride == 0) {
	      stride = offset.offset;
	    } else {
	      if(stride != (size_t)offset.offset)
		return false;
	    }
	    return true;
	  }

	  bool get_hybrid_soa_parameters(void *& base, size_t& stride,
					 size_t& block_size, size_t& block_stride) const
	  {
	    return false;
	  }

	  bool get_redfold_parameters(void *& base) const
	  {
	    // this has always assumed that the stride is ok
	    ByteOffset offset;
	    base = raw_rect_ptr<1>(&offset);
	    if(!base) return false;
	    return true;
	  }

	  bool get_redlist_parameters(void *& base, ptr_t *& next_ptr) const
	  {
	    return false;
	  }
	};

	// empty class that will have stuff put in it later if T is a struct
	template <typename T, typename PT, bool B> struct StructSpecific {};

	template <typename T, typename PT>
	struct Typed : public Untyped, public StructSpecific<T, PT, TemplateFu::IsAStruct<T>::value> {
	  Typed() : Untyped() {}
	  Typed(Realm::RegionInstance _inst, Realm::FieldID _field_id = 0)
	    : Untyped(_inst, _field_id)
	  {}

          bool valid(void) const { return inst.exists(); }

#if defined(LEGION_PRIVILEGE_CHECKS) || defined(LEGION_BOUNDS_CHECKS)
          inline void set_region(void *r) { region = r; }
#endif
#ifdef LEGION_PRIVILEGE_CHECKS
          inline void set_privileges(AccessorPrivilege p) { priv = p; }
#endif
	  template <typename PTRTYPE>
	  inline T read(PTRTYPE ptr) const 
          { 
#ifdef __OPTIMIZE__
            issue_performance_warning(); 
#endif
#ifdef LEGION_PRIVILEGE_CHECKS
            check_privileges<ACCESSOR_READ>(priv, region);
#endif
#ifdef LEGION_BOUNDS_CHECKS 
            DebugHooks::check_bounds(region, ptr);
#endif
            T val; read_untyped(ptr, &val, sizeof(val)); return val; 
          }

	  template <typename PTRTYPE>
	  inline void write(PTRTYPE ptr, const T& newval) const 
          { 
#ifdef __OPTIMIZE__
            issue_performance_warning(); 
#endif
#ifdef LEGION_PRIVILEGE_CHECKS
            check_privileges<ACCESSOR_WRITE>(priv, region);
#endif
#ifdef LEGION_BOUNDS_CHECKS 
            DebugHooks::check_bounds(region, ptr);
#endif
            write_untyped(ptr, &newval, sizeof(newval)); 
          }
          template<typename REDOP, typename PTRTYPE>
	  inline void reduce(PTRTYPE ptr, typename REDOP::RHS newval) const
	  {
#ifdef __OPTIMIZE__
            issue_performance_warning(); 
#endif
#ifdef LEGION_PRIVILEGE_CHECKS
            check_privileges<ACCESSOR_REDUCE>(priv, region);
#endif
#ifdef CHECK_BOUNDS 
            DebugHooks::check_bounds(region, ptr);
#endif
	    T val = read(ptr);
	    REDOP::template apply<true>(val, newval);
	    write(ptr, val);
	  }

	  void report_fault(ptr_t ptr) const
	  {
	    Untyped::report_fault(ptr, sizeof(T));
	  }

	  void report_fault(const Legion::DomainPoint& dp) const
	  {
	    Untyped::report_fault(dp, sizeof(T));
	  }

	  T *raw_span_ptr(ptr_t ptr, size_t req_count, size_t& act_count, ByteOffset& offset) const
	  { return (T*)(Untyped::raw_span_ptr(ptr, req_count, act_count, offset)); }

	  // for whole instance - fails if not completely affine
	  template <int DIM>
	  T *raw_rect_ptr(ByteOffset *offsets) const
	  { return (T*)(Untyped::raw_rect_ptr<DIM>(offsets)); }

	  template <int DIM>
	  T *raw_rect_ptr(const LegionRuntime::Arrays::Rect<DIM>& r, LegionRuntime::Arrays::Rect<DIM> &subrect, ByteOffset *offsets) const
	  { return (T*)(Untyped::raw_rect_ptr<DIM>(r, subrect, offsets)); }

	  template <int DIM>
	    T *raw_rect_ptr(const LegionRuntime::Arrays::Rect<DIM>& r, LegionRuntime::Arrays::Rect<DIM> &subrect, ByteOffset *offsets,
			    const std::vector<off_t> &field_offsets, ByteOffset &field_stride) const
	  { return (T*)(Untyped::raw_rect_ptr<DIM>(r, subrect, offsets, field_offsets, field_stride)); }

	  template <int DIM>
	  T *raw_dense_ptr(const LegionRuntime::Arrays::Rect<DIM>& r, LegionRuntime::Arrays::Rect<DIM> &subrect, ByteOffset &elem_stride) const
	  { return (T*)(Untyped::raw_dense_ptr<DIM>(r, subrect, elem_stride)); }

	  typedef AOS<sizeof(PT)> AOS_TYPE;

	  template <typename AT>
	  bool can_convert(void) const {
	    return can_convert_helper<AT>(static_cast<AT *>(0));
	  }

	  template <typename AT>
	  RegionAccessor<AT, T> convert(void) const {
	    return convert_helper<AT>(static_cast<AT *>(0));
	  }

	  template <typename AT, size_t STRIDE>
	  bool can_convert_helper(AOS<STRIDE> *dummy) const {
	    //printf("in aos(%zd) converter\n", STRIDE);
	    void *aos_base = 0;
	    size_t aos_stride = STRIDE;
	    bool ok = get_aos_parameters(aos_base, aos_stride);
	    return ok;
	  }

	  template <typename AT, size_t STRIDE>
	  RegionAccessor<AT, T> convert_helper(AOS<STRIDE> *dummy) const {
	    //printf("in aos(%zd) converter\n", STRIDE);
	    void *aos_base = 0;
	    size_t aos_stride = STRIDE;
#ifndef NDEBUG
	    bool ok = 
#endif
              get_aos_parameters(aos_base, aos_stride);
	    assert(ok);
	    typename AT::template Typed<T, T> t(aos_base, aos_stride);
            RegionAccessor<AT, T> result(t);
#if defined(LEGION_PRIVILEGE_CHECKS) || defined(LEGION_BOUNDS_CHECKS) 
            result.set_region(region);
#endif
#ifdef LEGION_PRIVILEGE_CHECKS
            result.set_privileges(priv);
#endif
            return result;
	  }
	      
	  template <typename AT, size_t STRIDE>
	  bool can_convert_helper(SOA<STRIDE> *dummy) const {
	    //printf("in soa(%zd) converter\n", STRIDE);
	    void *soa_base = 0;
	    size_t soa_stride = STRIDE;
	    bool ok = get_soa_parameters(soa_base, soa_stride);
	    return ok;
	  }

	  template <typename AT, size_t STRIDE>
	  RegionAccessor<AT, T> convert_helper(SOA<STRIDE> *dummy) const {
	    //printf("in soa(%zd) converter\n", STRIDE);
	    void *soa_base = 0;
	    size_t soa_stride = STRIDE;
#ifndef NDEBUG
	    bool ok =
#endif
	      get_soa_parameters(soa_base, soa_stride);
	    assert(ok);
	    typename AT::template Typed<T, T> t(soa_base, soa_stride);
            RegionAccessor<AT,T> result(t);
#if defined(LEGION_PRIVILEGE_CHECKS) || defined(LEGION_BOUNDS_CHECKS) 
            result.set_region(region);
#endif
#ifdef LEGION_PRIVILEGE_CHECKS
            result.set_privileges(priv);
#endif
            return result;
	  }
	      
	  template <typename AT, size_t STRIDE, size_t BLOCK_SIZE, size_t BLOCK_STRIDE>
	  bool can_convert_helper(HybridSOA<STRIDE, BLOCK_SIZE, BLOCK_STRIDE> *dummy) const {
	    //printf("in hybridsoa(%zd,%zd,%zd) converter\n", STRIDE, BLOCK_SIZE, BLOCK_STRIDE);
	    void *hybrid_soa_base = 0;
	    size_t hybrid_soa_stride = STRIDE;
	    size_t hybrid_soa_block_size = BLOCK_SIZE;
	    size_t hybrid_soa_block_stride = BLOCK_STRIDE;
	    bool ok = get_hybrid_soa_parameters(hybrid_soa_base, hybrid_soa_stride,
						hybrid_soa_block_size, hybrid_soa_block_stride);
	    return ok;
	  }

	  template <typename AT, size_t STRIDE, size_t BLOCK_SIZE, size_t BLOCK_STRIDE>
	  RegionAccessor<AT, T> convert_helper(HybridSOA<STRIDE, BLOCK_SIZE, BLOCK_STRIDE> *dummy) const {
	    //printf("in hybridsoa(%zd,%zd,%zd) converter\n", STRIDE, BLOCK_SIZE, BLOCK_STRIDE);
	    void *hybrid_soa_base = 0;
	    size_t hybrid_soa_stride = STRIDE;
	    size_t hybrid_soa_block_size = BLOCK_SIZE;
	    size_t hybrid_soa_block_stride = BLOCK_STRIDE;
#ifndef NDEBUG
	    bool ok =
#endif
	      get_hybrid_soa_parameters(hybrid_soa_base, hybrid_soa_stride,
						hybrid_soa_block_size, hybrid_soa_block_stride);
	    assert(ok);
	    typename AT::template Typed<T, T> t(hybrid_soa_base, hybrid_soa_stride,
						hybrid_soa_block_size, hybrid_soa_block_stride);
            RegionAccessor<AT, T> result(t);
#if defined(LEGION_PRIVILEGE_CHECKS) || defined(LEGION_BOUNDS_CHECKS)
            result.set_region(region);
#endif
#ifdef LEGION_PRIVILEGE_CHECKS
            result.set_privileges(priv);
#endif
            return result;
	  }

	  template <typename AT, unsigned DIM>
	  bool can_convert_helper(Affine<DIM> *dummy) const {
	    ByteOffset offsets[DIM];
	    T *ptr = raw_rect_ptr<DIM>(offsets);
	    bool ok = (ptr != 0);
	    return ok;
	  }

	  template <typename AT, unsigned DIM>
	  RegionAccessor<Affine<DIM>, T> convert_helper(Affine<DIM> *dummy) const {
	    RegionAccessor<Affine<DIM>, T> result;
	    T *ptr = raw_rect_ptr<DIM>(result.strides);
	    bool ok = (ptr != 0);
	    assert(ok);
	    result.base = ptr;
	    return result;
	  }

	  template <typename AT, typename REDOP>
	  bool can_convert_helper(ReductionFold<REDOP> *dummy) const {
	    void *redfold_base = 0;
	    bool ok = get_redfold_parameters(redfold_base);
	    return ok;
	  }

	  template <typename AT, typename REDOP>
	  RegionAccessor<AT, T> convert_helper(ReductionFold<REDOP> *dummy) const {
	    void *redfold_base = 0;
#ifndef NDEBUG
	    bool ok =
#endif
	      get_redfold_parameters(redfold_base);
	    assert(ok);
	    typename AT::template Typed<T, T> t(redfold_base);
            RegionAccessor<AT, T> result(t);
#if defined(LEGION_PRIVILEGE_CHECKS) || defined(LEGION_BOUNDS_CHECKS)
            result.set_region(region);
#endif
#ifdef LEGION_PRIVILEGE_CHECKS
            result.set_privileges(priv);
#endif
            return result;
	  }

	  template <typename AT, typename REDOP>
	  bool can_convert_helper(ReductionList<REDOP> *dummy) const {
	    void *redlist_base = 0;
	    ptr_t *redlist_next_ptr = 0;
	    bool ok = get_redlist_parameters(redlist_base, redlist_next_ptr);
	    return ok;
	  }

	  template <typename AT, typename REDOP>
	  RegionAccessor<AT, T> convert_helper(ReductionList<REDOP> *dummy) const {
	    void *redlist_base = 0;
	    ptr_t *redlist_next_ptr = 0;
#ifndef NDEBUG
	    bool ok =
#endif
	      get_redlist_parameters(redlist_base, redlist_next_ptr);
	    assert(ok);
	    typename AT::template Typed<T, T> t(redlist_base, redlist_next_ptr);
            RegionAccessor<AT, T> result(t);
#if defined(LEGION_PRIVILEGE_CHECKS) || defined(LEGION_BOUNDS_CHECKS) 
            result.set_region(region);
#endif
#ifdef LEGION_PRIVILEGE_CHECKS
            result.set_privileges(priv);
#endif
            return result;
	  }
	};
      };

      template <typename T, typename PT> 
      struct Generic::StructSpecific<T, PT, true> {
	template <typename FT>
	RegionAccessor<Generic, FT, PT> get_field_accessor(FT T::* ptr) { 
	  Generic::Typed<T, PT> *rthis = static_cast<Generic::Typed<T, PT> *>(this);
	  return RegionAccessor<Generic, FT, PT>(Typed<FT, PT>(rthis->internal,
							       rthis->field_offset +
							       ((off_t)&(((T *)0)->*ptr))));
	}
      };

      template <size_t STRIDE> struct Stride : public Const<size_t, STRIDE> {
        CUDAPREFIX
        Stride(void) {}
        CUDAPREFIX
        Stride(size_t _value) : Const<size_t, STRIDE>(_value) {}
      };

      template <size_t BLOCK_SIZE> struct BlockSize : public Const<size_t, BLOCK_SIZE> {
        CUDAPREFIX
        BlockSize(size_t _value) : Const<size_t, BLOCK_SIZE>(_value) {}
      };

      template <size_t BLOCK_STRIDE> struct BlockStride : public Const<size_t, BLOCK_STRIDE> {
        CUDAPREFIX
        BlockStride(size_t _value) : Const<size_t, BLOCK_STRIDE>(_value) {}
      };

      template <size_t STRIDE> 
      struct AOS {
	struct Untyped : public Stride<STRIDE> {
	  Untyped() : Stride<STRIDE>(), base(0) {}
	  Untyped(void *_base, size_t _stride) : Stride<STRIDE>(_stride), base((char *)_base) {}
	  
          CUDAPREFIX
	  inline char *elem_ptr(ptr_t ptr) const
	  {
#ifdef LEGION_BOUNDS_CHECKS 
            DebugHooks::check_bounds(region, ptr);
#endif
	    return(base + (ptr.value * Stride<STRIDE>::value));
	  }
	  //char *elem_ptr(const Legion::DomainPoint& dp) const;
	  //char *elem_ptr_linear(const Legion::Domain& d, Legion::Domain& subrect, ByteOffset *offsets);

	  char *base;
#if defined(LEGION_PRIVILEGE_CHECKS) || defined(LEGION_BOUNDS_CHECKS)
        protected:
          void *region;
        public:
          inline void set_region_untyped(void *r) { region = r; }
#endif
#ifdef LEGION_PRIVILEGE_CHECKS
        protected:
          AccessorPrivilege priv;
        public:
          inline void set_privileges_untyped(AccessorPrivilege p) { priv = p; }
#endif
	};

	template <typename T, typename PT>
	struct Typed : protected Untyped {
	  Typed() : Untyped() {}
	  Typed(void *_base, size_t _stride) : Untyped(_base, _stride) {}

#if defined(LEGION_PRIVILEGE_CHECKS) || defined(LEGION_BOUNDS_CHECKS) 
          inline void set_region(void *r) { this->region = r; }
#endif
#ifdef LEGION_PRIVILEGE_CHECKS
          inline void set_privileges(AccessorPrivilege p) { this->priv = p; }
#endif

          CUDAPREFIX
	  inline T read(ptr_t ptr) const 
          { 
#ifdef LEGION_PRIVILEGE_CHECKS
            check_privileges<ACCESSOR_READ>(this->template priv, this->region);
#endif
#ifdef LEGION_BOUNDS_CHECKS 
            DebugHooks::check_bounds(this->region, ptr);
#endif
            return *(const T *)(Untyped::elem_ptr(ptr)); 
          }
          CUDAPREFIX
	  inline void write(ptr_t ptr, const T& newval) const 
          { 
#ifdef LEGION_PRIVILEGE_CHECKS
            check_privileges<ACCESSOR_WRITE>(this->template priv, this->region);
#endif
#ifdef LEGION_BOUNDS_CHECKS 
            DebugHooks::check_bounds(this->region, ptr);
#endif
            *(T *)(Untyped::elem_ptr(ptr)) = newval; 
          }
          CUDAPREFIX
	  inline T *ptr(ptr_t ptr) const 
          { 
#ifdef LEGION_PRIVILEGE_CHECKS
            // Don't check privileges when getting pointers
            //check_privileges<ACCESSOR_WRITE>(this->template priv, this->region);
#endif
#ifdef LEGION_BOUNDS_CHECKS 
            DebugHooks::check_bounds(this->region, ptr);
#endif
            return (T *)Untyped::elem_ptr(ptr); 
          }
          CUDAPREFIX
          inline T& ref(ptr_t ptr) const 
          { 
#ifdef LEGION_PRIVILEGE_CHECKS
            // Don't check privileges when getting references
            //check_privileges<ACCESSOR_WRITE>(this->template priv, this->region);
#endif
#ifdef LEGION_BOUNDS_CHECKS 
            DebugHooks::check_bounds(this->region, ptr);
#endif
            return *((T*)Untyped::elem_ptr(ptr)); 
          }

	  template<typename REDOP> CUDAPREFIX
	  inline void reduce(ptr_t ptr, typename REDOP::RHS newval) const
	  {
#ifdef LEGION_PRIVILEGE_CHECKS
            check_privileges<ACCESSOR_REDUCE>(this->template priv, this->region);
#endif
#ifdef LEGION_BOUNDS_CHECKS 
            DebugHooks::check_bounds(this->region, ptr);
#endif
	    REDOP::template apply<false>(*(T *)Untyped::elem_ptr(ptr), newval);
	  }

	  //T *elem_ptr(const Legion::DomainPoint& dp) const { return (T*)(Untyped::elem_ptr(dp)); }
	  //T *elem_ptr_linear(const Legion::Domain& d, Legion::Domain& subrect, ByteOffset *offsets)
	  //{ return (T*)(Untyped::elem_ptr_linear(d, subrect, offsets)); }
	};
      };

      template <size_t STRIDE> 
      struct SOA {
	struct Untyped : public Stride<STRIDE> {
	  Untyped() : Stride<STRIDE>(STRIDE), base(0) {}
	  Untyped(void *_base, size_t _stride) : Stride<STRIDE>(_stride), base((char *)_base) {}
	  
          CUDAPREFIX
	  inline char *elem_ptr(ptr_t ptr) const
	  {
	    return(base + (ptr.value * Stride<STRIDE>::value));
	  }

	  char *base;
#if defined(LEGION_PRIVILEGE_CHECKS) || defined(LEGION_BOUNDS_CHECKS)
        protected:
          // TODO: Make pointer checks work on the GPU
          void *region;
        public:
          inline void set_region_untyped(void *r) { region = r; }
#endif
#ifdef LEGION_PRIVILEGE_CHECKS
        protected:
          AccessorPrivilege priv;
        public:
          inline void set_privileges_untyped(AccessorPrivilege p) { priv = p; }
#endif
	};

	template <typename T, typename PT>
	struct Typed : protected Untyped {
	  Typed() : Untyped() {}
	  Typed(void *_base, size_t _stride) : Untyped(_base, _stride) {}
	  Typed(const Typed& other): Untyped(other.base, other.Stride<STRIDE>::value) {
#if defined(LEGION_PRIVILEGE_CHECKS) || defined(LEGION_BOUNDS_CHECKS)
            this->set_region(other.region);
#endif
#ifdef LEGION_PRIVILEGE_CHECKS
            this->set_privileges(other.priv);
#endif
          }
	  CUDAPREFIX
	  Typed &operator=(const Typed& rhs) {
	    *static_cast<Untyped *>(this) = *static_cast<const Untyped *>(&rhs);
	    return *this;
	  }

#if defined(LEGION_PRIVILEGE_CHECKS) || defined(LEGION_BOUNDS_CHECKS) 
          inline void set_region(void *r) { this->region = r; }
#endif
#ifdef LEGION_PRIVILEGE_CHECKS
          inline void set_privileges(AccessorPrivilege p) { this->priv = p; }
#endif

          CUDAPREFIX
	  inline T read(ptr_t ptr) const 
          { 
#ifdef LEGION_PRIVILEGE_CHECKS
            check_privileges<ACCESSOR_READ>(this->template priv, this->region);
#endif
#ifdef LEGION_BOUNDS_CHECKS 
            DebugHooks::check_bounds(this->region, ptr);
#endif
            return *(const T *)(Untyped::elem_ptr(ptr)); 
          }
          CUDAPREFIX
	  inline void write(ptr_t ptr, const T& newval) const 
          { 
#ifdef LEGION_PRIVILEGE_CHECKS
            check_privileges<ACCESSOR_WRITE>(this->template priv, this->region);
#endif
#ifdef LEGION_BOUNDS_CHECKS 
            DebugHooks::check_bounds(this->region, ptr);
#endif
            *(T *)(Untyped::elem_ptr(ptr)) = newval; 
          }
          CUDAPREFIX
	  inline T *ptr(ptr_t ptr) const 
          { 
#ifdef LEGION_PRIVILEGE_CHECKS
            // Don't check privileges on pointers 
            //check_privileges<ACCESSOR_WRITE>(this->template priv, this->region);
#endif
#ifdef LEGION_BOUNDS_CHECKS 
            DebugHooks::check_bounds(this->region, ptr);
#endif
            return (T *)Untyped::elem_ptr(ptr); 
          }
          CUDAPREFIX
          inline T& ref(ptr_t ptr) const 
          { 
#ifdef LEGION_PRIVILEGE_CHECKS
            // Don't check privileges on references
            //check_privileges<ACCESSOR_WRITE>(this->template priv, this->region);
#endif
#ifdef LEGION_BOUNDS_CHECKS 
            DebugHooks::check_bounds(this->region, ptr);
#endif
            return *((T*)Untyped::elem_ptr(ptr)); 
          }

	  template<typename REDOP> CUDAPREFIX
	  inline void reduce(ptr_t ptr, typename REDOP::RHS newval) const
	  {
#ifdef LEGION_PRIVILEGE_CHECKS
            check_privileges<ACCESSOR_REDUCE>(this->template priv, this->region);
#endif
#ifdef LEGION_BOUNDS_CHECKS 
            DebugHooks::check_bounds(this->region, ptr);
#endif
	    REDOP::template apply<false>(*(T *)Untyped::elem_ptr(ptr), newval);
	  }
	};
      };

      template <size_t STRIDE, size_t BLOCK_SIZE, size_t BLOCK_STRIDE> 
      struct HybridSOA {
	struct Untyped : public Stride<STRIDE>, public BlockSize<BLOCK_SIZE>, public BlockStride<BLOCK_STRIDE> {
	  Untyped() : Stride<STRIDE>(0), BlockSize<BLOCK_SIZE>(0), BlockStride<BLOCK_STRIDE>(0), base(0) {}
          Untyped(void *_base, size_t _stride, size_t _block_size, size_t _block_stride) 
	  : Stride<STRIDE>(_stride), BlockSize<BLOCK_SIZE>(_block_size),
	    BlockStride<BLOCK_STRIDE>(_block_stride), base((char *)_base) {}
	  
          CUDAPREFIX
	  inline char *elem_ptr(ptr_t ptr) const
	  {
#ifdef LEGION_BOUNDS_CHECKS 
            DebugHooks::check_bounds(region, ptr);
#endif
	    return(base + (ptr.value * Stride<STRIDE>::value));
	  }

	  char *base;
#if defined(LEGION_PRIVILEGE_CHECKS) || defined(LEGION_BOUNDS_CHECKS)
        protected:
          void *region;
          // TODO: Make this work for GPUs
        public:
          inline void set_region_untyped(void *r) { region = r; }
#endif
#ifdef LEGION_PRIVILEGE_CHECKS
        protected:
          AccessorPrivilege priv;
        public:
          inline void set_privileges_untyped(AccessorPrivilege p) { priv = p; }
#endif
	};

	template <typename T, typename PT>
	struct Typed : protected Untyped {
	  Typed() : Untyped() {}
	  Typed(void *_base, size_t _stride, size_t _block_size, size_t _block_stride)
	    : Untyped(_base, _stride, _block_size, _block_stride) {}

#if defined(LEGION_PRIVILEGE_CHECKS) || defined(LEGION_BOUNDS_CHECKS)
          inline void set_region(void *r) { this->region = r; }
#endif
#ifdef LEGION_PRIVILEGE_CHECKS
          inline void set_privileges(AccessorPrivilege p) { this->priv = p; }
#endif

          CUDAPREFIX
	  inline T read(ptr_t ptr) const 
          { 
#ifdef LEGION_PRIVILEGE_CHECKS
            check_privileges<ACCESSOR_READ>(this->template priv, this->region);
#endif
#ifdef LEGION_BOUNDS_CHECKS 
            DebugHooks::check_bounds(this->region, ptr);
#endif
            return *(const T *)(Untyped::elem_ptr(ptr)); 
          }
          CUDAPREFIX
	  inline void write(ptr_t ptr, const T& newval) const 
          { 
#ifdef LEGION_PRIVILEGE_CHECKS
            check_privileges<ACCESSOR_WRITE>(this->template priv, this->region);
#endif
#ifdef LEGION_BOUNDS_CHECKS 
            DebugHooks::check_bounds(this->region, ptr);
#endif
            *(T *)(Untyped::elem_ptr(ptr)) = newval; 
          }
          CUDAPREFIX
	  inline T *ptr(ptr_t ptr) const 
          { 
#ifdef LEGION_PRIVILEGE_CHECKS
            // Don't check privileges on pointers
            //check_privileges<ACCESSOR_WRITE>(this->template priv, this->region);
#endif
#ifdef LEGION_BOUNDS_CHECKS 
            DebugHooks::check_bounds(this->region, ptr);
#endif
            return (T *)Untyped::elem_ptr(ptr); 
          }
          CUDAPREFIX
          inline T& ref(ptr_t ptr) const 
          { 
#ifdef LEGION_PRIVILEGE_CHECKS
            // Don't check privileges on references
            //check_privileges<ACCESSOR_WRITE>(this->template priv, this->region);
#endif
#ifdef LEGION_BOUNDS_CHECKS 
            DebugHooks::check_bounds(this->region, ptr);
#endif
            return *((T*)Untyped::elem_ptr(ptr)); 
          }

	  template<typename REDOP> CUDAPREFIX
	  inline void reduce(ptr_t ptr, typename REDOP::RHS newval) const
	  {
#ifdef LEGION_PRIVILEGE_CHECKS
            check_privileges<ACCESSOR_REDUCE>(this->template priv, this->region);
#endif
#ifdef LEGION_BOUNDS_CHECKS 
            DebugHooks::check_bounds(this->region, ptr);
#endif
	    REDOP::template apply<false>(*(T *)Untyped::elem_ptr(ptr), newval);
	  }
	};
      };

      template <unsigned DIM>
      struct Affine {
	struct Untyped {
	  Untyped(void) : base(0) {}

          CUDAPREFIX
	  inline void *elem_ptr(const LegionRuntime::Arrays::Point<DIM>& p)
	  {
	    void *ptr = base;
	    for(unsigned i = 0; i < DIM; i++)
	      ptr += strides[i] * p.x[i];
#if 0
	    printf("AFF[%p, (%d", this, p.x[0]);
	    for(unsigned i = 1; i < DIM; i++)
	      printf(", %d", p.x[i]);
	    printf(") -> %p\n", ptr);
#endif
	    return ptr;
	  }

	  void *base;
	  ByteOffset strides[DIM];
	};

	template <typename T, typename PT>
	struct Typed : public Untyped {
          CUDAPREFIX
	  Typed(void) : Untyped() {}

          CUDAPREFIX
	  inline T& ref(const LegionRuntime::Arrays::Point<DIM>& p)
	  {
	    return *(T *)(Untyped::elem_ptr(p));
	  }

          CUDAPREFIX
	  inline T& operator[](const LegionRuntime::Arrays::Point<DIM>& p)
	  {
	    return ref(p);
	  }

          CUDAPREFIX
	  inline T read(const LegionRuntime::Arrays::Point<DIM>& p)
	  {
	    return ref(p);
	  }

          CUDAPREFIX
	  inline void write(const LegionRuntime::Arrays::Point<DIM>& p, T newval)
	  {
	    ref(p) = newval;
	  }
	};
      };

      template <typename REDOP>
      struct ReductionFold {
	struct Untyped {
	  Untyped() : base(0) {}
	  Untyped(void *_base) : base((char *)_base) {}
	  
          CUDAPREFIX
	  inline char *elem_ptr(ptr_t ptr) const
	  {
#ifdef LEGION_BOUNDS_CHECKS 
            DebugHooks::check_bounds(this->region, ptr);
#endif
	    return(base + (ptr.value * sizeof(typename REDOP::RHS)));
	  }

	  char *base;
#if defined(LEGION_PRIVILEGE_CHECKS) || defined(LEGION_BOUNDS_CHECKS) 
        protected:
          void *region;
        public:
          inline void set_region_untyped(void *r) { region = r; }
#endif
#ifdef LEGION_PRIVILEGE_CHECKS
        protected:
          AccessorPrivilege priv;
        public:
          inline void set_privileges_untyped(AccessorPrivilege p) 
          { 
            assert((p == ACCESSOR_NONE) || (p == ACCESSOR_REDUCE));
            priv = p; 
          }
#endif
	};

	template <typename T, typename PT>
	struct Typed : protected Untyped {
	  Typed(void) : Untyped() {}
	  Typed(void *_base) : Untyped(_base) {}

#if defined(LEGION_PRIVILEGE_CHECKS) || defined(LEGION_BOUNDS_CHECKS)
          inline void set_region(void *r) { this->region = r; }
#endif
#ifdef LEGION_PRIVILEGE_CHECKS
          inline void set_privileges(AccessorPrivilege p) 
          { 
            assert((p == ACCESSOR_NONE) || (p == ACCESSOR_REDUCE));
            this->priv = p; 
          }
#endif

	  // only allowed operation on a reduction fold instance is a reduce (fold)
          CUDAPREFIX
	  inline void reduce(ptr_t ptr, typename REDOP::RHS newval) const
	  {
#ifdef LEGION_PRIVILEGE_CHECKS
            check_privileges<ACCESSOR_REDUCE>(this->template priv, this->region);
#endif
#ifdef LEGION_BOUNDS_CHECKS 
            DebugHooks::check_bounds(this->region, ptr);
#endif
	    REDOP::template fold<false>(*(typename REDOP::RHS *)Untyped::elem_ptr(ptr), newval);
	  }
	};
      };

      template <typename REDOP>
      struct ReductionList {
	struct ReductionListEntry {
	  ptr_t ptr;
	  typename REDOP::RHS rhs;
	};

	struct Untyped {
	Untyped() : base(0), next_entry(0) {}
	Untyped(void *_base, ptr_t *_next_entry) : base((char *)_base), next_entry(_next_entry) {}
	  
	  inline char *elem_ptr(ptr_t ptr) const
	  {
#ifdef LEGION_BOUNDS_CHECKS 
            DebugHooks::check_bounds(this->region, ptr);
#endif
	    return(base + (ptr.value * sizeof(ReductionListEntry)));
	  }

	  inline ptr_t get_next(void) const
	  {
	    ptr_t n;
#ifdef __GNUC__
	    n.value = __sync_fetch_and_add(&(next_entry->value), 1);
#else
            n.value = LowLevel::__sync_fetch_and_add(&(next_entry->value), 1); 
#endif
	    return n;
	  }

	  char *base;
	  ptr_t *next_entry;
#if defined(LEGION_PRIVILEGE_CHECKS) || defined(LEGION_BOUNDS_CHECKS)
        protected:
          void *region;
        public:
          inline void set_region_untpyed(void *r) { region = r; }
#endif
#ifdef LEGION_PRIVILEGE_CHECKS
        protected:
          AccessorPrivilege priv;
        public:
          inline void set_untyped_privileges(AccessorPrivilege p) 
          { 
            assert((p == ACCESSOR_NONE) || (p == ACCESSOR_REDUCE));
            priv = p; 
          }
#endif
	};

	template <typename T, typename PT>
	struct Typed : protected Untyped {
	  Typed(void) : Untyped() {}
	  Typed(void *_base, ptr_t *_next_entry) : Untyped(_base, _next_entry) {}

#if defined(LEGION_PRIVILEGE_CHECKS) || defined(LEGION_BOUNDS_CHECKS)
        inline void set_region(void *r) { this->region = r; } 
#endif
#ifdef LEGION_PRIVILEGE_CHECKS
        inline void set_privileges(AccessorPrivilege p) 
        { 
          assert((p == ACCESSOR_NONE) || (p == ACCESSOR_REDUCE));
          this->priv = p; 
        }
#endif

	  // only allowed operation on a reduction list instance is a reduce
	  inline void reduce(ptr_t ptr, typename REDOP::RHS newval) const
	  {
#ifdef LEGION_PRIVILEGE_CHECKS
            check_privileges<ACCESSOR_REDUCE>(this->template priv, this->region);
#endif
#ifdef LEGION_BOUNDS_CHECKS 
            DebugHooks::check_bounds(this->region, ptr);
#endif
	    ptr_t listptr = Untyped::get_next();
	    ReductionListEntry *entry = reinterpret_cast<ReductionListEntry *>(Untyped::elem_ptr(listptr));
	    entry->ptr = ptr;
	    entry->rhs = newval;
	  }
	};
      };
    };

    template <typename AT, typename ET, typename PT> 
    struct RegionAccessor : public AT::template Typed<ET, PT> {
      RegionAccessor()
	: AT::template Typed<ET, PT>() {}
      RegionAccessor(const typename AT::template Typed<ET, PT>& to_copy) 
	: AT::template Typed<ET, PT>(to_copy) {}

      template <typename FT>
      struct FieldAccessor : 
        public AT::template Typed<ET, PT>::template Field<FT> {
	FieldAccessor(void) {}

	//FieldAccessor(const typename AT::template Inner<ET, PT>::template Field<FT>& to_copy) {}
      };
    };
  };
};

#undef CUDAPREFIX
      
#endif
