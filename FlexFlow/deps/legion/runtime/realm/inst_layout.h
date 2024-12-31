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

// Layout descriptors for Realm RegionInstances

#ifndef REALM_INST_LAYOUT_H
#define REALM_INST_LAYOUT_H

#include "realm/indexspace.h"
#include "realm/serialize.h"

#if defined(REALM_USE_KOKKOS) && (REALM_CXX_STANDARD >= 11)
// we don't want to include Kokkos_View.hpp because it brings in too much
//  other stuff, so forward declare the pieces we need to define a templated
//  conversion from Realm accessor to Kokkos::View (anything that actually
//  instantiates the template will presumably have included Kokkos_View.hpp
//  in its full glory)
namespace Kokkos {
  template <class, class...> class View;
  template <unsigned> struct MemoryTraits;
  struct LayoutStride;
  template <class, size_t, class> struct Array;
  namespace Experimental {
    template <class, class...> class OffsetView;
  };
};
// Kokkos::Unmanaged is an enum, which we can't forward declare - we'll test
//  that we have the right value in the template though
enum { Kokkos_Unmanaged = 0x01 };

#define REALM_PROVIDE_ACCESSOR_TO_KOKKOS_VIEW_CONVERSION
#endif

#include <vector>
#include <map>
#include <iostream>

/**
 * \file int_layout.h
 * This file contains definitions of various instance layouts and
 * accessors to read/write the underlying data.
 */

namespace Realm {

  /**
   * \class InstanceLayoutConstraints
   * This class is used to describe the layout constraints for a
   * RegionInstance.
   */
  class REALM_PUBLIC_API InstanceLayoutConstraints {
  public:
    InstanceLayoutConstraints(void) { }
    InstanceLayoutConstraints(const std::map<FieldID, size_t>& field_sizes,
			      size_t block_size);
    InstanceLayoutConstraints(const std::vector<size_t>& field_sizes,
			      size_t block_size);
    InstanceLayoutConstraints(const std::vector<FieldID>& field_ids,
			      const std::vector<size_t>& field_sizes,
			      size_t block_size);

    struct FieldInfo {
      FieldID field_id;
      bool fixed_offset;
      size_t offset;  // used if `fixed_offset` is true
      size_t size;
      size_t alignment;
    };
    typedef std::vector<FieldInfo> FieldGroup;

    std::vector<FieldGroup> field_groups;
  };

  namespace PieceLookup {

    class CompiledProgram {
    public:
      virtual ~CompiledProgram() {}

      // used during compilation to request memory to store instructions
      //  in - can only be used once
      virtual void *allocate_memory(size_t bytes) = 0;

      // after any changes to the contents of the memory, this must be
      //  called to ensure all devices see the changes
      virtual void commit_updates() = 0;

      struct PerField {
	const PieceLookup::Instruction *start_inst;
	unsigned inst_usage_mask;
	uintptr_t field_offset;
      };

      std::map<FieldID, PerField> fields;
    };

    // this is a namespace instead of an enum so that it can be extended elsewhere
    namespace Opcodes {
      typedef unsigned char Opcode;

      static const Opcode OP_INVALID = 0;
      static const Opcode OP_SPLIT1 = 1;  // this is a SplitPlane<N,T>
    }

    // some processors are limited in which instruction types they can
    //  support, so we build masks to describe usage/capabilities
    static const unsigned ALLOW_SPLIT1 = 1U << Opcodes::OP_SPLIT1;

    struct REALM_INTERNAL_API_EXTERNAL_LINKAGE Instruction {
      // all instructions are at least 4 bytes and aligned to 16 bytes, but
      //  the only data common to all is the opcode, which appears in the low
      //  8 bits
      REALM_ALIGNED_TYPE_CONST(uint32_aligned_16, uint32_t, 16);
      uint32_aligned_16 data;

      Instruction(uint32_t _data);

      REALM_CUDA_HD
      Opcodes::Opcode opcode() const;

      // two helper methods that get you to another instruction

      // skip() gets the next instruction in sequence, which requires knowing
      //  the size of the current instruction - instructions below take care
      //  of this in most cases
      REALM_CUDA_HD
      const Instruction *skip(size_t bytes) const;

      // jump(N) jumps forward in the instruction stream by N 16-B chunks
      //   special case: a delta of 0 is "end of program" and returns null
      REALM_CUDA_HD
      const Instruction *jump(unsigned delta) const;
    };

  };

  /**
   * \class InstanceLayoutGeneric
   * Instance layouts are templated on the type of the IndexSpace used to
   * index them, but they all inherit from a generic version.
   */
  class REALM_PUBLIC_API InstanceLayoutGeneric {
  protected:
    // cannot be created directly
    InstanceLayoutGeneric(void);

  public:
    template <typename S>
    static InstanceLayoutGeneric *deserialize_new(S& deserializer);

    virtual ~InstanceLayoutGeneric(void);
    
    virtual InstanceLayoutGeneric *clone(void) const = 0;

    /**
     * Adjusts offsets of all pieces by 'adjust_amt'
     * \param adjust_amt
     */
    virtual void relocate(size_t adjust_amt) = 0;

    virtual void print(std::ostream& os) const = 0;

    /**
     * Create an affine layout using the bounds of 'is' (i.e. one piece)
     * using the requested dimension ordering and respecting the field
     * size/alignment constraints provided.
     * \param is Index space to use for layout creation.
     * \param ilc Layout constraints to use for layout creation.
     * \param dim_order Dimension ordering to use for layout creation.
     * \return InstanceLayoutGeneric* Pointer to the created layout.
     */
    template <int N, typename T>
    static InstanceLayoutGeneric *choose_instance_layout(IndexSpace<N,T> is,
							 const InstanceLayoutConstraints& ilc,
                                                         const int dim_order[N]);

    /**
     * Create a multi-affine layout using one piece for each rectangle in
     * 'covering' using the requested dimension ordering and respecting the
     * field size/alignment constraints provided.
     * \param is Index space to use for layout creation.
     * \param covering Rectangles to use for layout creation.
     * \param ilc Layout constraints to use for layout creation.
     * \param dim_order Dimension ordering to use for layout creation.
     * \return InstanceLayoutGeneric* Pointer to the created layout.
     */
    template <int N, typename T>
    static InstanceLayoutGeneric *choose_instance_layout(IndexSpace<N,T> is,
							 const std::vector<Rect<N,T> >& covering,
							 const InstanceLayoutConstraints& ilc,
                                                         const int dim_order[N]);

    REALM_INTERNAL_API_EXTERNAL_LINKAGE
    virtual void compile_lookup_program(PieceLookup::CompiledProgram& p) const = 0;

    size_t bytes_used;
    size_t alignment_reqd;

    // we optimize for fields being laid out similarly, and have fields
    //  indirectly reference a piece list
    struct FieldLayout {
      int list_idx;
      size_t rel_offset;
      int size_in_bytes;
    };

    std::map<FieldID, FieldLayout> fields;
  };

  REALM_PUBLIC_API
  std::ostream& operator<<(std::ostream& os, const InstanceLayoutGeneric& ilg);

  /**
   * \class InstanceLayoutOpaque
   * Users that wish to handle instances as simple blocks of bits may use
   * an InstanceLayoutOpaque to just request a contiguous range of bytes with
   * a specified alignment.
   */
  class REALM_PUBLIC_API InstanceLayoutOpaque : public InstanceLayoutGeneric {
  public:
    InstanceLayoutOpaque(size_t _bytes_used, size_t _alignment_reqd);

    virtual InstanceLayoutGeneric *clone(void) const;
  };

  namespace PieceLayoutTypes {
    typedef unsigned char LayoutType;

    static const LayoutType InvalidLayoutType = 0;
    static const LayoutType AffineLayoutType = 1;
  };

  /**
   * \class InstanceLayoutPieceBase
   * Base class for all pieces of an instance layout.
   */
  class REALM_PUBLIC_API InstanceLayoutPieceBase {
  public:
    InstanceLayoutPieceBase(PieceLayoutTypes::LayoutType _layout_type);

    virtual ~InstanceLayoutPieceBase(void);

    virtual void relocate(size_t base_offset) = 0;

    virtual void print(std::ostream& os) const = 0;

    // used for constructing lookup programs
    virtual size_t lookup_inst_size() const = 0;
    virtual PieceLookup::Instruction *create_lookup_inst(void *ptr,
							 unsigned next_delta) const = 0;

    PieceLayoutTypes::LayoutType layout_type;
  };

  /**
   * \class InstanceLayoutPiece
   * A piece of an instance layout that is defined by a single rectangle.
   */
  template <int N, typename T = int>
  class REALM_PUBLIC_API InstanceLayoutPiece : public InstanceLayoutPieceBase {
  public:
    InstanceLayoutPiece(void);
    InstanceLayoutPiece(PieceLayoutTypes::LayoutType _layout_type);
    InstanceLayoutPiece(PieceLayoutTypes::LayoutType _layout_type,
			const Rect<N,T>& _bounds);

    template <typename S>
    static InstanceLayoutPiece<N,T> *deserialize_new(S& deserializer);

    virtual InstanceLayoutPiece<N,T> *clone(void) const = 0;

    virtual size_t calculate_offset(const Point<N,T>& p) const = 0;

    Rect<N,T> bounds;
  };

  template <int N, typename T>
  REALM_PUBLIC_API
  std::ostream& operator<<(std::ostream& os, const InstanceLayoutPiece<N,T>& ilp);

  /**
   * \class AffineLayoutPiece
   * An affine piece of an instance layout. Affine pieces are defined by a
   * single rectangle and a set of strides.
   */
  template <int N, typename T = int>
  class REALM_PUBLIC_API AffineLayoutPiece : public InstanceLayoutPiece<N,T> {
  public:
    AffineLayoutPiece(void);

    template <typename S>
    static InstanceLayoutPiece<N,T> *deserialize_new(S& deserializer);

    virtual InstanceLayoutPiece<N,T> *clone(void) const;

    /**
     * Calculate the offset of a point within the affine piece.
     * The offet is calculated as: offset + sum(strides[i] * p[i]).
     * \param p Point to calculate offset for.
     * \return size_t Offset of point.
     */
    virtual size_t calculate_offset(const Point<N,T>& p) const;

    virtual void relocate(size_t base_offset);

    virtual void print(std::ostream& os) const;

    // used for constructing lookup programs
    virtual size_t lookup_inst_size() const;
    virtual PieceLookup::Instruction *create_lookup_inst(void *ptr,
							 unsigned next_delta) const;

    static Serialization::PolymorphicSerdezSubclass<InstanceLayoutPiece<N,T>, AffineLayoutPiece<N,T> > serdez_subclass;

    template <typename S>
    bool serialize(S& serializer) const;

    Point<N, size_t> strides;
    size_t offset;
  };

  /**
   * \class IntancePieceList
   * A list of pieces that make up an instance layout.
   */
  template <int N, typename T = int>
  class REALM_PUBLIC_API InstancePieceList {
  public:
    InstancePieceList(void);
    ~InstancePieceList(void);

    InstancePieceList(const InstancePieceList &) = default;
    InstancePieceList &operator=(const InstancePieceList &) = default;
    InstancePieceList(InstancePieceList &&) noexcept = default;
    InstancePieceList &operator=(InstancePieceList &&) noexcept = default;

    const InstanceLayoutPiece<N,T> *find_piece(Point<N,T> p) const;

    void relocate(size_t base_offset);

    template <typename S>
    bool serialize(S& serializer) const;
    template <typename S>
    bool deserialize(S& deserializer);

    std::vector<InstanceLayoutPiece<N,T> *> pieces;
    // placeholder for lookup structure (e.g. K-D tree)
  };

  template <int N, typename T>
  REALM_PUBLIC_API
  std::ostream& operator<<(std::ostream& os, const InstancePieceList<N,T>& ipl);

  /**
   * \class InstanceLayout
   * A layout for an instance of a logical region.
   */
  template <int N, typename T = int>
  class REALM_PUBLIC_API InstanceLayout : public InstanceLayoutGeneric {
  public:
    InstanceLayout(void);

    template <typename S>
    static InstanceLayoutGeneric *deserialize_new(S& deserializer);

    virtual ~InstanceLayout(void);

    virtual InstanceLayoutGeneric *clone(void) const;

    /**
     * Adjust offsets of pieces to start from 'base_offset'.
     * \param base_offset Offset to start from.
     */
    virtual void relocate(size_t base_offset);

    virtual void print(std::ostream& os) const;

    REALM_INTERNAL_API_EXTERNAL_LINKAGE
    virtual void compile_lookup_program(PieceLookup::CompiledProgram& p) const;

    /**
     * Compute the offset of the specified field for an element - this
     * is generally much less efficient than using a layout-specific accessor.
     * \param p Point to calculate offset for.
     * \param fid FieldID of field to calculate offset for.
     * \return size_t Offset of field.
     */
    size_t calculate_offset(Point<N,T> p, FieldID fid) const;

    IndexSpace<N,T> space;
    std::vector<InstancePieceList<N,T> > piece_lists;

    static Serialization::PolymorphicSerdezSubclass<InstanceLayoutGeneric, InstanceLayout<N,T> > serdez_subclass;

    template <typename S>
    bool serialize(S& serializer) const;
  };

  // instance layouts are compiled into "piece lookup programs" for fast
  //  access on both CPU and accelerators

  namespace PieceLookup {

    namespace Opcodes {
      static const Opcode OP_AFFINE_PIECE = 2;  // this is a AffinePiece<N,T>
    }

    static const unsigned ALLOW_AFFINE_PIECE = 1U << Opcodes::OP_AFFINE_PIECE;

    template <int N, typename T>
    struct REALM_INTERNAL_API_EXTERNAL_LINKAGE AffinePiece : public Instruction {
      // data is: { delta[23:0], opcode[7:0] }
      // top 24 bits of data is jump delta
      AffinePiece(unsigned next_delta);

      REALM_CUDA_HD
      unsigned delta() const;

      Rect<N,T> bounds;
      uintptr_t base;
      Point<N, size_t> strides;

      REALM_CUDA_HD
      const Instruction *next() const;
    };

    template <int N, typename T>
    struct REALM_INTERNAL_API_EXTERNAL_LINKAGE SplitPlane : public Instruction {
      // data is: { delta[15:0], dim[7:0], opcode[7:0] }
      SplitPlane(int _split_dim, T _split_plane, unsigned _next_delta);

      void set_delta(unsigned _next_delta);

      REALM_CUDA_HD
      unsigned delta() const;
      REALM_CUDA_HD
      int split_dim() const;

      // if point's coord is less than split_plane, go to next, else jump
      T split_plane;

      REALM_CUDA_HD
      const Instruction *next(const Point<N,T>& p) const;

      REALM_CUDA_HD
      bool splits_rect(const Rect<N,T>& r) const;
    };

  }; // namespace PieceLookup


  template <typename FT>
  class REALM_INTERNAL_API_EXTERNAL_LINKAGE AccessorRefHelper {
  public:
    AccessorRefHelper(RegionInstance _inst, size_t _offset);

    // "read"
    operator FT(void) const;

    // "write"
    AccessorRefHelper(const AccessorRefHelper &) = default;
    AccessorRefHelper(AccessorRefHelper &&) noexcept = default;
    AccessorRefHelper<FT>& operator=(const FT& newval);
    AccessorRefHelper<FT>& operator=(const AccessorRefHelper<FT>& rhs);

  protected:
    template <typename T>
    friend std::ostream& operator<<(std::ostream& os, const AccessorRefHelper<T>& arh);

    RegionInstance inst;
    size_t offset;
  };

  /**
   * \class GenericAccessor
   * A generic accessor that works for any instance layout.
   * GenericAccessors.  While useful, these accessors are generally more
   * expensive and should be used with caution. A read/write operation via
   * this accessor could potentially result in a network transaction,
   * assuming the data is stored on a remote node
   */
  template <typename FT, int N, typename T = int>
  class REALM_PUBLIC_API GenericAccessor {
  public:
    GenericAccessor(void);

    /**
     * Construct a GenericAccessor for the specified instance and field.
     * Implicitly tries to cover the entire instance's domain.
     * \param inst RegionInstance to construct accessor for.
     * \param field_id FieldID of field to construct accessor for.
     * \param subfield_offset Offset of subfield to construct accessor for.
     */
    GenericAccessor(RegionInstance inst,
		   FieldID field_id, size_t subfield_offset = 0);

    /**
     * Construct a GenericAccessor for the specified instance and field.
     * Limits the domain to the specified subrectangle.
     * \param inst RegionInstance to construct accessor for.
     * \param field_id FieldID of field to construct accessor for.
     * \param subrect Subrectangle to limit domain to.
     * \param subfield_offset Offset of subfield to construct accessor for.
     */
    GenericAccessor(RegionInstance inst,
		   FieldID field_id, const Rect<N,T>& subrect,
		   size_t subfield_offset = 0);

    ~GenericAccessor(void);

    static bool is_compatible(RegionInstance inst, size_t field_offset);
    static bool is_compatible(RegionInstance inst, size_t field_offset, const Rect<N,T>& subrect);

    template <typename INST>
    static bool is_compatible(const INST &instance, unsigned field_id);
    template <typename INST>
    static bool is_compatible(const INST &instance, unsigned field_id, const Rect<N,T>& subrect);

    // GenericAccessor does not support returning a pointer to an element
    //FT *ptr(const Point<N,T>& p) const;

    FT read(const Point<N,T>& p);
    void write(const Point<N,T>& p, FT newval);

    /**
     * Return a reference that can be used to read or write a single
     * element.
     * \param p Point to read/write.
     * \return Reference to element at p.
     */
    AccessorRefHelper<FT> operator[](const Point<N,T>& p);

    // instead of storing the top-level layout - we narrow down to just the
    //  piece list and relative offset of the field we're interested in
    RegionInstance inst;
    const InstancePieceList<N,T> *piece_list;
    size_t rel_offset;
    // cache the most recently-used piece
    const InstanceLayoutPiece<N,T> *prev_piece;

  //protected:
    // not a const method because of the piece caching
    size_t get_offset(const Point<N,T>& p);
  };
  
  template <typename FT, int N, typename T>
  REALM_PUBLIC_API
  std::ostream& operator<<(std::ostream& os, const GenericAccessor<FT,N,T>& a);


  /**
   * \class AffineAccessor
   * An accessor that works for any instance layout that can be
   * linearized using an affine transformation.  AffineAccessors are
   * generally more efficient than GenericAccessors, but are only
   * applicable to a subset of instance layouts.
   * The random-access look-ups are O(1) in the size of the instance.
   */
  template <typename FT, int N, typename T = int>
  class REALM_PUBLIC_API AffineAccessor {
  public:
    // NOTE: even when compiling with nvcc, non-default constructors are only
    //  available in host code

    // TODO: Sean check if this is safe for a default constructor
    REALM_CUDA_HD
    AffineAccessor(void);

    /**
     * Construct an AffineAccessor for the specified instance and field.
     * Implicitly tries to cover the entire instance's domain.
     * NOTE: this constructor will die if the instance is not compatible
     * with AffineAccessor. Call is_compatible(...) first if you're not
     * sure.
     * \param inst RegionInstance to construct accessor for.
     * \param field_id FieldID of field to construct accessor for.
     * \param subfield_offset Offset of subfield to construct accessor for.
     */
    AffineAccessor(RegionInstance inst,
		   FieldID field_id, size_t subfield_offset = 0);

    /** Construct an AffineAccessor for the specified instance and field.
     * Limits the domain to the specified subrectangle.
     * NOTE: this constructor will die if the instance is not compatible
     * with AffineAccessor. Call is_compatible(...) first if you're not
     * sure.
     * \param inst RegionInstance to construct accessor for.
     * \param field_id FieldID of field to construct accessor for.
     * \param subrect Subrectangle to limit domain to.
     * \param subfield_offset Offset of subfield to construct accessor for.
     */
    AffineAccessor(RegionInstance inst,
		   FieldID field_id, const Rect<N,T>& subrect,
		   size_t subfield_offset = 0);

    ///@{
    /**
     * Construct an AffineAccessor for the specified instance, field, 
     * and coordinate transform.
     * \param inst RegionInstance to construct accessor for.
     * \param transform Affine transform to apply to coordinates.
     * \param offset Offset to apply to coordinates.
     * \param field_id FieldID of field to construct accessor for.
     * \param subfield_offset Offset of subfield to construct accessor for.
     */
    template <int N2, typename T2>
    AffineAccessor(RegionInstance inst,
		   const Matrix<N2, N, T2>& transform,
		   const Point<N2, T2>& offset,
		   FieldID field_id, size_t subfield_offset = 0);

    // note that the subrect here is in in the accessor's indexspace
    //  (from which the corresponding subrectangle in the instance can be
    //  easily determined)
    template <int N2, typename T2>
    AffineAccessor(RegionInstance inst,
		   const Matrix<N2, N, T2>& transform,
		   const Point<N2, T2>& offset,
		   FieldID field_id, const Rect<N,T>& subrect,
		   size_t subfield_offset = 0);
    ///@}

    REALM_CUDA_HD
    ~AffineAccessor(void);

    AffineAccessor(const AffineAccessor &) = default;
    AffineAccessor &operator=(const AffineAccessor &) = default;
    AffineAccessor(AffineAccessor &&) noexcept = default;
    AffineAccessor &operator=(AffineAccessor &&) noexcept = default;

    static bool is_compatible(RegionInstance inst, FieldID field_id);
    static bool is_compatible(RegionInstance inst, FieldID field_id, const Rect<N,T>& subrect);
    template <int N2, typename T2>
    static bool is_compatible(RegionInstance inst,
			      const Matrix<N2, N, T2>& transform,
			      const Point<N2, T2>& offset,
			      FieldID field_id);
    template <int N2, typename T2>
    static bool is_compatible(RegionInstance inst,
			      const Matrix<N2, N, T2>& transform,
			      const Point<N2, T2>& offset,
			      FieldID field_id, const Rect<N,T>& subrect);

    // used by constructors or can be called directly
    REALM_CUDA_HD
    void reset();
    void reset(RegionInstance inst,
	       FieldID field_id, size_t subfield_offset = 0);
    void reset(RegionInstance inst,
	       FieldID field_id, const Rect<N,T>& subrect,
	       size_t subfield_offset = 0);
    template <int N2, typename T2>
    void reset(RegionInstance inst,
	       const Matrix<N2, N, T2>& transform,
	       const Point<N2, T2>& offset,
	       FieldID field_id, size_t subfield_offset = 0);
    template <int N2, typename T2>
    void reset(RegionInstance inst,
	       const Matrix<N2, N, T2>& transform,
	       const Point<N2, T2>& offset,
	       FieldID field_id, const Rect<N,T>& subrect,
	       size_t subfield_offset = 0);
  
    REALM_CUDA_HD
    FT *ptr(const Point<N,T>& p) const;
    REALM_CUDA_HD
    FT read(const Point<N,T>& p) const;
    REALM_CUDA_HD
    void write(const Point<N,T>& p, FT newval) const;

    REALM_CUDA_HD
    FT& operator[](const Point<N,T>& p) const;

    REALM_CUDA_HD
    bool is_dense_arbitrary(const Rect<N,T> &bounds) const; // any dimension ordering
    REALM_CUDA_HD
    bool is_dense_col_major(const Rect<N,T> &bounds) const; // Fortran dimension ordering
    REALM_CUDA_HD
    bool is_dense_row_major(const Rect<N,T> &bounds) const; // C dimension ordering

#ifdef REALM_PROVIDE_ACCESSOR_TO_KOKKOS_VIEW_CONVERSION
  // conversion to Kokkos unmanaged views

  // Kokkos::View uses relative ("local") indexing - the first element is
  //  always index 0, even when accessing a subregion that does not include
  //  global element 0
  template <typename ... Args>
  operator Kokkos::View<Args...>() const;

  // Kokkos::Experimental::OffsetView uses absolute ("global") indexing -
  //  the indices used on the OffsetView::operator() match what is used for
  //  the AffineAccessor's operator[]
  template <typename ... Args>
  operator Kokkos::Experimental::OffsetView<Args...>() const;
#endif

  //protected:
  //friend
  // std::ostream& operator<<(std::ostream& os, const AffineAccessor<FT,N,T>& a);
//#define REALM_ACCESSOR_DEBUG
#if defined(REALM_ACCESSOR_DEBUG) || defined(REALM_USE_KOKKOS)
  Rect<N,T> bounds;
#endif
#ifdef REALM_USE_KOKKOS
  bool bounds_specified;
#endif
#ifdef REALM_ACCESSOR_DEBUG
  RegionInstance dbg_inst;
#if defined (__CUDACC__) || defined (__HIPCC__)
#error "REALM_ACCESSOR_DEBUG macro for AffineAccessor not supported for GPU code"
#endif
#endif
    uintptr_t base;
    Point<N, size_t> strides;
  protected:
    REALM_CUDA_HD
    FT* get_ptr(const Point<N,T>& p) const;
  };

  template <typename FT, int N, typename T>
  REALM_PUBLIC_API
  std::ostream& operator<<(std::ostream& os, const AffineAccessor<FT,N,T>& a);

  // a multi-affine accessor handles instances with multiple pieces, but only
  //  if all of them are affine
  template <typename FT, int N, typename T = int>
  class MultiAffineAccessor;

  template <typename FT, int N, typename T>
  REALM_PUBLIC_API
  std::ostream& operator<<(std::ostream& os, const MultiAffineAccessor<FT,N,T>& a);

  /**
   * \class MultiAffineAccessor
   * A multi-affine accessor handles instances with multiple pieces, but only
   * if all of them are affine. Multi-affine accessors may be accessed and
   * copied in CUDA device code, but must be initially constructed on the host.
   * The random-access look-ups are O(log(N)) in the number of pieces.
   */
  template <typename FT, int N, typename T>
  class REALM_PUBLIC_API MultiAffineAccessor {
  public:

    REALM_CUDA_HD
    MultiAffineAccessor(void);

    /**
     * Construct a multi-affine accessor for the given instance and field.
     * The accessor will implicitly try to cover the entire instance's domain.
     * NOTE: This constructor will crash if the conversion is not
     * allowed - call is_compatible(...) first if you're not sure.
     * \param inst The instance to access.
     * \param field_id The field to access.
     * \param subfield_offset The offset of the subfield to access.
     */
    MultiAffineAccessor(RegionInstance inst,
			FieldID field_id, size_t subfield_offset = 0);

    /**
     * Construct a multi-affine accessor for the given instance and field.
     * Limits the domain to the given subrectangle. (NOTE: subrect need not be
     * entirely covered by the instance - a legal access must be both within
     * the subrect AND within the coverage of the instance.)
     * \param inst The instance to access.
     * \param field_id The field to access.
     * \param subrect The subrectangle to limit the domain to.
     * \param subfield_offset The offset of the subfield to access.
     */
    MultiAffineAccessor(RegionInstance inst,
			FieldID field_id, const Rect<N,T>& subrect,
			size_t subfield_offset = 0);

    REALM_CUDA_HD
    ~MultiAffineAccessor(void);

    MultiAffineAccessor(const MultiAffineAccessor &) = default;
    MultiAffineAccessor &operator=(const MultiAffineAccessor &) = default;
    MultiAffineAccessor(MultiAffineAccessor &&) noexcept = default;
    MultiAffineAccessor &operator=(MultiAffineAccessor &&) noexcept = default;

    static bool is_compatible(RegionInstance inst, FieldID field_id);
    static bool is_compatible(RegionInstance inst, FieldID field_id,
			      const Rect<N,T>& subrect);

    // used by constructors or can be called directly
    REALM_CUDA_HD
    void reset();
    void reset(RegionInstance inst,
	       FieldID field_id, size_t subfield_offset = 0);
    void reset(RegionInstance inst,
	       FieldID field_id, const Rect<N,T>& subrect,
	       size_t subfield_offset = 0);

    ///@{
    /**
     * Return a pointer to the given point in the instance. Nonconst version
     * is allowed to remember the most-recently-accessed piece.
     * \param p The point to access.
     * \return A pointer to the given point.
     */
    REALM_CUDA_HD
    FT *ptr(const Point<N,T>& p) const;
    REALM_CUDA_HD
    FT *ptr(const Rect<N,T>& r, size_t strides[N]) const;
    REALM_CUDA_HD
    FT read(const Point<N,T>& p) const;
    REALM_CUDA_HD
    void write(const Point<N,T>& p, FT newval) const;
    ///@}


    REALM_CUDA_HD
    FT& operator[](const Point<N,T>& p) const;

    REALM_CUDA_HD
    FT *ptr(const Point<N,T>& p);
    REALM_CUDA_HD
    FT *ptr(const Rect<N,T>& r, size_t strides[N]);
    REALM_CUDA_HD
    FT read(const Point<N,T>& p);
    REALM_CUDA_HD
    void write(const Point<N,T>& p, FT newval);

    REALM_CUDA_HD
    FT& operator[](const Point<N,T>& p);

  protected:
    friend std::ostream& operator<< <FT,N,T>(std::ostream& os, const MultiAffineAccessor<FT,N,T>& a);

    // cached info from the most recent piece, or authoritative info for
    //  a single piece
    bool piece_valid;
    Rect<N,T> piece_bounds;
    uintptr_t piece_base;
    Point<N, size_t> piece_strides;
    // if we need to do a new lookup, this is where we start
    const PieceLookup::Instruction *start_inst;
    size_t field_offset;
  };


}; // namespace Realm

#include "realm/inst_layout.inl"

#endif // ifndef REALM_INST_LAYOUT_H


