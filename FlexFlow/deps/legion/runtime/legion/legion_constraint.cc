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

#include "legion/legion_utilities.h"
#include "legion/legion_constraint.h"

#define SWAP_HELPER(type, member)   \
{                                   \
  type temp = member;               \
  member = rhs.member;              \
  rhs.member = temp;                \
}

namespace Legion {
  
    // some helper methods

    //--------------------------------------------------------------------------
    static inline bool bound_entails(EqualityKind eq1, long v1,
                                     EqualityKind eq2, long v2)
    //--------------------------------------------------------------------------
    {
      switch (eq1)
      {
        case LEGION_LT_EK: // < v1
          {
            // Can entail for <, <=, !=  
            if ((eq2 == LEGION_LT_EK) && (v1 <= v2)) // < v2
              return true;
            if ((eq2 == LEGION_LE_EK) && (v1 < v2)) // <= v2
              return true;
            if ((eq2 == LEGION_NE_EK) && (v1 <= v2)) // != v2
              return true;
            return false;
          }
        case LEGION_LE_EK: // <= v1
          {
            // Can entail for <, <=, !=
            if ((eq2 == LEGION_LT_EK) && (v1 < v2)) // < v2
              return true;
            if ((eq2 == LEGION_LE_EK) && (v1 <= v2)) // <= v2
              return true;
            if ((eq2 == LEGION_NE_EK) && (v1 < v2)) // != v2
              return true;
            return false;
          }
        case LEGION_GT_EK: // > v1
          {
            // Can entail for >, >=, !=
            if ((eq2 == LEGION_GT_EK) && (v1 >= v2)) // > v2
              return true;
            if ((eq2 == LEGION_GE_EK) && (v1 > v2)) // >= v2
              return true;
            if ((eq2 == LEGION_NE_EK) && (v1 >= v2)) // != v2
              return true;
            return false;
          }
        case LEGION_GE_EK: // >= v1
          {
            // Can entail for >, >=, !=
            if ((eq2 == LEGION_GT_EK) && (v1 > v2)) // > v2
              return true;
            if ((eq2 == LEGION_GE_EK) && (v1 >= v2)) // >= v2
              return true;
            if ((eq2 == LEGION_NE_EK) && (v1 > v2)) // != v2
              return true;
            return false;
          }
        case LEGION_EQ_EK: // == v1
          {
            // Can entail for <, <=, >, >=, ==, !=
            if ((eq2 == LEGION_LT_EK) && (v1 < v2)) // < v2
              return true;
            if ((eq2 == LEGION_LE_EK) && (v1 <= v2)) // <= v2
              return true;
            if ((eq2 == LEGION_GT_EK) && (v1 > v2)) // > v2
              return true;
            if ((eq2 == LEGION_GE_EK) && (v1 >= v2)) // >= v2
              return true;
            if ((eq2 == LEGION_EQ_EK) && (v1 == v2)) // == v2
              return true;
            if ((eq2 == LEGION_NE_EK) && (v1 != v2)) // != v2
              return true;
            return false;
          }
        case LEGION_NE_EK: // != v1
          {
            // Can only entail for != of the same value
            if ((eq2 == LEGION_NE_EK) && (v1 == v2)) // != v2
              return true;
            return false;
          }
        default:
          assert(false); // unknown
      }
      return false;
    }

    //--------------------------------------------------------------------------
    static inline bool bound_conflicts(EqualityKind eq1, long v1,
                                       EqualityKind eq2, long v2)
    //--------------------------------------------------------------------------
    {
      switch (eq1)
      {
        case LEGION_LT_EK: // < v1
          {
            // conflicts with >, >=, ==
            if ((eq2 == LEGION_GT_EK) && ((v1-1) <= v2)) // > v2
              return true;
            if ((eq2 == LEGION_GE_EK) && (v1 <= v2)) // >= v2
              return true;
            if ((eq2 == LEGION_EQ_EK) && (v1 <= v2)) // == v2
              return true;
            return false;
          }
        case LEGION_LE_EK: // <= v1
          {
            // conflicts with >, >=, == 
            if ((eq2 == LEGION_GT_EK) && (v1 <= v2)) // > v2
              return true;
            if ((eq2 == LEGION_GE_EK) && (v1 < v2)) // >= v2
              return true;
            if ((eq2 == LEGION_EQ_EK) && (v1 < v2)) // == v2
              return true;
            return false;
          }
        case LEGION_GT_EK: // > v1
          {
            // coflicts with <, <=, ==
            if ((eq2 == LEGION_LT_EK) && ((v1+1) >= v2)) // < v2
              return true;
            if ((eq2 == LEGION_LE_EK) && (v1 >= v2)) // <= v2
              return true;
            if ((eq2 == LEGION_EQ_EK) && (v1 >= v2)) // == v2
              return true;
            return false;
          }
        case LEGION_GE_EK: // >= v1
          {
            // conflicts with <, <=, ==
            if ((eq2 == LEGION_LT_EK) && (v1 >= v2)) // < v2
              return true;
            if ((eq2 == LEGION_LE_EK) && (v1 > v2)) // <= v2
              return true;
            if ((eq2 == LEGION_EQ_EK) && (v1 > v2)) // == v2
              return true;
            return false;
          }
        case LEGION_EQ_EK: // == v1
          {
            // conflicts with <, <=, >, >=, ==, !=
            if ((eq2 == LEGION_LT_EK) && (v1 >= v2)) // < v2
              return true;
            if ((eq2 == LEGION_LE_EK) && (v1 > v2)) // <= v2
              return true;
            if ((eq2 == LEGION_GT_EK) && (v1 <= v2)) // > v2
              return true;
            if ((eq2 == LEGION_GT_EK) && (v1 < v2)) // >= v2
              return true;
            if ((eq2 == LEGION_EQ_EK) && (v1 != v2)) // == v2
              return true;
            if ((eq2 == LEGION_NE_EK) && (v1 == v2)) // != v2
              return true;
            return false;
          }
        case LEGION_NE_EK: // != v1
          {
            // conflicts with ==
            if ((eq2 == LEGION_EQ_EK) && (v1 == v2)) // == v2
              return true;
            return false;
          }
        default:
          assert(false); // unknown
      }
      return false;
    }

    /////////////////////////////////////////////////////////////
    // ISAConstraint 
    /////////////////////////////////////////////////////////////

    //--------------------------------------------------------------------------
    ISAConstraint::ISAConstraint(uint64_t prop /*= 0*/)
      : isa_prop(prop)
    //--------------------------------------------------------------------------
    {
    }

    //--------------------------------------------------------------------------
    void ISAConstraint::swap(ISAConstraint &rhs)
    //--------------------------------------------------------------------------
    {
      SWAP_HELPER(uint64_t, isa_prop)
    }
    
    //--------------------------------------------------------------------------
    void ISAConstraint::serialize(Serializer &rez) const
    //--------------------------------------------------------------------------
    {
      rez.serialize(isa_prop);
    }

    //--------------------------------------------------------------------------
    void ISAConstraint::deserialize(Deserializer &derez)
    //--------------------------------------------------------------------------
    {
      derez.deserialize(isa_prop);
    }

    /////////////////////////////////////////////////////////////
    // Processor Constraint 
    /////////////////////////////////////////////////////////////

    //--------------------------------------------------------------------------
    ProcessorConstraint::ProcessorConstraint(Processor::Kind kind)
    //--------------------------------------------------------------------------
    {
      if ((kind != Processor::NO_KIND) && (kind != Processor::PROC_GROUP))
        valid_kinds.push_back(kind);
    }

    //--------------------------------------------------------------------------
    void ProcessorConstraint::add_kind(Processor::Kind kind)
    //--------------------------------------------------------------------------
    {
      if ((kind == Processor::NO_KIND) || (kind == Processor::PROC_GROUP))
        return;
      for (unsigned idx = 0; idx < valid_kinds.size(); idx++)
        if (valid_kinds[idx] == kind)
          return;
      valid_kinds.push_back(kind);
    }

    //--------------------------------------------------------------------------
    bool ProcessorConstraint::can_use(Processor::Kind kind) const
    //--------------------------------------------------------------------------
    {
      for (unsigned idx = 0; idx < valid_kinds.size(); idx++)
        if (valid_kinds[idx] == kind)
          return true;
      return false;
    }

    //--------------------------------------------------------------------------
    void ProcessorConstraint::swap(ProcessorConstraint &rhs)
    //--------------------------------------------------------------------------
    {
      valid_kinds.swap(rhs.valid_kinds);
    }

    //--------------------------------------------------------------------------
    void ProcessorConstraint::serialize(Serializer &rez) const
    //--------------------------------------------------------------------------
    {
      rez.serialize<size_t>(valid_kinds.size());
      for (std::vector<Processor::Kind>::const_iterator it = 
            valid_kinds.begin(); it != valid_kinds.end(); it++)
        rez.serialize(*it);
    }
    
    //--------------------------------------------------------------------------
    void ProcessorConstraint::deserialize(Deserializer &derez)
    //--------------------------------------------------------------------------
    {
      size_t num_kinds;
      derez.deserialize(num_kinds);
      if (num_kinds > 0)
      {
        valid_kinds.resize(num_kinds);
        for (unsigned idx = 0; idx < num_kinds; idx++)
          derez.deserialize(valid_kinds[idx]);
      }
    }

    /////////////////////////////////////////////////////////////
    // ResourceConstraint
    /////////////////////////////////////////////////////////////


    //--------------------------------------------------------------------------
    ResourceConstraint::ResourceConstraint(void)
    //--------------------------------------------------------------------------
    {
    }

    //--------------------------------------------------------------------------
    ResourceConstraint::ResourceConstraint(ResourceKind resource,
                                           EqualityKind equality, size_t val)
      : resource_kind(resource), equality_kind(equality), value(val)
    //--------------------------------------------------------------------------
    {
    }

    //--------------------------------------------------------------------------
    bool ResourceConstraint::operator==(const ResourceConstraint &other) const
    //--------------------------------------------------------------------------
    {
      return resource_kind == other.resource_kind
          && equality_kind == other.equality_kind
          && value == other.value;
    }

    //--------------------------------------------------------------------------
    void ResourceConstraint::swap(ResourceConstraint &rhs)
    //--------------------------------------------------------------------------
    {
      SWAP_HELPER(ResourceKind, resource_kind)
      SWAP_HELPER(EqualityKind, equality_kind)
      SWAP_HELPER(size_t, value);
    }

    //--------------------------------------------------------------------------
    void ResourceConstraint::serialize(Serializer &rez) const
    //--------------------------------------------------------------------------
    {
      rez.serialize(resource_kind);
      rez.serialize(equality_kind);
      rez.serialize(value);
    }

    //--------------------------------------------------------------------------
    void ResourceConstraint::deserialize(Deserializer &derez)
    //--------------------------------------------------------------------------
    {
      derez.deserialize(resource_kind);
      derez.deserialize(equality_kind);
      derez.deserialize(value);
    }

    /////////////////////////////////////////////////////////////
    // LaunchConstraint 
    /////////////////////////////////////////////////////////////

    //--------------------------------------------------------------------------
    LaunchConstraint::LaunchConstraint(void)
      : dims(0)
    //--------------------------------------------------------------------------
    {
    }

    //--------------------------------------------------------------------------
    LaunchConstraint::LaunchConstraint(LaunchKind kind, size_t value)
      : launch_kind(kind), dims(1)
    //--------------------------------------------------------------------------
    {
      values[0] = value;
    }

    //--------------------------------------------------------------------------
    LaunchConstraint::LaunchConstraint(LaunchKind kind, const size_t *vs, int d)
      : launch_kind(kind), dims(d)
    //--------------------------------------------------------------------------
    {
#ifdef DEBUG_LEGION
      assert(dims < 3);
#endif
      for (int i = 0; i < dims; i++)
        values[i] = vs[i];
    }

    //--------------------------------------------------------------------------
    bool LaunchConstraint::operator==(const LaunchConstraint &other) const
    //--------------------------------------------------------------------------
    {
      if (!(launch_kind == other.launch_kind && dims == other.dims))
        return false;
      for (int i = 0; i < dims; i++)
        if (values[i] != other.values[i])
          return false;
      return true;
    }

    //--------------------------------------------------------------------------
    void LaunchConstraint::swap(LaunchConstraint &rhs)
    //--------------------------------------------------------------------------
    {
      SWAP_HELPER(LaunchKind, launch_kind)
      size_t v_temp[3];
      for (unsigned idx = 0; idx < 3; idx++)
        v_temp[idx] = values[idx];
      for (unsigned idx = 0; idx < 3; idx++)
        values[idx] = rhs.values[idx];
      for (unsigned idx = 0; idx < 3; idx++)
        rhs.values[idx] = v_temp[idx];
      SWAP_HELPER(int, dims)
    }

    //--------------------------------------------------------------------------
    void LaunchConstraint::serialize(Serializer &rez) const
    //--------------------------------------------------------------------------
    {
      rez.serialize(launch_kind);
      rez.serialize(dims);
      for (int i = 0; i < dims; i++)
        rez.serialize(values[i]);
    }

    //--------------------------------------------------------------------------
    void LaunchConstraint::deserialize(Deserializer &derez)
    //--------------------------------------------------------------------------
    {
      derez.deserialize(launch_kind);
      derez.deserialize(dims);
      for (int i = 0; i < dims; i++)
        derez.deserialize(values[i]);
    }

    /////////////////////////////////////////////////////////////
    // ColocationConstraint 
    /////////////////////////////////////////////////////////////

    //--------------------------------------------------------------------------
    ColocationConstraint::ColocationConstraint(void)
    //--------------------------------------------------------------------------
    {
    }

    //--------------------------------------------------------------------------
    ColocationConstraint::ColocationConstraint(unsigned idx1, unsigned idx2)
    //--------------------------------------------------------------------------
    {
      indexes.insert(idx1);
      indexes.insert(idx2);
    }

    //--------------------------------------------------------------------------
    ColocationConstraint::ColocationConstraint(unsigned idx1, unsigned idx2,
                                               FieldID fid)
    //--------------------------------------------------------------------------
    {
      indexes.insert(idx1);
      indexes.insert(idx2);
      fields.insert(fid);
    }

    //--------------------------------------------------------------------------
    ColocationConstraint::ColocationConstraint(unsigned index1, unsigned index2,
                                               const std::set<FieldID> &fids)
    //--------------------------------------------------------------------------
    {
      indexes.insert(index1);
      indexes.insert(index2);
      fields = fids;
    }

    //--------------------------------------------------------------------------
    ColocationConstraint::ColocationConstraint(const std::vector<unsigned> &idx,
                                               const std::set<FieldID> &fids)
      : fields(fids), indexes(idx.begin(), idx.end())
    //--------------------------------------------------------------------------
    {
    }

    //--------------------------------------------------------------------------
    void ColocationConstraint::swap(ColocationConstraint &rhs)
    //--------------------------------------------------------------------------
    {
      fields.swap(rhs.fields);
      indexes.swap(rhs.indexes);
    }

    //--------------------------------------------------------------------------
    void ColocationConstraint::serialize(Serializer &rez) const
    //--------------------------------------------------------------------------
    {
      rez.serialize<size_t>(indexes.size());
      for (std::set<unsigned>::const_iterator it = indexes.begin();
            it != indexes.end(); it++)
        rez.serialize(*it);
      rez.serialize<size_t>(fields.size());
      for (std::set<FieldID>::const_iterator it = fields.begin();
            it != fields.end(); it++)
        rez.serialize(*it);
    }

    //--------------------------------------------------------------------------
    void ColocationConstraint::deserialize(Deserializer &derez)
    //--------------------------------------------------------------------------
    {
      size_t num_indexes;
      derez.deserialize(num_indexes);
      for (unsigned idx = 0; idx < num_indexes; idx++)
      {
        unsigned index;
        derez.deserialize(index);
        indexes.insert(index);
      }
      size_t num_fields;
      derez.deserialize(num_fields);
      for (unsigned idx = 0; idx < num_fields; idx++)
      {
        FieldID fid;
        derez.deserialize(fid);
        fields.insert(fid);
      }
    }

    /////////////////////////////////////////////////////////////
    // ColocationConstraint 
    /////////////////////////////////////////////////////////////

    //--------------------------------------------------------------------------
    ExecutionConstraintSet& ExecutionConstraintSet::add_constraint(
                                                const ISAConstraint &constraint)
    //--------------------------------------------------------------------------
    {
      isa_constraint = constraint;
      return *this;
    }

    //--------------------------------------------------------------------------
    ExecutionConstraintSet& ExecutionConstraintSet::add_constraint(
                                          const ProcessorConstraint &constraint)
    //--------------------------------------------------------------------------
    {
      processor_constraint = constraint;
      return *this;
    }

    //--------------------------------------------------------------------------
    ExecutionConstraintSet& ExecutionConstraintSet::add_constraint(
                                           const ResourceConstraint &constraint)
    //--------------------------------------------------------------------------
    {
      resource_constraints.push_back(constraint);
      return *this;
    }

    //--------------------------------------------------------------------------
    ExecutionConstraintSet& ExecutionConstraintSet::add_constraint(
                                             const LaunchConstraint &constraint)
    //--------------------------------------------------------------------------
    {
      launch_constraints.push_back(constraint);
      return *this;
    }

    //--------------------------------------------------------------------------
    ExecutionConstraintSet& ExecutionConstraintSet::add_constraint(
                                         const ColocationConstraint &constraint)
    //--------------------------------------------------------------------------
    {
      colocation_constraints.push_back(constraint);
      return *this;
    }

    //--------------------------------------------------------------------------
    bool ExecutionConstraintSet::operator==(
                                      const ExecutionConstraintSet &other) const
    //--------------------------------------------------------------------------
    {
      return isa_constraint == other.isa_constraint
          && processor_constraint == other.processor_constraint
          && resource_constraints == other.resource_constraints
          && launch_constraints == other.launch_constraints
          && colocation_constraints == other.colocation_constraints;
    }

    //--------------------------------------------------------------------------
    void ExecutionConstraintSet::swap(ExecutionConstraintSet &rhs)
    //--------------------------------------------------------------------------
    {
      isa_constraint.swap(rhs.isa_constraint);
      processor_constraint.swap(rhs.processor_constraint);
      resource_constraints.swap(rhs.resource_constraints);
      launch_constraints.swap(rhs.launch_constraints);
      colocation_constraints.swap(rhs.colocation_constraints);
    }

    //--------------------------------------------------------------------------
    void ExecutionConstraintSet::serialize(Serializer &rez) const
    //--------------------------------------------------------------------------
    {
      isa_constraint.serialize(rez);
      processor_constraint.serialize(rez);
#define PACK_CONSTRAINTS(Type, constraints)                             \
      rez.serialize<size_t>(constraints.size());                        \
      for (std::vector<Type>::const_iterator it = constraints.begin();  \
            it != constraints.end(); it++)                              \
      {                                                                 \
        it->serialize(rez);                                             \
      }
      PACK_CONSTRAINTS(ResourceConstraint, resource_constraints)
      PACK_CONSTRAINTS(LaunchConstraint, launch_constraints)
      PACK_CONSTRAINTS(ColocationConstraint, colocation_constraints)
#undef PACK_CONSTRAINTS
    }

    //--------------------------------------------------------------------------
    void ExecutionConstraintSet::deserialize(Deserializer &derez)
    //--------------------------------------------------------------------------
    {
      isa_constraint.deserialize(derez);
      processor_constraint.deserialize(derez);
#define UNPACK_CONSTRAINTS(Type, constraints)                       \
      {                                                             \
        size_t constraint_size;                                     \
        derez.deserialize(constraint_size);                         \
        constraints.resize(constraint_size);                        \
        for (std::vector<Type>::iterator it = constraints.begin();  \
              it != constraints.end(); it++)                        \
        {                                                           \
          it->deserialize(derez);                                   \
        }                                                           \
      }
      UNPACK_CONSTRAINTS(ResourceConstraint, resource_constraints)
      UNPACK_CONSTRAINTS(LaunchConstraint, launch_constraints)
      UNPACK_CONSTRAINTS(ColocationConstraint, colocation_constraints)
#undef UNPACK_CONSTRAINTS
    }

    /////////////////////////////////////////////////////////////
    // Specialized Constraint
    /////////////////////////////////////////////////////////////

    //--------------------------------------------------------------------------
    SpecializedConstraint::SpecializedConstraint(SpecializedKind k,
      ReductionOpID r, bool no, bool ext, size_t pieces, int overhead)
      : kind(k), redop(r),  max_pieces(pieces), max_overhead(overhead),
        no_access(no), exact(ext)
    //-------------------------------------------------------------------------
    {
      if (redop != 0)
      {
        if ((kind != LEGION_AFFINE_REDUCTION_SPECIALIZE) &&
            (kind != LEGION_COMPACT_REDUCTION_SPECIALIZE))
        {
          fprintf(stderr,"Illegal specialize constraint with reduction op %d."
                         "Only reduction specialized constraints are "
                         "permitted to have non-zero reduction operators.",
                         redop);
          assert(false);
        }
      }
    }

    //--------------------------------------------------------------------------
    bool SpecializedConstraint::operator==(
                                       const SpecializedConstraint &other) const
    //--------------------------------------------------------------------------
    {
      return ((kind == other.kind) && (redop == other.redop) &&
        (max_pieces == other.max_pieces) && (max_overhead == other.max_overhead)
          && (no_access == other.no_access) && (exact == other.exact));
    }

    //--------------------------------------------------------------------------
    bool SpecializedConstraint::entails(const SpecializedConstraint &other)const
    //--------------------------------------------------------------------------
    {
      // entails if the other doesn't have any specialization
      if (other.kind == LEGION_NO_SPECIALIZE)
        return true;
      if (kind != other.kind)
        return false;
      // Make sure we also handle the unspecialized case of redop 0
      if ((redop != other.redop) && (other.redop != 0))
        return false;
      if (max_pieces > other.max_pieces)
        return false;
      if (max_overhead > other.max_overhead)
        return false;
      if (no_access && !other.no_access)
        return false;
      // We'll test for exactness inside the runtime
      return true;
    }

    //--------------------------------------------------------------------------
    bool SpecializedConstraint::conflicts(
                                       const SpecializedConstraint &other) const
    //--------------------------------------------------------------------------
    {
      if (kind == LEGION_NO_SPECIALIZE)
        return false;
      if (other.kind == LEGION_NO_SPECIALIZE)
        return false;
      if (kind != other.kind)
        return true;
      // Only conflicts if we both have non-zero redops that don't equal
      if ((redop != other.redop) && (redop != 0) && (other.redop != 0))
        return true;
      if (max_pieces != other.max_pieces)
        return true;
      if (max_overhead != other.max_overhead)
        return true;
      // No access never causes a conflict
      // We'll test for exactness inside the runtime
      return false;
    }

    //--------------------------------------------------------------------------
    void SpecializedConstraint::swap(SpecializedConstraint &rhs)
    //--------------------------------------------------------------------------
    {
      SWAP_HELPER(SpecializedKind, kind)
      SWAP_HELPER(ReductionOpID, redop)
      SWAP_HELPER(size_t, max_pieces)
      SWAP_HELPER(int, max_overhead)
      SWAP_HELPER(bool, no_access)
      SWAP_HELPER(bool, exact)
    }

    //--------------------------------------------------------------------------
    void SpecializedConstraint::serialize(Serializer &rez) const
    //--------------------------------------------------------------------------
    {
      rez.serialize(kind);
      if ((kind == LEGION_AFFINE_REDUCTION_SPECIALIZE) || 
          (kind == LEGION_COMPACT_REDUCTION_SPECIALIZE))
        rez.serialize(redop);
      if ((kind == LEGION_COMPACT_SPECIALIZE) ||
          (kind == LEGION_COMPACT_REDUCTION_SPECIALIZE)) 
      {
        rez.serialize(max_pieces);
        rez.serialize(max_overhead);
      }
      rez.serialize<bool>(no_access);
      rez.serialize<bool>(exact);
    }

    //--------------------------------------------------------------------------
    void SpecializedConstraint::deserialize(Deserializer &derez)
    //--------------------------------------------------------------------------
    {
      derez.deserialize(kind);
      if ((kind == LEGION_AFFINE_REDUCTION_SPECIALIZE) || 
          (kind == LEGION_COMPACT_REDUCTION_SPECIALIZE))
        derez.deserialize(redop);
      if ((kind == LEGION_COMPACT_SPECIALIZE) ||
          (kind == LEGION_COMPACT_REDUCTION_SPECIALIZE))
      {
        derez.deserialize(max_pieces);
        derez.deserialize(max_overhead);
      }
      derez.deserialize<bool>(no_access);
      derez.deserialize<bool>(exact);
    }

    //--------------------------------------------------------------------------
    bool SpecializedConstraint::is_normal(void) const
    //--------------------------------------------------------------------------
    {
      return (kind == LEGION_AFFINE_SPECIALIZE);
    }

    //--------------------------------------------------------------------------
    bool SpecializedConstraint::is_affine(void) const
    //--------------------------------------------------------------------------
    {
      return ((kind == LEGION_AFFINE_SPECIALIZE) || 
              (kind == LEGION_AFFINE_REDUCTION_SPECIALIZE));
    }

    //--------------------------------------------------------------------------
    bool SpecializedConstraint::is_compact(void) const
    //--------------------------------------------------------------------------
    {
      return ((kind == LEGION_COMPACT_SPECIALIZE) || 
              (kind == LEGION_COMPACT_REDUCTION_SPECIALIZE));
    }

    //--------------------------------------------------------------------------
    bool SpecializedConstraint::is_virtual(void) const
    //--------------------------------------------------------------------------
    {
      return (kind == LEGION_VIRTUAL_SPECIALIZE);
    }

    //--------------------------------------------------------------------------
    bool SpecializedConstraint::is_reduction(void) const
    //--------------------------------------------------------------------------
    {
      return ((kind == LEGION_AFFINE_REDUCTION_SPECIALIZE) || 
              (kind == LEGION_COMPACT_REDUCTION_SPECIALIZE));
    }

    //--------------------------------------------------------------------------
    bool SpecializedConstraint::is_file(void) const
    //--------------------------------------------------------------------------
    {
      return (LEGION_VIRTUAL_SPECIALIZE < kind);
    }

    /////////////////////////////////////////////////////////////
    // Memory Constraint 
    /////////////////////////////////////////////////////////////

    //--------------------------------------------------------------------------
    MemoryConstraint::MemoryConstraint(void)
      : kind(Memory::GLOBAL_MEM), has_kind(false)
    //--------------------------------------------------------------------------
    {
    }

    //--------------------------------------------------------------------------
    MemoryConstraint::MemoryConstraint(Memory::Kind k)
      : kind(k), has_kind(true)
    //--------------------------------------------------------------------------
    {
    }

    //--------------------------------------------------------------------------
    bool MemoryConstraint::entails(const MemoryConstraint &other) const
    //--------------------------------------------------------------------------
    {
      if (!other.has_kind)
        return true;
      if (!has_kind)
        return false;
      if (kind == other.kind)
        return true;
      return false;
    }

    //--------------------------------------------------------------------------
    bool MemoryConstraint::conflicts(const MemoryConstraint &other) const
    //--------------------------------------------------------------------------
    {
      if (!has_kind || !other.has_kind)
        return false;
      if (kind != other.kind)
        return true;
      return false;
    }

    //--------------------------------------------------------------------------
    void MemoryConstraint::swap(MemoryConstraint &rhs)
    //--------------------------------------------------------------------------
    {
      SWAP_HELPER(Memory::Kind, kind)
      SWAP_HELPER(bool, has_kind)
    }

    //--------------------------------------------------------------------------
    void MemoryConstraint::serialize(Serializer &rez) const
    //--------------------------------------------------------------------------
    {
      rez.serialize(has_kind);
      if (has_kind)
        rez.serialize(kind);
    }

    //--------------------------------------------------------------------------
    void MemoryConstraint::deserialize(Deserializer &derez)
    //--------------------------------------------------------------------------
    {
      derez.deserialize(has_kind);
      if (has_kind)
        derez.deserialize(kind);
    }

    /////////////////////////////////////////////////////////////
    // Field Constraint 
    /////////////////////////////////////////////////////////////

    //--------------------------------------------------------------------------
    FieldConstraint::FieldConstraint(bool contig, bool in)
      : contiguous(contig), inorder(in)
    //--------------------------------------------------------------------------
    {
    }

    //--------------------------------------------------------------------------
    FieldConstraint::FieldConstraint(const std::vector<FieldID> &set, 
                                     bool cg, bool in)
      : field_set(set), contiguous(cg), inorder(in)
    //--------------------------------------------------------------------------
    {
    }

    //--------------------------------------------------------------------------
    FieldConstraint::FieldConstraint(const std::set<FieldID> &set,
                                     bool cg, bool in)
      : field_set(set.begin(),set.end()), contiguous(cg), inorder(in)
    //--------------------------------------------------------------------------
    {
    }

    //--------------------------------------------------------------------------
    bool FieldConstraint::operator==(const FieldConstraint &other) const
    //--------------------------------------------------------------------------
    {
      return field_set == other.field_set
          && contiguous == other.contiguous
          && inorder == other.inorder;
    }

    //--------------------------------------------------------------------------
    bool FieldConstraint::entails(const FieldConstraint &other) const
    //--------------------------------------------------------------------------
    {
      // Handle empty field sets quickly
      if (other.field_set.empty())
        return true;
      if (field_set.empty())
        return false;
      if (field_set.size() < other.field_set.size())
        return false; // can't have all the fields
      // If the other can have any fields then we can just test directly
      if (field_set.empty() || other.field_set.empty())
      {
        if (other.contiguous && !contiguous)
          return false;
        if (other.inorder && !inorder)
          return false;
        return true;
      }
      // Find the indexes of the other fields in our set
      std::vector<unsigned> field_indexes(other.field_set.size());
      unsigned local_idx = 0;
      for (std::vector<FieldID>::const_iterator it = other.field_set.begin();
            it != other.field_set.end(); it++,local_idx++)
      {
        bool found = false;
        for (unsigned idx = 0; idx < field_set.size(); idx++)
        {
          if (field_set[idx] == (*it))
          {
            field_indexes[local_idx] = idx;
            found = true;
            break;
          }
        }
        if (!found)
          return false; // can't entail if we don't have the field
      }
      if (other.contiguous)
      {
        if (other.inorder)
        {
          // Other is both inorder and contiguous
          // If we're not both contiguous and inorder we can't entail it
          if (!contiguous || !inorder)
            return false;
          // See if our fields are in order and grow by one each time 
          for (unsigned idx = 1; idx < field_indexes.size(); idx++)
          {
            if ((field_indexes[idx-1]+1) != field_indexes[idx])
              return false;
          }
          return true;
        }
        else
        {
          // Other is contiguous but not inorder
          // If we're not contiguous we can't entail it
          if (!contiguous)
            return false;
          // See if all our indexes are continuous 
          std::set<unsigned> sorted_indexes(field_indexes.begin(),
                                            field_indexes.end());
          int previous = -1;
          for (std::set<unsigned>::const_iterator it = sorted_indexes.begin();
                it != sorted_indexes.end(); it++)
          {
            if (previous != -1)
            {
              if ((previous+1) != int(*it))
                return false;
            }
            previous = (*it); 
          }
          return true;
        }
      }
      else
      {
        if (other.inorder)
        {
          // Other is inorder but not contiguous
          // If we're not inorder we can't entail it
          if (!inorder)
            return false;
          // Must be in order but not necessarily contiguous
          // See if our indexes are monotonically increasing 
          for (unsigned idx = 1; idx < field_indexes.size(); idx++)
          {
            // Not monotonically increasing
            if (field_indexes[idx-1] > field_indexes[idx])
              return false;
          }
          return true;
        }
        else
        {
          // Other is neither inorder or contiguous
          // We already know we have all the fields so we are done 
          return true;
        }
      }
    }

    //--------------------------------------------------------------------------
    bool FieldConstraint::conflicts(const FieldConstraint &other) const
    //--------------------------------------------------------------------------
    {
      // If they need inorder and we're not that is bad
      if (!inorder && other.inorder)
        return true;
      // If they need contiguous and we're not that is bad
      if (!contiguous && other.contiguous)
        return true;
      if (other.field_set.empty())
        return false;
      // If we can have any of their fields and we haven't explicitly
      // specified which ones we don't have then we don't conflict
      if (field_set.empty() || other.field_set.empty())
        return false;
      // See if they need us to be inorder
      if (other.inorder)
      {
        // See if they need us to be contiguous
        if (other.contiguous)
        {
          // We must have their fields inorder and contiguous
          unsigned our_index = 0;
          for (/*nothing*/; our_index < field_set.size(); our_index++)
            if (field_set[our_index] == other.field_set[0])
              break;
          // If it doesn't have enough space that is bad
          if ((our_index + other.field_set.size()) > field_set.size())
              return true;
          for (unsigned other_idx = 0; 
                other_idx < other.field_set.size(); other_idx++, our_index++)
            if (field_set[our_index] != other.field_set[other_idx])
              return true;
        }
        else
        {
          // We must have their fields inorder but not contiguous
          unsigned other_index = 0;
          for (unsigned idx = 0; field_set.size(); idx++)
          {
            if (other.field_set[other_index] == field_set[idx])
            {
              other_index++;
              if (other_index == other.field_set.size())
                break;
            }
          }
          return (other_index < other.field_set.size());
        }
      }
      else
      {
        // See if they need us to be contiguous
        if (other.contiguous)
        {
          // We have to have their fields contiguous but not in order
          // Find a start index   
          std::set<FieldID> other_fields(other.field_set.begin(),
                                         other.field_set.end());
          unsigned our_index = 0;
          for (/*nothing*/; our_index < field_set.size(); our_index++)
            if (other_fields.find(field_set[our_index]) != other_fields.end())
              break;
          // If it doesn't have enough space that is bad
          if ((our_index + other_fields.size()) > field_set.size())
              return true;
          for ( /*nothing*/; our_index < other_fields.size(); our_index++)
            if (other_fields.find(field_set[our_index]) == other_fields.end())
              return true;
        }
        else
        {
          // We just have to have their fields in any order
          std::set<FieldID> our_fields(field_set.begin(), field_set.end());
          for (unsigned idx = 0; idx < other.field_set.size(); idx++)
            if (our_fields.find(other.field_set[idx]) == our_fields.end())
              return true;
        }
      }
      return false;  
    }

    //--------------------------------------------------------------------------
    void FieldConstraint::swap(FieldConstraint &rhs)
    //--------------------------------------------------------------------------
    {
      field_set.swap(rhs.field_set);
      SWAP_HELPER(bool, contiguous)
      SWAP_HELPER(bool, inorder)
    }

    //--------------------------------------------------------------------------
    void FieldConstraint::serialize(Serializer &rez) const
    //--------------------------------------------------------------------------
    {
      rez.serialize<bool>(contiguous);
      rez.serialize<bool>(inorder);
      rez.serialize<size_t>(field_set.size());
      for (std::vector<FieldID>::const_iterator it = field_set.begin();
            it != field_set.end(); it++)
        rez.serialize(*it);
    }
    
    //--------------------------------------------------------------------------
    void FieldConstraint::deserialize(Deserializer &derez)
    //--------------------------------------------------------------------------
    {
      derez.deserialize<bool>(contiguous);
      derez.deserialize<bool>(inorder);
      size_t num_orders;
      derez.deserialize(num_orders);
      field_set.resize(num_orders);
      for (std::vector<FieldID>::iterator it = field_set.begin();
            it != field_set.end(); it++)
        derez.deserialize(*it);
    }

    /////////////////////////////////////////////////////////////
    // Ordering Constraint 
    /////////////////////////////////////////////////////////////

    //--------------------------------------------------------------------------
    OrderingConstraint::OrderingConstraint(bool contig)
      : contiguous(contig)
    //--------------------------------------------------------------------------
    {
    }

    //--------------------------------------------------------------------------
    OrderingConstraint::OrderingConstraint(
                           const std::vector<DimensionKind> &order, bool contig)
      : ordering(order), contiguous(contig)
    //--------------------------------------------------------------------------
    {
    }

    //--------------------------------------------------------------------------
    bool OrderingConstraint::entails(const OrderingConstraint &other, 
                                     unsigned total_dims) const
    //--------------------------------------------------------------------------
    {
      if (other.ordering.empty())
        return true;
      // See if we have all the dimensions
      std::vector<unsigned> dim_indexes(ordering.size());
      unsigned local_idx = 0;
      for (std::vector<DimensionKind>::const_iterator it = 
           other.ordering.begin(); it != other.ordering.end(); it++)
      {
        // See if this is a dimension we are still considering
        if (is_skip_dimension(*it, total_dims))
          continue;
        bool found = false;
        for (unsigned idx = 0; idx < ordering.size(); idx++)
        {
          if (ordering[idx] == (*it))
          {
            dim_indexes[local_idx] = idx;
            // If they aren't in the same order, it is no good
            if ((local_idx > 0) && (dim_indexes[local_idx-1] > idx))
              return false;
            found = true;
            // Update the local index
            local_idx++;
            break;
          }
        }
        if (!found)
          return false; // if we don't have the dimension can't entail
      }
      // We don't even have enough fields so no way we can entail

      if (other.contiguous)
      {
        // If we're not contiguous we can't entail the other
        if (!contiguous)
          return false;
        // See if the indexes are contiguous
        std::set<unsigned> sorted_indexes(dim_indexes.begin(), 
                                          dim_indexes.end());
        int previous = -1;
        for (std::set<unsigned>::const_iterator it = sorted_indexes.begin();
              it != sorted_indexes.end(); it++)
        {
          if (previous != -1)
          {
            // Not contiguous
            if ((previous+1) != int(*it))
              return false;
          }
          previous = (*it);
        }
        return true;
      }
      else
      {
        // We've got all the dimensions in the right order so we are good
        return true; 
      }
    }

    //--------------------------------------------------------------------------
    bool OrderingConstraint::conflicts(const OrderingConstraint &other,
                                       unsigned total_dims) const
    //--------------------------------------------------------------------------
    {
      // If they both must be contiguous there is a slightly different check
      if (contiguous && other.contiguous)
      {
        int previous_idx = -1;
        for (std::vector<DimensionKind>::const_iterator it = ordering.begin();
              it != ordering.end(); it++)
        {
          // See if we can skip this dimesion
          if (is_skip_dimension(*it, total_dims))
            continue;
          int next_idx = -1;
          unsigned skipped_dims = 0;
          for (unsigned idx = 0; idx < other.ordering.size(); idx++)
          {
            // See if we can skip this dimension
            if (is_skip_dimension(other.ordering[idx], total_dims))
            {
              skipped_dims++;
              continue;
            }
            if ((*it) == other.ordering[idx])
            {
              // don't include skipped dimensions
              next_idx = idx - skipped_dims;
              break;
            }
          }
          if (next_idx >= 0)
          {
            // This field was in the other set, see if it was in a good place
            if (previous_idx >= 0)
            {
              if (next_idx != (previous_idx+1))
                return true; // conflict
            }
            // Record the previous and keep going
            previous_idx = next_idx;
          }
          else if (previous_idx >= 0)
            return true; // fields are not contiguous
        }
      }
      else
      {
        int previous_idx = -1;
        for (std::vector<DimensionKind>::const_iterator it = ordering.begin();
              it != ordering.end(); it++)
        {
          // See if we can skip this dimension
          if (is_skip_dimension(*it, total_dims))
            continue;
          int next_idx = -1;
          unsigned skipped_dims = 0;
          for (unsigned idx = 0; idx < other.ordering.size(); idx++)
          {
            if (is_skip_dimension(other.ordering[idx], total_dims))
            {
              skipped_dims++;
              continue;
            }
            if ((*it) == other.ordering[idx])
            {
              // Don't include skipped dimensions
              next_idx = idx - skipped_dims;
              break;
            }
          }
          // Only care if we found it
          if (next_idx >= 0)
          {
            if ((previous_idx >= 0) && (next_idx < previous_idx))
              return true; // not in the right order
            // Record this as the previous
            previous_idx = next_idx;
          }
        }
      }
      return false;
    }

    //--------------------------------------------------------------------------
    void OrderingConstraint::swap(OrderingConstraint &rhs)
    //--------------------------------------------------------------------------
    {
      ordering.swap(rhs.ordering);
      SWAP_HELPER(bool, contiguous)
    }

    //--------------------------------------------------------------------------
    void OrderingConstraint::serialize(Serializer &rez) const
    //--------------------------------------------------------------------------
    {
      rez.serialize(contiguous);
      rez.serialize<size_t>(ordering.size());
      for (std::vector<DimensionKind>::const_iterator it = ordering.begin();
            it != ordering.end(); it++)
        rez.serialize(*it);
    }

    //--------------------------------------------------------------------------
    void OrderingConstraint::deserialize(Deserializer &derez)
    //--------------------------------------------------------------------------
    {
      derez.deserialize(contiguous);
      size_t num_orders;
      derez.deserialize(num_orders);
      ordering.resize(num_orders);
      for (std::vector<DimensionKind>::iterator it = ordering.begin();
            it != ordering.end(); it++)
        derez.deserialize(*it);
    }

    //--------------------------------------------------------------------------
    /*static*/ bool OrderingConstraint::is_skip_dimension(DimensionKind dim,
                                                          unsigned total_dims)
    //--------------------------------------------------------------------------
    {
      if (total_dims == 0)
        return false;
      if (dim == LEGION_DIM_F)
        return false;
      if (dim < LEGION_DIM_F)
      {
        // Normal spatial dimension
        if (dim >= total_dims)
          return true;
      }
      else
      {
        // Split spatial dimension
        const unsigned actual_dim = (dim - (LEGION_DIM_F + 1)) / 2;
        if (actual_dim >= total_dims)
          return true;
      }
      return false;
    }

    /////////////////////////////////////////////////////////////
    // Tiling Constraint 
    /////////////////////////////////////////////////////////////

    //--------------------------------------------------------------------------
    TilingConstraint::TilingConstraint(void)
      : tiles(true)
    //--------------------------------------------------------------------------
    {
    }

    //--------------------------------------------------------------------------
    TilingConstraint::TilingConstraint(DimensionKind d)
      : dim(d), tiles(true)
    //--------------------------------------------------------------------------
    {
    }

    //--------------------------------------------------------------------------
    TilingConstraint::TilingConstraint(DimensionKind d, size_t v, bool t)
      : dim(d), value(v), tiles(t)
    //--------------------------------------------------------------------------
    {
    }

    //--------------------------------------------------------------------------
    bool TilingConstraint::entails(const TilingConstraint &other) const
    //--------------------------------------------------------------------------
    {
      if (dim != other.dim)
        return false;
      if (value != other.value)
        return false;
      if (tiles != other.tiles)
        return false;
      return true;
    }

    //--------------------------------------------------------------------------
    bool TilingConstraint::conflicts(const TilingConstraint &other) const
    //--------------------------------------------------------------------------
    {
      if (dim != other.dim)
        return false;
      if (value != other.value)
        return true;
      if (tiles != other.tiles)
        return true;
      return false;
    }

    //--------------------------------------------------------------------------
    void TilingConstraint::swap(TilingConstraint &rhs)
    //--------------------------------------------------------------------------
    {
      SWAP_HELPER(DimensionKind, dim)
      SWAP_HELPER(size_t, value)
      SWAP_HELPER(bool, tiles)
    }

    //--------------------------------------------------------------------------
    void TilingConstraint::serialize(Serializer &rez) const
    //--------------------------------------------------------------------------
    {
      rez.serialize(dim);
      rez.serialize(value);
      rez.serialize(tiles);
    }

    //--------------------------------------------------------------------------
    void TilingConstraint::deserialize(Deserializer &derez)
    //--------------------------------------------------------------------------
    {
      derez.deserialize(dim);
      derez.deserialize(value);
      derez.deserialize(tiles);
    }

    /////////////////////////////////////////////////////////////
    // Dimension Constraint 
    /////////////////////////////////////////////////////////////

    //--------------------------------------------------------------------------
    DimensionConstraint::DimensionConstraint(void)
    //--------------------------------------------------------------------------
    {
    }

    //--------------------------------------------------------------------------
    DimensionConstraint::DimensionConstraint(DimensionKind k, 
                                             EqualityKind eq, size_t val)
      : kind(k), eqk(eq), value(val)
    //--------------------------------------------------------------------------
    {
    }

    //--------------------------------------------------------------------------
    bool DimensionConstraint::entails(const DimensionConstraint &other) const
    //--------------------------------------------------------------------------
    {
      if (kind != other.kind)
        return false;
      if (bound_entails(eqk, value, other.eqk, other.value))
        return true;
      return false;
    }

    //--------------------------------------------------------------------------
    bool DimensionConstraint::conflicts(const DimensionConstraint &other) const
    //--------------------------------------------------------------------------
    {
      if (kind != other.kind)
        return false;
      if (bound_conflicts(eqk, value, other.eqk, other.value))
        return true;
      return false;
    }

    //--------------------------------------------------------------------------
    void DimensionConstraint::swap(DimensionConstraint &rhs)
    //--------------------------------------------------------------------------
    {
      SWAP_HELPER(DimensionKind, kind)
      SWAP_HELPER(EqualityKind, eqk)
      SWAP_HELPER(size_t, value)
    }

    //--------------------------------------------------------------------------
    void DimensionConstraint::serialize(Serializer &rez) const
    //--------------------------------------------------------------------------
    {
      rez.serialize(kind);
      rez.serialize(eqk);
      rez.serialize(value);
    }

    //--------------------------------------------------------------------------
    void DimensionConstraint::deserialize(Deserializer &derez)
    //--------------------------------------------------------------------------
    {
      derez.deserialize(kind);
      derez.deserialize(eqk);
      derez.deserialize(value);
    }

    /////////////////////////////////////////////////////////////
    // Alignment Constraint 
    /////////////////////////////////////////////////////////////

    //--------------------------------------------------------------------------
    AlignmentConstraint::AlignmentConstraint(void)
    //--------------------------------------------------------------------------
    {
    }

    //--------------------------------------------------------------------------
    AlignmentConstraint::AlignmentConstraint(FieldID f, 
                                             EqualityKind eq, size_t align)
      : fid(f), eqk(eq), alignment(align)
    //--------------------------------------------------------------------------
    {
    }

    //--------------------------------------------------------------------------
    bool AlignmentConstraint::entails(const AlignmentConstraint &other) const
    //--------------------------------------------------------------------------
    {
      if (fid != other.fid)
        return false;
      if (bound_entails(eqk, alignment, other.eqk, other.alignment))
        return true;
      return false;
    }

    //--------------------------------------------------------------------------
    bool AlignmentConstraint::conflicts(const AlignmentConstraint &other) const
    //--------------------------------------------------------------------------
    {
      if (fid != other.fid)
        return false;
      if (bound_conflicts(eqk, alignment, other.eqk, other.alignment))
        return true;
      return false;
    }

    //--------------------------------------------------------------------------
    void AlignmentConstraint::swap(AlignmentConstraint &rhs)
    //--------------------------------------------------------------------------
    {
      SWAP_HELPER(FieldID, fid)
      SWAP_HELPER(EqualityKind, eqk)
      SWAP_HELPER(size_t, alignment)
    }

    //--------------------------------------------------------------------------
    void AlignmentConstraint::serialize(Serializer &rez) const
    //--------------------------------------------------------------------------
    {
      rez.serialize(fid);
      rez.serialize(eqk);
      rez.serialize(alignment);
    }

    //--------------------------------------------------------------------------
    void AlignmentConstraint::deserialize(Deserializer &derez)
    //--------------------------------------------------------------------------
    {
      derez.deserialize(fid);
      derez.deserialize(eqk);
      derez.deserialize(alignment);
    }

    /////////////////////////////////////////////////////////////
    // Offset Constraint 
    /////////////////////////////////////////////////////////////

    //--------------------------------------------------------------------------
    OffsetConstraint::OffsetConstraint(void)
    //--------------------------------------------------------------------------
    {
    }

    //--------------------------------------------------------------------------
    OffsetConstraint::OffsetConstraint(FieldID f, size_t off)
      : fid(f), offset(off)
    //--------------------------------------------------------------------------
    {
    }

    //--------------------------------------------------------------------------
    bool OffsetConstraint::entails(const OffsetConstraint &other) const
    //--------------------------------------------------------------------------
    {
      if (fid != other.fid)
        return false;
      if (offset == other.offset)
        return true;
      return false;
    }

    //--------------------------------------------------------------------------
    bool OffsetConstraint::conflicts(const OffsetConstraint &other) const
    //--------------------------------------------------------------------------
    {
      if (fid != other.fid)
        return false;
      if (offset != other.offset)
        return true;
      return false;
    }

    //--------------------------------------------------------------------------
    void OffsetConstraint::swap(OffsetConstraint &rhs)
    //--------------------------------------------------------------------------
    {
      SWAP_HELPER(FieldID, fid)
      SWAP_HELPER(off_t, offset)
    }

    //--------------------------------------------------------------------------
    void OffsetConstraint::serialize(Serializer &rez) const
    //--------------------------------------------------------------------------
    {
      rez.serialize(fid);
      rez.serialize(offset);
    }

    //--------------------------------------------------------------------------
    void OffsetConstraint::deserialize(Deserializer &derez)
    //--------------------------------------------------------------------------
    {
      derez.deserialize(fid);
      derez.deserialize(offset);
    }

    /////////////////////////////////////////////////////////////
    // Pointer Constraint 
    /////////////////////////////////////////////////////////////

    //--------------------------------------------------------------------------
    PointerConstraint::PointerConstraint(void)
      : is_valid(false), memory(Memory::NO_MEMORY), ptr(0)
    //--------------------------------------------------------------------------
    {
    }

    //--------------------------------------------------------------------------
    PointerConstraint::PointerConstraint(Memory m, uintptr_t p)
      : is_valid(true), memory(m), ptr(p)
    //--------------------------------------------------------------------------
    {
    }

    //--------------------------------------------------------------------------
    bool PointerConstraint::entails(const PointerConstraint &other) const
    //--------------------------------------------------------------------------
    {
      if (!other.is_valid)
        return true;
      if (!is_valid)
        return false;
      if (memory != other.memory)
        return false;
      if (ptr != other.ptr)
        return false;
      return true;
    }

    //--------------------------------------------------------------------------
    bool PointerConstraint::conflicts(const PointerConstraint &other) const
    //--------------------------------------------------------------------------
    {
      if (!is_valid || !other.is_valid)
        return false;
      if (memory != other.memory)
        return false;
      if (ptr != other.ptr)
        return true;
      return false;
    }

    //--------------------------------------------------------------------------
    void PointerConstraint::swap(PointerConstraint &rhs)
    //--------------------------------------------------------------------------
    {
      SWAP_HELPER(bool, is_valid)
      SWAP_HELPER(Memory, memory)
      SWAP_HELPER(uintptr_t, ptr)
    }

    //--------------------------------------------------------------------------
    void PointerConstraint::serialize(Serializer &rez) const
    //--------------------------------------------------------------------------
    {
      rez.serialize(is_valid);
      if (is_valid)
      {
        rez.serialize(ptr);
        rez.serialize(memory);
      }
    }

    //--------------------------------------------------------------------------
    void PointerConstraint::deserialize(Deserializer &derez)
    //--------------------------------------------------------------------------
    {
      derez.deserialize(is_valid);
      if (is_valid)
      {
        derez.deserialize(ptr);
        derez.deserialize(memory);
      }
    }

    /////////////////////////////////////////////////////////////
    // Padding Constraint
    /////////////////////////////////////////////////////////////

    //--------------------------------------------------------------------------
    PaddingConstraint::PaddingConstraint(const Domain &del)
      : delta(del)
    //--------------------------------------------------------------------------
    {
    }

    //--------------------------------------------------------------------------
    PaddingConstraint::PaddingConstraint(const DomainPoint &lower,
                                         const DomainPoint &upper)
      : delta(Domain(lower, upper))
    //--------------------------------------------------------------------------
    {
    }

    //--------------------------------------------------------------------------
    bool PaddingConstraint::entails(const PaddingConstraint &other) const
    //--------------------------------------------------------------------------
    {
      if (other.delta.get_dim() > 0)
      {
        if (delta.get_dim() != other.delta.get_dim())
          return false;
        for (int idx = 0; idx < delta.get_dim(); idx++)
        {
          if (other.delta.lo()[idx] >= 0)
          {
            if (other.delta.lo()[idx] == 0)
            {
              if (delta.lo()[idx] != 0)
                return false;
            }
            else if (delta.lo()[idx] < other.delta.lo()[idx])
              return false;
          }
          if (other.delta.hi()[idx] >= 0)
          {
            if (other.delta.hi()[idx] == 0)
            {
              if (delta.hi()[idx] != 0)
                return false;
            }
            else if (delta.hi()[idx] < other.delta.hi()[idx])
              return false;
          }
        }
      }
      return true;
    }

    //--------------------------------------------------------------------------
    bool PaddingConstraint::conflicts(const PaddingConstraint &other) const
    //--------------------------------------------------------------------------
    {
      if ((delta.get_dim() > 0) && (other.delta.get_dim() > 0))
      {
        if (delta.get_dim() != other.delta.get_dim())
          return true;
        for (int idx = 0; idx < delta.get_dim(); idx++)
        {
          if ((delta.lo()[idx] >= 0) && (other.delta.lo()[idx] >= 0))
          {
            if ((delta.lo()[idx] == 0) || (other.delta.lo()[idx] == 0))
            {
              if (delta.lo()[idx] != other.delta.lo()[idx])
                return true;
            }
            else if (delta.lo()[idx] < other.delta.lo()[idx])
              return true;
          }
          if ((delta.hi()[idx] >= 0) && (other.delta.hi()[idx] >= 0))
          {
            if ((delta.hi()[idx] == 0) || (other.delta.hi()[idx] == 0))
            {
              if (delta.hi()[idx] != other.delta.hi()[idx])
                return true;
            }
            else if (delta.hi()[idx] < other.delta.hi()[idx])
              return true;
          }
        }
      }
      return false;
    }

    //--------------------------------------------------------------------------
    void PaddingConstraint::swap(PaddingConstraint &rhs)
    //--------------------------------------------------------------------------
    {
      SWAP_HELPER(Domain, delta)
    }

    //--------------------------------------------------------------------------
    void PaddingConstraint::serialize(Serializer &rez) const
    //--------------------------------------------------------------------------
    {
      rez.serialize(delta);
    }

    //--------------------------------------------------------------------------
    void PaddingConstraint::deserialize(Deserializer &derez)
    //--------------------------------------------------------------------------
    {
      derez.deserialize(delta);
    }

    /////////////////////////////////////////////////////////////
    // Layout Constraint Set 
    /////////////////////////////////////////////////////////////

    //--------------------------------------------------------------------------
    LayoutConstraintSet& LayoutConstraintSet::add_constraint(
                                        const SpecializedConstraint &constraint)
    //--------------------------------------------------------------------------
    {
      specialized_constraint = constraint;
      return *this;
    }

    //--------------------------------------------------------------------------
    LayoutConstraintSet& LayoutConstraintSet::add_constraint(
                                             const MemoryConstraint &constraint)
    //--------------------------------------------------------------------------
    {
      memory_constraint = constraint;
      return *this;
    }

    //--------------------------------------------------------------------------
    LayoutConstraintSet& LayoutConstraintSet::add_constraint(
                                           const OrderingConstraint &constraint)
    //--------------------------------------------------------------------------
    {
      ordering_constraint = constraint;
      return *this;
    }

    //--------------------------------------------------------------------------
    LayoutConstraintSet& LayoutConstraintSet::add_constraint(
                                             const TilingConstraint &constraint)
    //--------------------------------------------------------------------------
    {
      tiling_constraints.push_back(constraint);
      return *this;
    }

    //--------------------------------------------------------------------------
    LayoutConstraintSet& LayoutConstraintSet::add_constraint(
                                              const FieldConstraint &constraint)
    //--------------------------------------------------------------------------
    {
      field_constraint = constraint;
      return *this;
    }

    //--------------------------------------------------------------------------
    LayoutConstraintSet& LayoutConstraintSet::add_constraint(
                                          const DimensionConstraint &constraint)
    //--------------------------------------------------------------------------
    {
      dimension_constraints.push_back(constraint);
      return *this;
    }

    //--------------------------------------------------------------------------
    LayoutConstraintSet& LayoutConstraintSet::add_constraint(
                                          const AlignmentConstraint &constraint)
    //--------------------------------------------------------------------------
    {
      alignment_constraints.push_back(constraint);
      return *this;
    }

    //--------------------------------------------------------------------------
    LayoutConstraintSet& LayoutConstraintSet::add_constraint(
                                             const OffsetConstraint &constraint)
    //--------------------------------------------------------------------------
    {
      offset_constraints.push_back(constraint);
      return *this;
    }

    //--------------------------------------------------------------------------
    LayoutConstraintSet& LayoutConstraintSet::add_constraint(
                                            const PointerConstraint &constraint)
    //--------------------------------------------------------------------------
    {
      pointer_constraint = constraint;
      return *this;
    }

    //--------------------------------------------------------------------------
    LayoutConstraintSet& LayoutConstraintSet::add_constraint(
                                            const PaddingConstraint &constraint)
    //--------------------------------------------------------------------------
    {
      padding_constraint = constraint;
      return *this;
    }

    //--------------------------------------------------------------------------
    bool LayoutConstraintSet::operator==(const LayoutConstraintSet &other) const
    //--------------------------------------------------------------------------
    {
      return equals(other);
    }

    //--------------------------------------------------------------------------
    bool LayoutConstraintSet::operator!=(const LayoutConstraintSet &other) const
    //--------------------------------------------------------------------------
    {
      return !equals(other);
    }

    //--------------------------------------------------------------------------
    bool LayoutConstraintSet::equals(const LayoutConstraintSet &other,
                        LayoutConstraintKind *bad_kind, size_t *bad_index) const
    //--------------------------------------------------------------------------
    {
      if (specialized_constraint != other.specialized_constraint)
      {
        if (bad_kind != NULL)
          *bad_kind = LEGION_SPECIALIZED_CONSTRAINT;
        if (bad_index != NULL)
          *bad_index = 0;
        return false;
      }
      if (field_constraint != other.field_constraint)
      {
        if (bad_kind != NULL)
          *bad_kind = LEGION_FIELD_CONSTRAINT;
        if (bad_index != NULL)
          *bad_index = 0;
        return false;
      }
      if (memory_constraint != other.memory_constraint)
      {
        if (bad_kind != NULL)
          *bad_kind = LEGION_MEMORY_CONSTRAINT;
        if (bad_index != NULL)
          *bad_index = 0;
        return false;
      }
      if (pointer_constraint != other.pointer_constraint)
      {
        if (bad_kind != NULL)
          *bad_kind = LEGION_POINTER_CONSTRAINT;
        if (bad_index != NULL)
          *bad_index = 0;
        return false;
      }
      if (ordering_constraint != other.ordering_constraint)
      {
        if (bad_kind != NULL)
          *bad_kind = LEGION_ORDERING_CONSTRAINT;
        if (bad_index != NULL)
          *bad_index = 0;
        return false;
      }
      if (padding_constraint != other.padding_constraint)
      {
        if (bad_kind != NULL)
          *bad_kind = LEGION_PADDING_CONSTRAINT;
        if (bad_index != NULL)
          *bad_index = 0;
        return false;
      }
      if (tiling_constraints.size() != other.tiling_constraints.size())
      {
        if (bad_kind != NULL)
          *bad_kind = LEGION_TILING_CONSTRAINT;
        if (bad_index != NULL)
          *bad_index = 0;
        return false;
      }
      for (unsigned idx = 0; idx < tiling_constraints.size(); idx++)
      {
        bool found = false;
        for (std::vector<TilingConstraint>::const_iterator it =
              other.tiling_constraints.begin(); it !=
              other.tiling_constraints.end(); it++)
        {
          if (tiling_constraints[idx] != *it)
            continue;
          found = true;
          break;
        }
        if (!found)
        {
          if (bad_kind != NULL)
            *bad_kind = LEGION_TILING_CONSTRAINT;
          if (bad_index != NULL)
            *bad_index = idx;
          return false;
        }
      }
      if (dimension_constraints.size() != other.dimension_constraints.size())
      {
        if (bad_kind != NULL)
          *bad_kind = LEGION_DIMENSION_CONSTRAINT;
        if (bad_index != NULL)
          *bad_index = 0;
        return false;
      }
      for (unsigned idx = 0; idx < dimension_constraints.size(); idx++)
      {
        bool found = false;
        for (std::vector<DimensionConstraint>::const_iterator it =
              other.dimension_constraints.begin(); it !=
              other.dimension_constraints.end(); it++)
        {
          if (dimension_constraints[idx] != *it)
            continue;
          found = true;
          break;
        }
        if (!found)
        {
          if (bad_kind != NULL)
            *bad_kind = LEGION_DIMENSION_CONSTRAINT;
          if (bad_index != NULL)
            *bad_index = idx;
          return false;
        }
      }
      if (alignment_constraints.size() != other.alignment_constraints.size())
      {
        if (bad_kind != NULL)
          *bad_kind = LEGION_ALIGNMENT_CONSTRAINT;
        if (bad_index != NULL)
          *bad_index = 0;
        return false;
      }
      for (unsigned idx = 0; idx < alignment_constraints.size(); idx++)
      {
        bool found = false;
        for (std::vector<AlignmentConstraint>::const_iterator it =
              other.alignment_constraints.begin(); it !=
              other.alignment_constraints.end(); it++)
        {
          if (alignment_constraints[idx] != *it)
            continue;
          found = true;
          break;
        }
        if (!found)
        {
          if (bad_kind != NULL)
            *bad_kind = LEGION_ALIGNMENT_CONSTRAINT;
          if (bad_index != NULL)
            *bad_index = idx;
          return false;
        }
      }
      if (offset_constraints.size() != other.offset_constraints.size())
      {
        if (bad_kind != NULL)
          *bad_kind = LEGION_OFFSET_CONSTRAINT;
        if (bad_index != NULL)
          *bad_index = 0;
        return false;
      }
      for (unsigned idx = 0; idx < offset_constraints.size(); idx++)
      {
        bool found = false;
        for (std::vector<OffsetConstraint>::const_iterator it =
              other.offset_constraints.begin(); it !=
              other.offset_constraints.end(); it++)
        {
          if (offset_constraints[idx] != *it)
            continue;
          found = true;
          break;
        }
        if (!found)
        {
          if (bad_kind != NULL)
            *bad_kind = LEGION_OFFSET_CONSTRAINT;
          if (bad_index != NULL)
            *bad_index = idx;
          return false;
        }
      }
      return true;
    }

    //--------------------------------------------------------------------------
    bool LayoutConstraintSet::entails(const LayoutConstraintSet &other,
                                      unsigned total_dims,
                                      const LayoutConstraint **failed,
                                      bool test_pointer) const
    //--------------------------------------------------------------------------
    {
      if (!specialized_constraint.entails(other.specialized_constraint))
      {
        if (failed != NULL)
          *failed = &other.specialized_constraint;
        return false;
      }
      if (!field_constraint.entails(other.field_constraint))
      {
        if (failed != NULL)
          *failed = &other.field_constraint;
        return false;
      }
      if (!memory_constraint.entails(other.memory_constraint))
      {
        if (failed != NULL)
          *failed = &other.memory_constraint;
        return false;
      }
      if (test_pointer && !pointer_constraint.entails(other.pointer_constraint))
      {
        if (failed != NULL)
          *failed = &other.pointer_constraint;
        return false;
      }
      if (!padding_constraint.entails(other.padding_constraint))
      {
        if (failed != NULL)
          *failed = &other.padding_constraint;
        return false;
      }
      if (!ordering_constraint.entails(other.ordering_constraint, total_dims))
      {
        if (failed != NULL)
          *failed = &other.ordering_constraint;
        return false;
      }
      for (std::vector<TilingConstraint>::const_iterator it = 
            other.tiling_constraints.begin(); it !=
            other.tiling_constraints.end(); it++)
      {
        bool entailed = false;
        for (unsigned idx = 0; idx < tiling_constraints.size(); idx++)
        {
          if (tiling_constraints[idx].entails(*it))
          {
            entailed = true;
            break;
          }
        }
        if (!entailed)
        {
          if (failed != NULL)
            *failed = &(*it);
          return false;
        }
      }
      for (std::vector<DimensionConstraint>::const_iterator it = 
            other.dimension_constraints.begin(); it != 
            other.dimension_constraints.end(); it++)
      {
        bool entailed = false;
        for (unsigned idx = 0; idx < dimension_constraints.size(); idx++)
        {
          if (dimension_constraints[idx].entails(*it))
          {
            entailed = true;
            break;
          }
        }
        if (!entailed)
        {
          if (failed != NULL)
            *failed = &(*it);
          return false;
        }
      }
      for (std::vector<AlignmentConstraint>::const_iterator it = 
            other.alignment_constraints.begin(); it != 
            other.alignment_constraints.end(); it++)
      {
        bool entailed = false;
        for (unsigned idx = 0; idx < alignment_constraints.size(); idx++)
        {
          if (alignment_constraints[idx].entails(*it))
          {
            entailed = true;
            break;
          }
        }
        if (!entailed)
        {
          if (failed != NULL)
            *failed = &(*it);
          return false;
        }
      }
      for (std::vector<OffsetConstraint>::const_iterator it = 
            other.offset_constraints.begin(); it != 
            other.offset_constraints.end(); it++)
      {
        bool entailed = false;
        for (unsigned idx = 0; idx < offset_constraints.size(); idx++)
        {
          if (offset_constraints[idx].entails(*it))
          {
            entailed = true;
            break;
          }
        }
        if (!entailed)
        {
          if (failed != NULL)
            *failed = &(*it);
          return false;
        }
      }
      return true;
    }

    //--------------------------------------------------------------------------
    bool LayoutConstraintSet::conflicts(const LayoutConstraintSet &other,
                                        unsigned total_dims,
                                        const LayoutConstraint **conflict) const
    //--------------------------------------------------------------------------
    {
      // Do these in order
      if (specialized_constraint.conflicts(other.specialized_constraint))
      {
        if (conflict != NULL)
          *conflict = &specialized_constraint;
        return true;
      }
      if (field_constraint.conflicts(other.field_constraint))
      {
        if (conflict != NULL)
          *conflict = &field_constraint;
        return true;
      }
      if (memory_constraint.conflicts(other.memory_constraint))
      {
        if (conflict != NULL)
          *conflict = &memory_constraint;
        return true;
      }
      if (pointer_constraint.conflicts(other.pointer_constraint))
      {
        if (conflict != NULL)
          *conflict = &pointer_constraint;
        return true;
      }
      if (padding_constraint.conflicts(other.padding_constraint))
      {
        if (conflict != NULL)
          *conflict = &padding_constraint;
        return true;
      }
      if (ordering_constraint.conflicts(other.ordering_constraint, total_dims))
      {
        if (conflict != NULL)
          *conflict = &ordering_constraint;
        return true;
      }
      for (std::vector<TilingConstraint>::const_iterator it = 
            tiling_constraints.begin(); it != 
            tiling_constraints.end(); it++)
      {
        for (unsigned idx = 0; idx < other.tiling_constraints.size(); idx++)
          if (it->conflicts(other.tiling_constraints[idx]))
          {
            if (conflict != NULL)
              *conflict = &(*it);
            return true;
          }
      }
      for (std::vector<DimensionConstraint>::const_iterator it = 
            dimension_constraints.begin(); it !=
            dimension_constraints.end(); it++)
      {
        for (unsigned idx = 0; idx < other.dimension_constraints.size(); idx++)
          if (it->conflicts(other.dimension_constraints[idx]))
          {
            if (conflict != NULL)
              *conflict = &(*it);
            return true;
          }
      }
      for (std::vector<AlignmentConstraint>::const_iterator it = 
            alignment_constraints.begin(); it !=
            alignment_constraints.end(); it++)
      {
        for (unsigned idx = 0; idx < other.alignment_constraints.size(); idx++)
          if (it->conflicts(other.alignment_constraints[idx]))
          {
            if (conflict != NULL)
              *conflict = &(*it);
            return true;
          }
      }
      for (std::vector<OffsetConstraint>::const_iterator it = 
            offset_constraints.begin(); it != 
            offset_constraints.end(); it++)
      {
        for (unsigned idx = 0; idx < other.offset_constraints.size(); idx++)
          if (it->conflicts(other.offset_constraints[idx]))
          {
            if (conflict != NULL)
              *conflict = &(*it);
            return true;
          }
      }
      return false;
    }

    //--------------------------------------------------------------------------
    const LayoutConstraint* LayoutConstraintSet::convert_unsatisfied(
                                LayoutConstraintKind kind, unsigned index) const
    //--------------------------------------------------------------------------
    {
      switch (kind)
      {
        case LEGION_SPECIALIZED_CONSTRAINT:
          return &specialized_constraint;
        case LEGION_MEMORY_CONSTRAINT:
          return &memory_constraint;
        case LEGION_FIELD_CONSTRAINT:
          return &field_constraint;
        case LEGION_ORDERING_CONSTRAINT:
          return &ordering_constraint;
        case LEGION_POINTER_CONSTRAINT:
          return &pointer_constraint;
        case LEGION_PADDING_CONSTRAINT:
          return &padding_constraint;
        case LEGION_TILING_CONSTRAINT:
          return &tiling_constraints[index];
        case LEGION_DIMENSION_CONSTRAINT:
          return &dimension_constraints[index];
        case LEGION_ALIGNMENT_CONSTRAINT:
          return &alignment_constraints[index];
        case LEGION_OFFSET_CONSTRAINT:
          return &offset_constraints[index];
        default:
          assert(false);
      }
      return NULL;
    }

    //--------------------------------------------------------------------------
    void LayoutConstraintSet::swap(LayoutConstraintSet &rhs)
    //--------------------------------------------------------------------------
    {
      specialized_constraint.swap(rhs.specialized_constraint);
      field_constraint.swap(rhs.field_constraint);
      memory_constraint.swap(rhs.memory_constraint);
      pointer_constraint.swap(rhs.pointer_constraint);
      padding_constraint.swap(rhs.padding_constraint);
      ordering_constraint.swap(rhs.ordering_constraint);
      tiling_constraints.swap(rhs.tiling_constraints);
      dimension_constraints.swap(rhs.dimension_constraints);
      alignment_constraints.swap(rhs.alignment_constraints);
      offset_constraints.swap(rhs.offset_constraints);
    }

    //--------------------------------------------------------------------------
    void LayoutConstraintSet::serialize(Serializer &rez) const
    //--------------------------------------------------------------------------
    {
      specialized_constraint.serialize(rez);
      field_constraint.serialize(rez);
      memory_constraint.serialize(rez);
      pointer_constraint.serialize(rez);
      padding_constraint.serialize(rez);
      ordering_constraint.serialize(rez);
#define PACK_CONSTRAINTS(Type, constraints)                             \
      rez.serialize<size_t>(constraints.size());                        \
      for (std::vector<Type>::const_iterator it = constraints.begin();  \
            it != constraints.end(); it++)                              \
      {                                                                 \
        it->serialize(rez);                                             \
      }
      PACK_CONSTRAINTS(TilingConstraint, tiling_constraints)
      PACK_CONSTRAINTS(DimensionConstraint, dimension_constraints)
      PACK_CONSTRAINTS(AlignmentConstraint, alignment_constraints)
      PACK_CONSTRAINTS(OffsetConstraint, offset_constraints)
#undef PACK_CONSTRAINTS
    }

    //--------------------------------------------------------------------------
    void LayoutConstraintSet::deserialize(Deserializer &derez)
    //--------------------------------------------------------------------------
    {
      specialized_constraint.deserialize(derez);
      field_constraint.deserialize(derez);
      memory_constraint.deserialize(derez);
      pointer_constraint.deserialize(derez);
      padding_constraint.deserialize(derez);
      ordering_constraint.deserialize(derez);
#define UNPACK_CONSTRAINTS(Type, constraints)                       \
      {                                                             \
        size_t constraint_size;                                     \
        derez.deserialize(constraint_size);                         \
        constraints.resize(constraint_size);                        \
        for (std::vector<Type>::iterator it = constraints.begin();  \
              it != constraints.end(); it++)                        \
        {                                                           \
          it->deserialize(derez);                                   \
        }                                                           \
      }
      UNPACK_CONSTRAINTS(TilingConstraint, tiling_constraints)
      UNPACK_CONSTRAINTS(DimensionConstraint, dimension_constraints)
      UNPACK_CONSTRAINTS(AlignmentConstraint, alignment_constraints)
      UNPACK_CONSTRAINTS(OffsetConstraint, offset_constraints)
#undef UNPACK_CONSTRAINTS
    }

    /////////////////////////////////////////////////////////////
    // Task Layout Constraint Set 
    /////////////////////////////////////////////////////////////

    //--------------------------------------------------------------------------
    TaskLayoutConstraintSet& TaskLayoutConstraintSet::add_layout_constraint(
                                          unsigned idx, LayoutConstraintID desc)
    //--------------------------------------------------------------------------
    {
      layouts.insert(std::pair<unsigned,LayoutConstraintID>(idx, desc));
      return *this;
    }

    //--------------------------------------------------------------------------
    void TaskLayoutConstraintSet::swap(TaskLayoutConstraintSet &rhs)
    //--------------------------------------------------------------------------
    {
      layouts.swap(rhs.layouts);
    }

    //--------------------------------------------------------------------------
    void TaskLayoutConstraintSet::serialize(Serializer &rez) const
    //--------------------------------------------------------------------------
    {
      rez.serialize<size_t>(layouts.size());
      for (std::multimap<unsigned,LayoutConstraintID>::const_iterator it = 
            layouts.begin(); it != layouts.end(); it++)
      {
        rez.serialize(it->first);
        rez.serialize(it->second);
      }
    }

    //--------------------------------------------------------------------------
    void TaskLayoutConstraintSet::deserialize(Deserializer &derez)
    //--------------------------------------------------------------------------
    {
      size_t num_layouts;
      derez.deserialize(num_layouts);
      for (unsigned idx = 0; idx < num_layouts; idx++)
      {
        std::pair<unsigned,LayoutConstraintID> pair;
        derez.deserialize(pair.first);
        derez.deserialize(pair.second);
        layouts.insert(pair);
      }
    }

}; // namespace Legion

// EOF

