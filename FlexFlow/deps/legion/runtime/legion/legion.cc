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

#include "legion.h"
#include "legion/runtime.h"
#include "legion/legion_ops.h"
#include "legion/legion_tasks.h"
#include "legion/legion_context.h"
#include "legion/legion_profiling.h"
#include "legion/legion_allocation.h"

namespace Legion {
    namespace Internal {
      LEGION_EXTERN_LOGGER_DECLARATIONS
    };

    // Make sure all the handle types are trivially copyable.

    // Note: GCC 4.9 breaks even with C++11, so for now peg this on
    // C++14 until we deprecate GCC 4.9 support.
#if !defined(__GNUC__) || (__GNUC__ >= 5)
    static_assert(std::is_trivially_copyable<IndexSpace>::value,
                  "IndexSpace is not trivially copyable");
    static_assert(std::is_trivially_copyable<IndexPartition>::value,
                  "IndexPartition is not trivially copyable");
    static_assert(std::is_trivially_copyable<FieldSpace>::value,
                  "FieldSpace is not trivially copyable");
    static_assert(std::is_trivially_copyable<LogicalRegion>::value,
                  "LogicalRegion is not trivially copyable");
    static_assert(std::is_trivially_copyable<LogicalPartition>::value,
                  "LogicalPartition is not trivially copyable");
#define DIMFUNC(DIM) \
    static_assert(std::is_trivially_copyable<IndexSpaceT<DIM> >::value, \
                  "IndexSpaceT is not trivially copyable"); \
    static_assert(std::is_trivially_copyable<IndexPartitionT<DIM> >::value, \
                  "IndexPartitionT is not trivially copyable"); \
    static_assert(std::is_trivially_copyable<LogicalRegionT<DIM> >::value, \
                  "LogicalRegionT is not trivially copyable"); \
    static_assert(std::is_trivially_copyable<LogicalPartitionT<DIM> >::value, \
                  "LogicalPartitionT is not trivially copyable");
    LEGION_FOREACH_N(DIMFUNC)
#undef DIMFUNC
#endif

    const LogicalRegion LogicalRegion::NO_REGION = LogicalRegion();
    const LogicalPartition LogicalPartition::NO_PART = LogicalPartition();  
    const Domain Domain::NO_DOMAIN = Domain();

    // Cache static type tags so we don't need to recompute them all the time
#define DIMFUNC(DIM) \
    static const TypeTag TYPE_TAG_##DIM##D = \
      Internal::NT_TemplateHelper::encode_tag<DIM,coord_t>();
    LEGION_FOREACH_N(DIMFUNC)
#undef DIMFUNC 

    /////////////////////////////////////////////////////////////
    // Mappable 
    /////////////////////////////////////////////////////////////

    //--------------------------------------------------------------------------
    Mappable::Mappable(void)
      : map_id(0),tag(0),parent_task(NULL),mapper_data(NULL),mapper_data_size(0)
    //--------------------------------------------------------------------------
    {
    }

    /////////////////////////////////////////////////////////////
    // Task 
    /////////////////////////////////////////////////////////////

    //--------------------------------------------------------------------------
    Task::Task(void)
      : Mappable(), task_id(0), args(NULL), arglen(0), is_index_space(false),
        must_epoch_task(false), local_args(NULL), local_arglen(0),
        steal_count(0),stealable(false),speculated(false),local_function(false)
    //--------------------------------------------------------------------------
    {
    }

    /////////////////////////////////////////////////////////////
    // Copy 
    /////////////////////////////////////////////////////////////

    //--------------------------------------------------------------------------
    Copy::Copy(void)
      : Mappable(), is_index_space(false)
    //--------------------------------------------------------------------------
    {
    }

    /////////////////////////////////////////////////////////////
    // Inline Mapping 
    /////////////////////////////////////////////////////////////

    //--------------------------------------------------------------------------
    InlineMapping::InlineMapping(void)
      : Mappable(), layout_constraint_id(0)
    //--------------------------------------------------------------------------
    {
    }

    /////////////////////////////////////////////////////////////
    // Acquire 
    /////////////////////////////////////////////////////////////

    //--------------------------------------------------------------------------
    Acquire::Acquire(void)
      : Mappable()
    //--------------------------------------------------------------------------
    {
    }

    /////////////////////////////////////////////////////////////
    // Release 
    /////////////////////////////////////////////////////////////

    //--------------------------------------------------------------------------
    Release::Release(void)
      : Mappable()
    //--------------------------------------------------------------------------
    {
    }

    /////////////////////////////////////////////////////////////
    // Close 
    /////////////////////////////////////////////////////////////

    //--------------------------------------------------------------------------
    Close::Close(void)
      : Mappable()
    //--------------------------------------------------------------------------
    {
    }

    /////////////////////////////////////////////////////////////
    // Fill 
    /////////////////////////////////////////////////////////////

    //--------------------------------------------------------------------------
    Fill::Fill(void)
      : Mappable(), is_index_space(false)
    //--------------------------------------------------------------------------
    {
    }

    /////////////////////////////////////////////////////////////
    // Partition
    /////////////////////////////////////////////////////////////

    //--------------------------------------------------------------------------
    Partition::Partition(void)
      : Mappable(), is_index_space(false)
    //--------------------------------------------------------------------------
    {
    }

    /////////////////////////////////////////////////////////////
    // MustEpoch 
    /////////////////////////////////////////////////////////////

    //--------------------------------------------------------------------------
    MustEpoch::MustEpoch(void)
      : Mappable()
    //--------------------------------------------------------------------------
    {
    }

    /////////////////////////////////////////////////////////////
    // IndexSpace 
    /////////////////////////////////////////////////////////////

    /*static*/ const IndexSpace IndexSpace::NO_SPACE = IndexSpace();

    //--------------------------------------------------------------------------
    IndexSpace::IndexSpace(IndexSpaceID _id, IndexTreeID _tid, TypeTag _tag)
      : id(_id), tid(_tid), type_tag(_tag)
    //--------------------------------------------------------------------------
    {
    }

    //--------------------------------------------------------------------------
    IndexSpace::IndexSpace(void)
      : id(0), tid(0), type_tag(0)
    //--------------------------------------------------------------------------
    {
    }

    /////////////////////////////////////////////////////////////
    // IndexPartition 
    /////////////////////////////////////////////////////////////

    /*static*/ const IndexPartition IndexPartition::NO_PART = IndexPartition();

    //--------------------------------------------------------------------------
    IndexPartition::IndexPartition(IndexPartitionID _id, 
                                   IndexTreeID _tid, TypeTag _tag)
      : id(_id), tid(_tid), type_tag(_tag)
    //--------------------------------------------------------------------------
    {
    }

    //--------------------------------------------------------------------------
    IndexPartition::IndexPartition(void)
      : id(0), tid(0), type_tag(0)
    //--------------------------------------------------------------------------
    {
    }

    /////////////////////////////////////////////////////////////
    // FieldSpace 
    /////////////////////////////////////////////////////////////

    /*static*/ const FieldSpace FieldSpace::NO_SPACE = FieldSpace(0);

    //--------------------------------------------------------------------------
    FieldSpace::FieldSpace(unsigned _id)
      : id(_id)
    //--------------------------------------------------------------------------
    {
    }

    //--------------------------------------------------------------------------
    FieldSpace::FieldSpace(void)
      : id(0)
    //--------------------------------------------------------------------------
    {
    }
    
    /////////////////////////////////////////////////////////////
    // Logical Region  
    /////////////////////////////////////////////////////////////

    //--------------------------------------------------------------------------
    LogicalRegion::LogicalRegion(RegionTreeID tid, IndexSpace index, 
                                 FieldSpace field)
      : tree_id(tid), index_space(index), field_space(field)
    //--------------------------------------------------------------------------
    {
    }

    //--------------------------------------------------------------------------
    LogicalRegion::LogicalRegion(void)
      : tree_id(0), index_space(IndexSpace::NO_SPACE), 
        field_space(FieldSpace::NO_SPACE)
    //--------------------------------------------------------------------------
    {
    }

    /////////////////////////////////////////////////////////////
    // Logical Partition 
    /////////////////////////////////////////////////////////////

    //--------------------------------------------------------------------------
    LogicalPartition::LogicalPartition(RegionTreeID tid, IndexPartition pid, 
                                       FieldSpace field)
      : tree_id(tid), index_partition(pid), field_space(field)
    //--------------------------------------------------------------------------
    {
    }

    //--------------------------------------------------------------------------
    LogicalPartition::LogicalPartition(void)
      : tree_id(0), index_partition(IndexPartition::NO_PART), 
        field_space(FieldSpace::NO_SPACE)
    //--------------------------------------------------------------------------
    {
    }

    /////////////////////////////////////////////////////////////
    // Argument Map 
    /////////////////////////////////////////////////////////////

    //--------------------------------------------------------------------------
    ArgumentMap::ArgumentMap(void)
    //--------------------------------------------------------------------------
    {
      impl = new Internal::ArgumentMapImpl();
#ifdef DEBUG_LEGION
      assert(impl != NULL);
#endif
      impl->add_reference();
    }

    //--------------------------------------------------------------------------
    ArgumentMap::ArgumentMap(const FutureMap &rhs)
    //--------------------------------------------------------------------------
    {
      impl = new Internal::ArgumentMapImpl(rhs);
#ifdef DEBUG_LEGION
      assert(impl != NULL);
#endif
      impl->add_reference();
    }

    //--------------------------------------------------------------------------
    ArgumentMap::ArgumentMap(const ArgumentMap &rhs)
      : impl(rhs.impl)
    //--------------------------------------------------------------------------
    {
      if (impl != NULL)
        impl->add_reference();
    }

    //--------------------------------------------------------------------------
    ArgumentMap::ArgumentMap(ArgumentMap &&rhs) noexcept
      : impl(rhs.impl)
    //--------------------------------------------------------------------------
    {
      rhs.impl = NULL;
    }

    //--------------------------------------------------------------------------
    ArgumentMap::ArgumentMap(Internal::ArgumentMapImpl *i)
      : impl(i)
    //--------------------------------------------------------------------------
    {
      if (impl != NULL)
        impl->add_reference();
    }

    //--------------------------------------------------------------------------
    ArgumentMap::~ArgumentMap(void)
    //--------------------------------------------------------------------------
    {
      if (impl != NULL)
      {
        // Remove our reference and if we were the
        // last reference holder, then delete it
        if (impl->remove_reference())
        {
          delete impl;
        }
        impl = NULL;
      }
    }

    //--------------------------------------------------------------------------
    ArgumentMap& ArgumentMap::operator=(const FutureMap &rhs)
    //--------------------------------------------------------------------------
    {
      // Check to see if our current impl is not NULL,
      // if so remove our reference
      if (impl != NULL)
      {
        if (impl->remove_reference())
        {
          delete impl;
        }
      }
      impl = new Internal::ArgumentMapImpl(rhs);
      impl->add_reference();
      return *this;
    }
    
    //--------------------------------------------------------------------------
    ArgumentMap& ArgumentMap::operator=(const ArgumentMap &rhs)
    //--------------------------------------------------------------------------
    {
      // Check to see if our current impl is not NULL,
      // if so remove our reference
      if (impl != NULL)
      {
        if (impl->remove_reference())
        {
          delete impl;
        }
      }
      impl = rhs.impl;
      // Add our reference to the new impl
      if (impl != NULL)
      {
        impl->add_reference();
      }
      return *this;
    }

    //--------------------------------------------------------------------------
    ArgumentMap& ArgumentMap::operator=(ArgumentMap &&rhs) noexcept
    //--------------------------------------------------------------------------
    {
      if ((impl != NULL) && impl->remove_reference())
        delete impl;
      impl = rhs.impl;
      rhs.impl = NULL;
      return *this;
    }

    //--------------------------------------------------------------------------
    bool ArgumentMap::has_point(const DomainPoint &point)
    //--------------------------------------------------------------------------
    {
#ifdef DEBUG_LEGION
      assert(impl != NULL);
#endif
      return impl->has_point(point);
    }

    //--------------------------------------------------------------------------
    void ArgumentMap::set_point(const DomainPoint &point, 
                                const UntypedBuffer &arg, bool replace/*=true*/)
    //--------------------------------------------------------------------------
    {
#ifdef DEBUG_LEGION
      assert(impl != NULL);
#endif
      impl->set_point(point, arg, replace);
    }

    //--------------------------------------------------------------------------
    void ArgumentMap::set_point(const DomainPoint &point, 
                                const Future &f, bool replace/*= true*/)
    //--------------------------------------------------------------------------
    {
#ifdef DEBUG_LEGION
      assert(impl != NULL);
#endif
      impl->set_point(point, f, replace);
    }

    //--------------------------------------------------------------------------
    bool ArgumentMap::remove_point(const DomainPoint &point)
    //--------------------------------------------------------------------------
    {
#ifdef DEBUG_LEGION
      assert(impl != NULL);
#endif
      return impl->remove_point(point);
    }

    //--------------------------------------------------------------------------
    UntypedBuffer ArgumentMap::get_point(const DomainPoint &point) const
    //--------------------------------------------------------------------------
    {
#ifdef DEBUG_LEGION
      assert(impl != NULL);
#endif
      return impl->get_point(point);
    }

    /////////////////////////////////////////////////////////////
    // Predicate 
    /////////////////////////////////////////////////////////////

    const Predicate Predicate::TRUE_PRED = Predicate(true);
    const Predicate Predicate::FALSE_PRED = Predicate(false);

    //--------------------------------------------------------------------------
    Predicate::Predicate(void)
      : impl(NULL), const_value(true)
    //--------------------------------------------------------------------------
    {
    }

    //--------------------------------------------------------------------------
    Predicate::Predicate(const Predicate &p)
    //--------------------------------------------------------------------------
    {
      const_value = p.const_value;
      impl = p.impl;
      if (impl != NULL)
        impl->add_reference();
    }

    //--------------------------------------------------------------------------
    Predicate::Predicate(Predicate &&p) noexcept
    //--------------------------------------------------------------------------
    {
      const_value = p.const_value;
      impl = p.impl;
      p.impl = NULL;
    }

    //--------------------------------------------------------------------------
    Predicate::Predicate(bool value)
      : impl(NULL), const_value(value)
    //--------------------------------------------------------------------------
    {
    }

    //--------------------------------------------------------------------------
    Predicate::Predicate(Internal::PredicateImpl *i)
      : impl(i)
    //--------------------------------------------------------------------------
    {
      if (impl != NULL)
        impl->add_reference();
    }

    //--------------------------------------------------------------------------
    Predicate::~Predicate(void)
    //--------------------------------------------------------------------------
    {
      if ((impl != NULL) && impl->remove_reference())
        delete impl;
    }

    //--------------------------------------------------------------------------
    Predicate& Predicate::operator=(const Predicate &rhs)
    //--------------------------------------------------------------------------
    {
      if ((impl != NULL) && impl->remove_reference())
        delete impl;
      const_value = rhs.const_value;
      impl = rhs.impl;
      if (impl != NULL)
        impl->add_reference();
      return *this;
    }

    //--------------------------------------------------------------------------
    Predicate& Predicate::operator=(Predicate &&rhs) noexcept
    //--------------------------------------------------------------------------
    {
      if ((impl != NULL) && impl->remove_reference())
        delete impl;
      const_value = rhs.const_value;
      impl = rhs.impl;
      rhs.impl = NULL;
      return *this;
    }

    /////////////////////////////////////////////////////////////
    // Lock 
    /////////////////////////////////////////////////////////////

    //--------------------------------------------------------------------------
    Lock::Lock(void)
      : reservation_lock(Reservation::NO_RESERVATION)
    //--------------------------------------------------------------------------
    {
    }

    //--------------------------------------------------------------------------
    Lock::Lock(Reservation r)
      : reservation_lock(r)
    //--------------------------------------------------------------------------
    {
    }

    //--------------------------------------------------------------------------
    bool Lock::operator<(const Lock &rhs) const
    //--------------------------------------------------------------------------
    {
      return (reservation_lock < rhs.reservation_lock);
    }

    //--------------------------------------------------------------------------
    bool Lock::operator==(const Lock &rhs) const
    //--------------------------------------------------------------------------
    {
      return (reservation_lock == rhs.reservation_lock);
    }

    //--------------------------------------------------------------------------
    void Lock::acquire(unsigned mode /*=0*/, bool exclusive /*=true*/)
    //--------------------------------------------------------------------------
    {
#ifdef DEBUG_LEGION
      assert(reservation_lock.exists());
#endif
      Internal::ApEvent lock_event(reservation_lock.acquire(mode,exclusive));
      bool poisoned = false;
      lock_event.wait_faultaware(poisoned);
      if (poisoned)
        Internal::implicit_context->raise_poison_exception();
    }

    //--------------------------------------------------------------------------
    void Lock::release(void)
    //--------------------------------------------------------------------------
    {
#ifdef DEBUG_LEGION
      assert(reservation_lock.exists());
#endif
      reservation_lock.release();
    }

    /////////////////////////////////////////////////////////////
    // Lock Request
    /////////////////////////////////////////////////////////////

    //--------------------------------------------------------------------------
    LockRequest::LockRequest(Lock l, unsigned m, bool excl)
      : lock(l), mode(m), exclusive(excl)
    //--------------------------------------------------------------------------
    {
    }

    /////////////////////////////////////////////////////////////
    // Grant 
    /////////////////////////////////////////////////////////////

    //--------------------------------------------------------------------------
    Grant::Grant(void)
      : impl(NULL)
    //--------------------------------------------------------------------------
    {
    }

    //--------------------------------------------------------------------------
    Grant::Grant(Internal::GrantImpl *i)
      : impl(i)
    //--------------------------------------------------------------------------
    {
      if (impl != NULL)
        impl->add_reference();
    }

    //--------------------------------------------------------------------------
    Grant::Grant(const Grant &rhs)
      : impl(rhs.impl)
    //--------------------------------------------------------------------------
    {
      if (impl != NULL)
        impl->add_reference();
    }

    //--------------------------------------------------------------------------
    Grant::~Grant(void)
    //--------------------------------------------------------------------------
    {
      if (impl != NULL)
      {
        if (impl->remove_reference())
          delete impl;
        impl = NULL;
      }
    }

    //--------------------------------------------------------------------------
    Grant& Grant::operator=(const Grant &rhs)
    //--------------------------------------------------------------------------
    {
      if (impl != NULL)
      {
        if (impl->remove_reference())
          delete impl;
      }
      impl = rhs.impl;
      if (impl != NULL)
        impl->add_reference();
      return *this;
    }

    /////////////////////////////////////////////////////////////
    // Phase Barrier 
    /////////////////////////////////////////////////////////////

    //--------------------------------------------------------------------------
    PhaseBarrier::PhaseBarrier(void)
      : phase_barrier(Internal::ApBarrier::NO_AP_BARRIER)
    //--------------------------------------------------------------------------
    {
    }

    //--------------------------------------------------------------------------
    PhaseBarrier::PhaseBarrier(Internal::ApBarrier b)
      : phase_barrier(b)
    //--------------------------------------------------------------------------
    {
    }

    //--------------------------------------------------------------------------
    bool PhaseBarrier::operator<(const PhaseBarrier &rhs) const
    //--------------------------------------------------------------------------
    {
      return (phase_barrier < rhs.phase_barrier);
    }

    //--------------------------------------------------------------------------
    bool PhaseBarrier::operator==(const PhaseBarrier &rhs) const
    //--------------------------------------------------------------------------
    {
      return (phase_barrier == rhs.phase_barrier);
    }

    //--------------------------------------------------------------------------
    bool PhaseBarrier::operator!=(const PhaseBarrier &rhs) const
    //--------------------------------------------------------------------------
    {
      return (phase_barrier != rhs.phase_barrier);
    }

    //--------------------------------------------------------------------------
    void PhaseBarrier::arrive(unsigned count /*=1*/)
    //--------------------------------------------------------------------------
    {
#ifdef DEBUG_LEGION
      assert(phase_barrier.exists());
#endif
      Internal::Runtime::phase_barrier_arrive(*this, count);
    }

    //--------------------------------------------------------------------------
    void PhaseBarrier::wait(void)
    //--------------------------------------------------------------------------
    {
#ifdef DEBUG_LEGION
      assert(phase_barrier.exists());
#endif
      Internal::ApEvent e = Internal::Runtime::get_previous_phase(*this);
      bool poisoned = false;
      e.wait_faultaware(poisoned);
      if (poisoned)
        Internal::implicit_context->raise_poison_exception();
    }

    //--------------------------------------------------------------------------
    void PhaseBarrier::alter_arrival_count(int delta)
    //--------------------------------------------------------------------------
    {
      Internal::Runtime::alter_arrival_count(*this, delta);
    }

    //--------------------------------------------------------------------------
    bool PhaseBarrier::exists(void) const
    //--------------------------------------------------------------------------
    {
      return phase_barrier.exists();
    }

    /////////////////////////////////////////////////////////////
    // Dynamic Collective 
    /////////////////////////////////////////////////////////////

    //--------------------------------------------------------------------------
    DynamicCollective::DynamicCollective(void)
      : PhaseBarrier(), redop(0)
    //--------------------------------------------------------------------------
    {
    }

    //--------------------------------------------------------------------------
    DynamicCollective::DynamicCollective(Internal::ApBarrier b, ReductionOpID r)
      : PhaseBarrier(b), redop(r)
    //--------------------------------------------------------------------------
    {
    }

    //--------------------------------------------------------------------------
    void DynamicCollective::arrive(const void *value, size_t size, 
                                   unsigned count /*=1*/)
    //--------------------------------------------------------------------------
    {
      Internal::Runtime::phase_barrier_arrive(*this, count, 
                                  Internal::ApEvent::NO_AP_EVENT, value, size);
    }

    /////////////////////////////////////////////////////////////
    // Region Requirement 
    /////////////////////////////////////////////////////////////

    //--------------------------------------------------------------------------
    RegionRequirement::RegionRequirement(void)
      : region(LogicalRegion::NO_REGION), partition(LogicalPartition::NO_PART),
        privilege(LEGION_NO_ACCESS), prop(LEGION_EXCLUSIVE), 
        parent(LogicalRegion::NO_REGION), redop(0), tag(0), 
        flags(LEGION_NO_FLAG), handle_type(LEGION_SINGULAR_PROJECTION), 
        projection(0), projection_args(NULL), projection_args_size(0)
    //--------------------------------------------------------------------------
    {
    }

    //--------------------------------------------------------------------------
    RegionRequirement::RegionRequirement(LogicalRegion _handle, 
                                        const std::set<FieldID> &priv_fields,
                                        const std::vector<FieldID> &inst_fields,
                                        PrivilegeMode _priv, 
                                        CoherenceProperty _prop, 
                                        LogicalRegion _parent,
				        MappingTagID _tag, bool _verified)
      : region(_handle), privilege(_priv), prop(_prop), parent(_parent),
        redop(0), tag(_tag), flags(_verified ? LEGION_VERIFIED_FLAG : 
            LEGION_NO_FLAG), handle_type(LEGION_SINGULAR_PROJECTION), 
        projection(0), projection_args(NULL), projection_args_size(0)
    //--------------------------------------------------------------------------
    { 
      privilege_fields = priv_fields;
      instance_fields = inst_fields;
      // For backwards compatibility with the old encoding
      if (privilege == LEGION_WRITE_PRIV)
        privilege = LEGION_WRITE_DISCARD;
#ifdef DEBUG_LEGION
      if (IS_REDUCE(*this)) // Shouldn't use this constructor for reductions
        REPORT_LEGION_ERROR(ERROR_USE_REDUCTION_REGION_REQ, 
                                   "Use different RegionRequirement "
                            "constructor for reductions");
#endif
    }

    //--------------------------------------------------------------------------
    RegionRequirement::RegionRequirement(LogicalPartition pid, 
                ProjectionID _proj, 
                const std::set<FieldID> &priv_fields,
                const std::vector<FieldID> &inst_fields,
                PrivilegeMode _priv, CoherenceProperty _prop,
                LogicalRegion _parent, MappingTagID _tag, bool _verified)
      : partition(pid), privilege(_priv), prop(_prop), parent(_parent),
        redop(0), tag(_tag), flags(_verified ? LEGION_VERIFIED_FLAG : 
            LEGION_NO_FLAG), handle_type(LEGION_PARTITION_PROJECTION), 
        projection(_proj), projection_args(NULL), projection_args_size(0)
    //--------------------------------------------------------------------------
    { 
      privilege_fields = priv_fields;
      instance_fields = inst_fields;
      // For backwards compatibility with the old encoding
      if (privilege == LEGION_WRITE_PRIV)
        privilege = LEGION_WRITE_DISCARD;
#ifdef DEBUG_LEGION
      if (IS_REDUCE(*this))
        REPORT_LEGION_ERROR(ERROR_USE_REDUCTION_REGION_REQ, 
                                   "Use different RegionRequirement "
                            "constructor for reductions");
#endif
    }

    //--------------------------------------------------------------------------
    RegionRequirement::RegionRequirement(LogicalRegion _handle, 
                ProjectionID _proj,
                const std::set<FieldID> &priv_fields,
                const std::vector<FieldID> &inst_fields,
                PrivilegeMode _priv, CoherenceProperty _prop,
                LogicalRegion _parent, MappingTagID _tag, bool _verified)
      : region(_handle), privilege(_priv), prop(_prop), parent(_parent),
        redop(0), tag(_tag), flags(_verified ? LEGION_VERIFIED_FLAG : 
            LEGION_NO_FLAG), handle_type(LEGION_REGION_PROJECTION), 
        projection(_proj), projection_args(NULL), projection_args_size(0)
    //--------------------------------------------------------------------------
    {
      privilege_fields = priv_fields;
      instance_fields = inst_fields;
      // For backwards compatibility with the old encoding
      if (privilege == LEGION_WRITE_PRIV)
        privilege = LEGION_WRITE_DISCARD;
#ifdef DEBUG_LEGION
      if (IS_REDUCE(*this))
        REPORT_LEGION_ERROR(ERROR_USE_REDUCTION_REGION_REQ, 
                                   "Use different RegionRequirement "
                                   "constructor for reductions")
#endif
    }

    //--------------------------------------------------------------------------
    RegionRequirement::RegionRequirement(LogicalRegion _handle,  
                                    const std::set<FieldID> &priv_fields,
                                    const std::vector<FieldID> &inst_fields,
                                    ReductionOpID op, CoherenceProperty _prop, 
                                    LogicalRegion _parent, MappingTagID _tag, 
                                    bool _verified)
      : region(_handle), privilege(LEGION_REDUCE), prop(_prop), parent(_parent),
        redop(op), tag(_tag), flags(_verified ? LEGION_VERIFIED_FLAG : 
            LEGION_NO_FLAG), handle_type(LEGION_SINGULAR_PROJECTION), 
        projection(0), projection_args(NULL), projection_args_size(0)
    //--------------------------------------------------------------------------
    {
      privilege_fields = priv_fields;
      instance_fields = inst_fields;
#ifdef DEBUG_LEGION
      if (redop == 0)
        REPORT_LEGION_ERROR(ERROR_RESERVED_REDOP_ID, 
                                   "Zero is not a valid ReductionOpID")
#endif
    }

    //--------------------------------------------------------------------------
    RegionRequirement::RegionRequirement(LogicalPartition pid, 
                        ProjectionID _proj,  
                        const std::set<FieldID> &priv_fields,
                        const std::vector<FieldID> &inst_fields,
                        ReductionOpID op, CoherenceProperty _prop,
                        LogicalRegion _parent, MappingTagID _tag, 
                        bool _verified)
      : partition(pid), privilege(LEGION_REDUCE), prop(_prop), parent(_parent),
        redop(op), tag(_tag), flags(_verified ? LEGION_VERIFIED_FLAG : 
            LEGION_NO_FLAG), handle_type(LEGION_PARTITION_PROJECTION), 
        projection(_proj), projection_args(NULL), projection_args_size(0)
    //--------------------------------------------------------------------------
    {
      privilege_fields = priv_fields;
      instance_fields = inst_fields;
#ifdef DEBUG_LEGION
      if (redop == 0)
        REPORT_LEGION_ERROR(ERROR_RESERVED_REDOP_ID, 
                                   "Zero is not a valid ReductionOpID")        
#endif
    }

    //--------------------------------------------------------------------------
    RegionRequirement::RegionRequirement(LogicalRegion _handle, 
                        ProjectionID _proj,
                        const std::set<FieldID> &priv_fields,
                        const std::vector<FieldID> &inst_fields,
                        ReductionOpID op, CoherenceProperty _prop,
                        LogicalRegion _parent, MappingTagID _tag, 
                        bool _verified)
      : region(_handle), privilege(LEGION_REDUCE), prop(_prop), parent(_parent),
        redop(op), tag(_tag), flags(_verified ? LEGION_VERIFIED_FLAG : 
            LEGION_NO_FLAG), handle_type(LEGION_REGION_PROJECTION), 
        projection(_proj), projection_args(NULL), projection_args_size(0)
    //--------------------------------------------------------------------------
    {
      privilege_fields = priv_fields;
      instance_fields = inst_fields;
#ifdef DEBUG_LEGION
      if (redop == 0)
        REPORT_LEGION_ERROR(ERROR_RESERVED_REDOP_ID, 
                                   "Zero is not a valid ReductionOpID")
#endif
    }

    //--------------------------------------------------------------------------
    RegionRequirement::RegionRequirement(LogicalRegion _handle, 
                                         PrivilegeMode _priv, 
                                         CoherenceProperty _prop, 
                                         LogicalRegion _parent,
					 MappingTagID _tag, 
                                         bool _verified)
      : region(_handle), privilege(_priv), prop(_prop), parent(_parent),
        redop(0), tag(_tag), flags(_verified ? LEGION_VERIFIED_FLAG : 
            LEGION_NO_FLAG), handle_type(LEGION_SINGULAR_PROJECTION), 
        projection(), projection_args(NULL), projection_args_size(0)
    //--------------------------------------------------------------------------
    { 
      // For backwards compatibility with the old encoding
      if (privilege == LEGION_WRITE_PRIV)
        privilege = LEGION_WRITE_DISCARD;
#ifdef DEBUG_LEGION
      if (IS_REDUCE(*this)) // Shouldn't use this constructor for reductions
        REPORT_LEGION_ERROR(ERROR_USE_REDUCTION_REGION_REQ, 
                                   "Use different RegionRequirement "
                                   "constructor for reductions")
#endif
    }

    //--------------------------------------------------------------------------
    RegionRequirement::RegionRequirement(LogicalPartition pid, 
                                         ProjectionID _proj, 
                                         PrivilegeMode _priv, 
                                         CoherenceProperty _prop,
                                         LogicalRegion _parent, 
                                         MappingTagID _tag, 
                                         bool _verified)
      : partition(pid), privilege(_priv), prop(_prop), parent(_parent),
        redop(0), tag(_tag), flags(_verified ? LEGION_VERIFIED_FLAG : 
            LEGION_NO_FLAG), handle_type(LEGION_PARTITION_PROJECTION), 
        projection(_proj), projection_args(NULL), projection_args_size(0)
    //--------------------------------------------------------------------------
    { 
      // For backwards compatibility with the old encoding
      if (privilege == LEGION_WRITE_PRIV)
        privilege = LEGION_WRITE_DISCARD;
#ifdef DEBUG_LEGION
      if (IS_REDUCE(*this))
        REPORT_LEGION_ERROR(ERROR_USE_REDUCTION_REGION_REQ, 
                                   "Use different RegionRequirement "
                                   "constructor for reductions")
#endif
    }

    //--------------------------------------------------------------------------
    RegionRequirement::RegionRequirement(LogicalRegion _handle, 
                                         ProjectionID _proj,
                                         PrivilegeMode _priv, 
                                         CoherenceProperty _prop,
                                         LogicalRegion _parent, 
                                         MappingTagID _tag, 
                                         bool _verified)
      : region(_handle), privilege(_priv), prop(_prop), parent(_parent),
        redop(0), tag(_tag), flags(_verified ? LEGION_VERIFIED_FLAG : 
            LEGION_NO_FLAG), handle_type(LEGION_REGION_PROJECTION), 
        projection(_proj), projection_args(NULL), projection_args_size(0)
    //--------------------------------------------------------------------------
    {
      // For backwards compatibility with the old encoding
      if (privilege == LEGION_WRITE_PRIV)
        privilege = LEGION_WRITE_DISCARD;
#ifdef DEBUG_LEGION
      if (IS_REDUCE(*this))
        REPORT_LEGION_ERROR(ERROR_USE_REDUCTION_REGION_REQ, 
                                   "Use different RegionRequirement "
                                   "constructor for reductions")
#endif
    }

    //--------------------------------------------------------------------------
    RegionRequirement::RegionRequirement(LogicalRegion _handle,  
                                         ReductionOpID op, 
                                         CoherenceProperty _prop, 
                                         LogicalRegion _parent, 
                                         MappingTagID _tag, 
                                         bool _verified)
      : region(_handle), privilege(LEGION_REDUCE), prop(_prop), parent(_parent),
        redop(op), tag(_tag), flags(_verified ? LEGION_VERIFIED_FLAG : 
            LEGION_NO_FLAG), handle_type(LEGION_SINGULAR_PROJECTION), 
        projection(0), projection_args(NULL), projection_args_size(0)
    //--------------------------------------------------------------------------
    {
#ifdef DEBUG_LEGION
      if (redop == 0)
        REPORT_LEGION_ERROR(ERROR_RESERVED_REDOP_ID, 
                                   "Zero is not a valid ReductionOpID")        
#endif
    }

    //--------------------------------------------------------------------------
    RegionRequirement::RegionRequirement(LogicalPartition pid, 
                                         ProjectionID _proj,  
                                         ReductionOpID op, 
                                         CoherenceProperty _prop,
                                         LogicalRegion _parent, 
                                         MappingTagID _tag, 
                                         bool _verified)
      : partition(pid), privilege(LEGION_REDUCE), prop(_prop), parent(_parent),
        redop(op), tag(_tag), flags(_verified ? LEGION_VERIFIED_FLAG : 
            LEGION_NO_FLAG), handle_type(LEGION_PARTITION_PROJECTION), 
        projection(_proj), projection_args(NULL), projection_args_size(0)
    //--------------------------------------------------------------------------
    {
#ifdef DEBUG_LEGION
      if (redop == 0)
        REPORT_LEGION_ERROR(ERROR_RESERVED_REDOP_ID, 
                                   "Zero is not a valid ReductionOpID")
#endif
    }

    //--------------------------------------------------------------------------
    RegionRequirement::RegionRequirement(LogicalRegion _handle, 
                                         ProjectionID _proj,
                                         ReductionOpID op, 
                                         CoherenceProperty _prop,
                                         LogicalRegion _parent, 
                                         MappingTagID _tag, 
                                         bool _verified)
      : region(_handle), privilege(LEGION_REDUCE), prop(_prop), parent(_parent),
        redop(op), tag(_tag), flags(_verified ? LEGION_VERIFIED_FLAG : 
            LEGION_NO_FLAG), handle_type(LEGION_REGION_PROJECTION), 
        projection(_proj), projection_args(NULL), projection_args_size(0)
    //--------------------------------------------------------------------------
    {
#ifdef DEBUG_LEGION
      if (redop == 0)
        REPORT_LEGION_ERROR(ERROR_RESERVED_REDOP_ID, 
                                   "Zero is not a valid ReductionOpID")
#endif
    }

    //--------------------------------------------------------------------------
    RegionRequirement::RegionRequirement(const RegionRequirement &rhs)
      : region(rhs.region), partition(rhs.partition), 
        privilege_fields(rhs.privilege_fields), 
        instance_fields(rhs.instance_fields), privilege(rhs.privilege),
        prop(rhs.prop), parent(rhs.parent), redop(rhs.redop), tag(rhs.tag),
        flags(rhs.flags), handle_type(rhs.handle_type), 
        projection(rhs.projection), projection_args(NULL),
        projection_args_size(rhs.projection_args_size)
    //--------------------------------------------------------------------------
    {
      if (projection_args_size > 0)
      {
        projection_args = malloc(projection_args_size);
        memcpy(projection_args, rhs.projection_args, projection_args_size);
      }
    }

    //--------------------------------------------------------------------------
    RegionRequirement::~RegionRequirement(void)
    //--------------------------------------------------------------------------
    {
      if (projection_args_size > 0)
        free(projection_args);
    }

    //--------------------------------------------------------------------------
    RegionRequirement& RegionRequirement::operator=(
                                                   const RegionRequirement &rhs)
    //--------------------------------------------------------------------------
    {
      region = rhs.region;
      partition = rhs.partition;
      privilege_fields = rhs.privilege_fields;
      instance_fields = rhs.instance_fields;
      privilege = rhs.privilege;
      prop = rhs.prop;
      parent = rhs.parent;
      redop = rhs.redop;
      tag = rhs.tag;
      flags = rhs.flags;
      handle_type = rhs.handle_type;
      projection = rhs.projection;
      projection_args_size = rhs.projection_args_size;
      if (projection_args != NULL)
      {
        free(projection_args);
        projection_args = NULL;
      }
      if (projection_args_size > 0)
      {
        projection_args = malloc(projection_args_size);
        memcpy(projection_args, rhs.projection_args, projection_args_size);
      }
      return *this;
    }

    //--------------------------------------------------------------------------
    bool RegionRequirement::operator==(const RegionRequirement &rhs) const
    //--------------------------------------------------------------------------
    {
      if ((handle_type == rhs.handle_type) && (privilege == rhs.privilege) &&
          (prop == rhs.prop) && (parent == rhs.parent) && (redop == rhs.redop)
          && (tag == rhs.tag) && (flags == rhs.flags))
      {
        if (((handle_type == LEGION_SINGULAR_PROJECTION) && 
              (region == rhs.region)) ||
            ((handle_type == LEGION_PARTITION_PROJECTION) && 
             (partition == rhs.partition) && (projection == rhs.projection)) ||
            ((handle_type == LEGION_REGION_PROJECTION) && 
             (region == rhs.region)))
        {
          if ((privilege_fields.size() == rhs.privilege_fields.size()) &&
              (instance_fields.size() == rhs.instance_fields.size()))
          {
            if (projection_args_size == rhs.projection_args_size)
            {
              if ((projection_args_size == 0) ||
                  (memcmp(projection_args, rhs.projection_args, 
                          projection_args_size) == 0))
              {
                return ((privilege_fields == rhs.privilege_fields) 
                    && (instance_fields == rhs.instance_fields));
              }
            }
          }
        }
      }
      return false;
    }

    //--------------------------------------------------------------------------
    bool RegionRequirement::operator<(const RegionRequirement &rhs) const
    //--------------------------------------------------------------------------
    {
      if (handle_type < rhs.handle_type)
        return true;
      else if (handle_type > rhs.handle_type)
        return false;
      else
      {
        if (privilege < rhs.privilege)
          return true;
        else if (privilege > rhs.privilege)
          return false;
        else
        {
          if (prop < rhs.prop)
            return true;
          else if (prop > rhs.prop)
            return false;
          else
          {
            if (parent < rhs.parent)
              return true;
            else if (!(parent == rhs.parent)) // therefore greater than
              return false;
            else
            {
              if (redop < rhs.redop)
                return true;
              else if (redop > rhs.redop)
                return false;
              else
              {
                if (tag < rhs.tag)
                  return true;
                else if (tag > rhs.tag)
                  return false;
                else
                {
                  if (flags < rhs.flags)
                    return true;
                  else if (flags > rhs.flags)
                    return false;
                  else
                  {
                    if (privilege_fields < rhs.privilege_fields)
                      return true;
                    else if (privilege_fields > rhs.privilege_fields)
                      return false;
                    else
                    {
                      if (instance_fields < rhs.instance_fields)
                        return true;
                      else if (instance_fields > rhs.instance_fields)
                        return false;
                      else
                      {
                        if (handle_type == LEGION_SINGULAR_PROJECTION)
                          return (region < rhs.region);
                        else if (projection_args_size < 
                                  rhs.projection_args_size)
                          return true;
                        else if (projection_args_size > 
                                  rhs.projection_args_size)
                          return false;
                        else if ((projection_args_size > 0) &&
                            (memcmp(projection_args, rhs.projection_args, 
                                    projection_args_size) < 0))
                          return true;
                        else if ((projection_args_size > 0) && 
                            (memcmp(projection_args, rhs.projection_args,
                                    projection_args_size) > 0))
                          return false;
                        else if (handle_type == LEGION_PARTITION_PROJECTION)
                        {
                          if (partition < rhs.partition)
                            return true;
                          // therefore greater than
                          else if (partition != rhs.partition) 
                            return false;
                          else
                            return (projection < rhs.projection);
                        }
                        else
                        {
                          if (region < rhs.region)
                            return true;
                          else if (region != rhs.region)
                            return false;
                          else
                            return (projection < rhs.projection);
                        }
                      }
                    }
                  }
                }
              }
            }
          }
        }
      }
    }

#ifdef LEGION_PRIVILEGE_CHECKS
    //--------------------------------------------------------------------------
    unsigned RegionRequirement::get_accessor_privilege(void) const
    //--------------------------------------------------------------------------
    {
      switch (privilege)
      {
        case LEGION_NO_ACCESS:
          return LegionRuntime::ACCESSOR_NONE;
        case LEGION_READ_ONLY:
          return LegionRuntime::ACCESSOR_READ;
        case LEGION_READ_WRITE:
        case LEGION_WRITE_DISCARD:
          return LegionRuntime::ACCESSOR_ALL;
        case LEGION_REDUCE:
          return LegionRuntime::ACCESSOR_REDUCE;
        default:
          assert(false);
      }
      return LegionRuntime::ACCESSOR_NONE;
    }
#endif

    //--------------------------------------------------------------------------
    bool RegionRequirement::has_field_privilege(FieldID fid) const
    //--------------------------------------------------------------------------
    {
      return (privilege_fields.find(fid) != privilege_fields.end());
    }

    //--------------------------------------------------------------------------
    const void* RegionRequirement::get_projection_args(size_t *size) const
    //--------------------------------------------------------------------------
    {
      if (size != NULL)
        *size = projection_args_size;
      return projection_args;
    }

    //--------------------------------------------------------------------------
    void RegionRequirement::set_projection_args(const void *args, size_t size,
                                                bool own)
    //--------------------------------------------------------------------------
    {
      if (projection_args_size > 0)
      {
        free(projection_args);
        projection_args = NULL;
      }
      projection_args_size = size;
      if (projection_args_size > 0)
      {
        if (!own)
        {
          projection_args = malloc(projection_args_size);
          memcpy(projection_args, args, projection_args_size);
        }
        else
          projection_args = const_cast<void*>(args);
      }
    }

    /////////////////////////////////////////////////////////////
    // Output Requirement
    /////////////////////////////////////////////////////////////

    //--------------------------------------------------------------------------
    OutputRequirement::OutputRequirement(bool valid)
      : RegionRequirement(), type_tag(TYPE_TAG_1D),
        field_space(FieldSpace::NO_SPACE),
        global_indexing(false), valid_requirement(valid),
        color_space(IndexSpace::NO_SPACE)
    //--------------------------------------------------------------------------
    {
    }

    //--------------------------------------------------------------------------
    OutputRequirement::OutputRequirement(const RegionRequirement &req)
      : RegionRequirement(req), type_tag(req.parent.get_type_tag()),
        global_indexing(false), valid_requirement(true),
        color_space(IndexSpace::NO_SPACE)
    //--------------------------------------------------------------------------
    {
    }

    //--------------------------------------------------------------------------
    OutputRequirement::OutputRequirement(FieldSpace _field_space,
                                        const std::set<FieldID> &fields,
                                        int dim /*=1*/,
                                        bool _global_indexing /*=false*/)
      : RegionRequirement(), field_space(_field_space),
        global_indexing(_global_indexing), valid_requirement(false),
        color_space(IndexSpace::NO_SPACE)
    //--------------------------------------------------------------------------
    {
      switch (dim)
      {
#define DIMFUNC(DIM)                      \
        case DIM:                         \
          {                               \
            type_tag = TYPE_TAG_##DIM##D; \
            break;                        \
          }
        LEGION_FOREACH_N(DIMFUNC)
#undef DIMFUNC
        default:
          assert(false);
      }
      for (std::set<FieldID>::const_iterator it = fields.begin();
           it != fields.end(); ++it)
        RegionRequirement::add_field(*it);
    }

    //--------------------------------------------------------------------------
    OutputRequirement::OutputRequirement(const OutputRequirement &other)
      : RegionRequirement(static_cast<const RegionRequirement&>(other)),
        type_tag(other.type_tag), field_space(other.field_space),
        global_indexing(other.global_indexing),
        valid_requirement(other.valid_requirement),
        color_space(other.color_space)
    //--------------------------------------------------------------------------
    {
    }

    //--------------------------------------------------------------------------
    OutputRequirement::~OutputRequirement(void)
    //--------------------------------------------------------------------------
    {
    }

    //--------------------------------------------------------------------------
    OutputRequirement& OutputRequirement::operator=(
                                                   const OutputRequirement &rhs)
    //--------------------------------------------------------------------------
    {
      static_cast<RegionRequirement&>(*this) =
        static_cast<const RegionRequirement&>(rhs);
      field_space = rhs.field_space;
      global_indexing = rhs.global_indexing;
      valid_requirement = rhs.valid_requirement;
      type_tag = rhs.type_tag;
      color_space = rhs.color_space;
      return *this;
    }

    //--------------------------------------------------------------------------
    OutputRequirement& OutputRequirement::operator=(
                                                   const RegionRequirement &rhs)
    //--------------------------------------------------------------------------
    {
      static_cast<RegionRequirement&>(*this) =
        static_cast<const RegionRequirement&>(rhs);
      field_space = FieldSpace::NO_SPACE;
      global_indexing = false;
      valid_requirement = true;
      type_tag = rhs.region.get_type_tag();
      color_space = IndexSpace::NO_SPACE;
      return *this;
    }

    //--------------------------------------------------------------------------
    bool OutputRequirement::operator==(const OutputRequirement &rhs) const
    //--------------------------------------------------------------------------
    {
      if ((field_space != rhs.field_space) ||
          (global_indexing != rhs.global_indexing) ||
          (valid_requirement != rhs.valid_requirement) ||
          (color_space != rhs.color_space))
        return false;
      return static_cast<const RegionRequirement&>(*this) ==
             static_cast<const RegionRequirement&>(rhs);
    }

    //--------------------------------------------------------------------------
    bool OutputRequirement::operator<(const OutputRequirement &rhs) const
    //--------------------------------------------------------------------------
    {
      if (field_space < rhs.field_space)
        return true;
      if(field_space > rhs.field_space)
        return false;
      if (global_indexing < rhs.global_indexing)
        return true;
      if (global_indexing > rhs.global_indexing)
        return false;
      if (valid_requirement < rhs.valid_requirement)
        return true;
      if (valid_requirement > rhs.valid_requirement)
        return false;
      if (color_space < rhs.color_space)
        return true;
      if (color_space > rhs.color_space)
        return false;
      return static_cast<const RegionRequirement&>(*this) <
             static_cast<const RegionRequirement&>(rhs);
    }

    //--------------------------------------------------------------------------
    void OutputRequirement::set_projection(ProjectionID proj, IndexSpace cspace)
    //--------------------------------------------------------------------------
    {
      projection = proj;
      color_space = cspace;
    }

    /////////////////////////////////////////////////////////////
    // Index Space Requirement 
    /////////////////////////////////////////////////////////////

    //--------------------------------------------------------------------------
    IndexSpaceRequirement::IndexSpaceRequirement(void)
      : handle(IndexSpace::NO_SPACE), privilege(LEGION_NO_MEMORY), 
        parent(IndexSpace::NO_SPACE), verified(false)
    //--------------------------------------------------------------------------
    {
    }

    //--------------------------------------------------------------------------
    IndexSpaceRequirement::IndexSpaceRequirement(IndexSpace _handle, 
                                                 AllocateMode _priv,
                                                 IndexSpace _parent, 
                                                 bool _verified /*=false*/)
      : handle(_handle), privilege(_priv), parent(_parent), verified(_verified)
    //--------------------------------------------------------------------------
    {
    }

    //--------------------------------------------------------------------------
    bool IndexSpaceRequirement::operator<(
                                        const IndexSpaceRequirement &rhs) const
    //--------------------------------------------------------------------------
    {
      if (handle < rhs.handle)
        return true;
      else if (handle != rhs.handle) // therefore greater than
        return false;
      else
      {
        if (privilege < rhs.privilege)
          return true;
        else if (privilege > rhs.privilege)
          return false;
        else
        {
          if (parent < rhs.parent)
            return true;
          else if (parent != rhs.parent) // therefore greater than
            return false;
          else
            return verified < rhs.verified;
        }
      }
    }

    //--------------------------------------------------------------------------
    bool IndexSpaceRequirement::operator==(
                                        const IndexSpaceRequirement &rhs) const
    //--------------------------------------------------------------------------
    {
      return (handle == rhs.handle) && (privilege == rhs.privilege) &&
             (parent == rhs.parent) && (verified == rhs.verified);
    }

    /////////////////////////////////////////////////////////////
    // Field Space Requirement 
    /////////////////////////////////////////////////////////////

    //--------------------------------------------------------------------------
    FieldSpaceRequirement::FieldSpaceRequirement(void)
      : handle(FieldSpace::NO_SPACE),privilege(LEGION_NO_MEMORY),verified(false)
    //--------------------------------------------------------------------------
    {
    }

    //--------------------------------------------------------------------------
    FieldSpaceRequirement::FieldSpaceRequirement(FieldSpace _handle, 
                                                 AllocateMode _priv,
                                                 bool _verified /*=false*/)
      : handle(_handle), privilege(_priv), verified(_verified)
    //--------------------------------------------------------------------------
    {
    }

    //--------------------------------------------------------------------------
    bool FieldSpaceRequirement::operator<(
                                        const FieldSpaceRequirement &rhs) const
    //--------------------------------------------------------------------------
    {
      if (handle < rhs.handle)
        return true;
      else if (!(handle == rhs.handle)) // therefore greater than
        return false;
      else
      {
        if (privilege < rhs.privilege)
          return true;
        else if (privilege > rhs.privilege)
          return false;
        else
          return verified < rhs.verified;
      }
    }

    //--------------------------------------------------------------------------
    bool FieldSpaceRequirement::operator==(
                                        const FieldSpaceRequirement &rhs) const
    //--------------------------------------------------------------------------
    {
      return (handle == rhs.handle) && 
              (privilege == rhs.privilege) && (verified == rhs.verified);
    }

    /////////////////////////////////////////////////////////////
    // StaticDependence 
    /////////////////////////////////////////////////////////////

    //--------------------------------------------------------------------------
    StaticDependence::StaticDependence(void)
      : previous_offset(0), previous_req_index(0), current_req_index(0),
        dependence_type(LEGION_NO_DEPENDENCE),validates(false),shard_only(false)
    //--------------------------------------------------------------------------
    {
    }

    //--------------------------------------------------------------------------
    StaticDependence::StaticDependence(unsigned prev, unsigned prev_req,
                           unsigned current_req, DependenceType dtype, 
                           bool val, bool shard)
      : previous_offset(prev), previous_req_index(prev_req),
        current_req_index(current_req), dependence_type(dtype), 
        validates(val), shard_only(shard)
    //--------------------------------------------------------------------------
    {
    }

    /////////////////////////////////////////////////////////////
    // TaskLauncher 
    /////////////////////////////////////////////////////////////

    //--------------------------------------------------------------------------
    TaskLauncher::TaskLauncher(void)
      : task_id(0), argument(UntypedBuffer()), predicate(Predicate::TRUE_PRED),
        map_id(0), tag(0), point(DomainPoint(0)), 
        sharding_space(IndexSpace::NO_SPACE), static_dependences(NULL),
        enable_inlining(false), local_function_task(false),
        independent_requirements(false), elide_future_return(false),
        silence_warnings(false)
    //--------------------------------------------------------------------------
    {
    }

    //--------------------------------------------------------------------------
    TaskLauncher::TaskLauncher(TaskID tid, UntypedBuffer arg,
                               Predicate pred /*= Predicate::TRUE_PRED*/,
                               MapperID mid /*=0*/, MappingTagID t /*=0*/,
                               UntypedBuffer marg /*=UntypedBuffer*/,
                               const char *prov /*=UntypedBuffer*/)
      : task_id(tid), argument(arg), predicate(pred), map_id(mid), tag(t), 
        map_arg(marg), point(DomainPoint(0)),
        sharding_space(IndexSpace::NO_SPACE), provenance(prov),
        static_dependences(NULL), enable_inlining(false),
        local_function_task(false), independent_requirements(false), 
        elide_future_return(false), silence_warnings(false)
    //--------------------------------------------------------------------------
    {
    }

    /////////////////////////////////////////////////////////////
    // IndexTaskLauncher 
    /////////////////////////////////////////////////////////////

    //--------------------------------------------------------------------------
    IndexTaskLauncher::IndexTaskLauncher(void)
      : task_id(0), launch_domain(Domain::NO_DOMAIN), 
        launch_space(IndexSpace::NO_SPACE), 
        sharding_space(IndexSpace::NO_SPACE), global_arg(UntypedBuffer()), 
        argument_map(ArgumentMap()), predicate(Predicate::TRUE_PRED), 
        concurrent(false), must_parallelism(false), map_id(0), tag(0),
        static_dependences(NULL), enable_inlining(false),
        independent_requirements(false), elide_future_return(false), 
        silence_warnings(false)
    //--------------------------------------------------------------------------
    {
    }

    //--------------------------------------------------------------------------
    IndexTaskLauncher::IndexTaskLauncher(TaskID tid, Domain dom,
                                     UntypedBuffer global,
                                     ArgumentMap map,
                                     Predicate pred /*= Predicate::TRUE_PRED*/,
                                     bool must /*=false*/, MapperID mid /*=0*/,
                                     MappingTagID t /*=0*/, UntypedBuffer marg,
                                     const char *prov)
      : task_id(tid), launch_domain(dom), launch_space(IndexSpace::NO_SPACE),
        sharding_space(IndexSpace::NO_SPACE), global_arg(global), 
        argument_map(map), predicate(pred), concurrent(false), 
        must_parallelism(must), map_id(mid), tag(t), map_arg(marg),
        provenance(prov), static_dependences(NULL), enable_inlining(false),
        independent_requirements(false), elide_future_return(false),
        silence_warnings(false)
    //--------------------------------------------------------------------------
    {
    }

    //--------------------------------------------------------------------------
    IndexTaskLauncher::IndexTaskLauncher(TaskID tid, 
                                     IndexSpace space,
                                     UntypedBuffer global,
                                     ArgumentMap map,
                                     Predicate pred /*= Predicate::TRUE_PRED*/,
                                     bool must /*=false*/, MapperID mid /*=0*/,
                                     MappingTagID t /*=0*/, UntypedBuffer marg,
                                     const char *prov)
      : task_id(tid), launch_domain(Domain::NO_DOMAIN), launch_space(space),
        sharding_space(IndexSpace::NO_SPACE), global_arg(global), 
        argument_map(map), predicate(pred), concurrent(false),
        must_parallelism(must), map_id(mid), tag(t), map_arg(marg),
        provenance(prov), static_dependences(NULL), enable_inlining(false),
        independent_requirements(false), elide_future_return(false),
        silence_warnings(false)
    //--------------------------------------------------------------------------
    {
    }

    /////////////////////////////////////////////////////////////
    // InlineLauncher 
    /////////////////////////////////////////////////////////////

    //--------------------------------------------------------------------------
    InlineLauncher::InlineLauncher(void)
      : map_id(0), tag(0), layout_constraint_id(0), static_dependences(NULL)
    //--------------------------------------------------------------------------
    {
    }

    //--------------------------------------------------------------------------
    InlineLauncher::InlineLauncher(const RegionRequirement &req,
                                   MapperID mid /*=0*/, MappingTagID t /*=0*/,
                                   LayoutConstraintID lay_id /*=0*/,
                                   UntypedBuffer marg /*= UntypedBuffer()*/,
                                   const char *prov /*= UntypedBuffer()*/)
      : requirement(req), map_id(mid), tag(t), map_arg(marg),
        layout_constraint_id(lay_id), provenance(prov), static_dependences(NULL)
    //--------------------------------------------------------------------------
    {
    }

    /////////////////////////////////////////////////////////////
    // CopyLauncher 
    /////////////////////////////////////////////////////////////

    //--------------------------------------------------------------------------
    CopyLauncher::CopyLauncher(Predicate pred /*= Predicate::TRUE_PRED*/,
                               MapperID mid /*=0*/, MappingTagID t /*=0*/,
                               UntypedBuffer marg /*=UntypedBuffer()*/,
                               const char *prov /*=NULL*/)
      : predicate(pred), map_id(mid), tag(t), map_arg(marg),
        point(DomainPoint(0)), sharding_space(IndexSpace::NO_SPACE),
        provenance(prov), static_dependences(NULL),
        possible_src_indirect_out_of_range(true),
        possible_dst_indirect_out_of_range(true),
        possible_dst_indirect_aliasing(true), silence_warnings(false)
    //--------------------------------------------------------------------------
    {
    }

    /////////////////////////////////////////////////////////////
    // IndexCopyLauncher 
    /////////////////////////////////////////////////////////////

    //--------------------------------------------------------------------------
    IndexCopyLauncher::IndexCopyLauncher(void) 
      : launch_domain(Domain::NO_DOMAIN), launch_space(IndexSpace::NO_SPACE),
        sharding_space(IndexSpace::NO_SPACE), predicate(Predicate::TRUE_PRED), 
        map_id(0), tag(0), static_dependences(NULL),
        possible_src_indirect_out_of_range(true),
        possible_dst_indirect_out_of_range(true),
        possible_dst_indirect_aliasing(true), 
        collective_src_indirect_points(true),
        collective_dst_indirect_points(true), silence_warnings(false)
    //--------------------------------------------------------------------------
    {
    }

    //--------------------------------------------------------------------------
    IndexCopyLauncher::IndexCopyLauncher(Domain dom, 
                                    Predicate pred /*= Predicate::TRUE_PRED*/,
                                    MapperID mid /*=0*/, MappingTagID t /*=0*/,
                                    UntypedBuffer marg /*=UntypedBuffer()*/,
                                    const char *prov /*=NULL*/)
      : launch_domain(dom), launch_space(IndexSpace::NO_SPACE), 
        sharding_space(IndexSpace::NO_SPACE), predicate(pred), map_id(mid),
        tag(t), map_arg(marg), provenance(prov), static_dependences(NULL),
        possible_src_indirect_out_of_range(true),
        possible_dst_indirect_out_of_range(true),
        possible_dst_indirect_aliasing(true), 
        collective_src_indirect_points(true),
        collective_dst_indirect_points(true), silence_warnings(false)
    //--------------------------------------------------------------------------
    {
    }

    //--------------------------------------------------------------------------
    IndexCopyLauncher::IndexCopyLauncher(IndexSpace space, 
                                    Predicate pred /*= Predicate::TRUE_PRED*/,
                                    MapperID mid /*=0*/, MappingTagID t /*=0*/,
                                    UntypedBuffer marg /*=UntypedBuffer()*/,
                                    const char *prov /*=NULL*/) 
      : launch_domain(Domain::NO_DOMAIN), launch_space(space), 
        sharding_space(IndexSpace::NO_SPACE), predicate(pred), map_id(mid), 
        tag(t), map_arg(marg), provenance(prov), static_dependences(NULL),
        possible_src_indirect_out_of_range(true),
        possible_dst_indirect_out_of_range(true),
        possible_dst_indirect_aliasing(true), 
        collective_src_indirect_points(true),
        collective_dst_indirect_points(true), silence_warnings(false)
    //--------------------------------------------------------------------------
    {
    }

    /////////////////////////////////////////////////////////////
    // AcquireLauncher 
    /////////////////////////////////////////////////////////////

    //--------------------------------------------------------------------------
    AcquireLauncher::AcquireLauncher(LogicalRegion reg, LogicalRegion par,
                                     PhysicalRegion phy,
                                     Predicate pred /*= Predicate::TRUE_PRED*/,
                                     MapperID id /*=0*/, MappingTagID t /*=0*/,
                                     UntypedBuffer marg /*=UntypedBuffer()*/,
                                     const char *prov /*=UntypedBuffer()*/)
      : logical_region(reg), parent_region(par), physical_region(phy), 
        predicate(pred), map_id(id), tag(t), map_arg(marg), provenance(prov),
        static_dependences(NULL), silence_warnings(false)
    //--------------------------------------------------------------------------
    {
    }

    /////////////////////////////////////////////////////////////
    // ReleaseLauncher 
    /////////////////////////////////////////////////////////////

    //--------------------------------------------------------------------------
    ReleaseLauncher::ReleaseLauncher(LogicalRegion reg, LogicalRegion par,
                                     PhysicalRegion phy,
                                     Predicate pred /*= Predicate::TRUE_PRED*/,
                                     MapperID id /*=0*/, MappingTagID t /*=0*/,
                                     UntypedBuffer marg /*=UntypedBuffer()*/,
                                     const char *prov /*=UntypedBuffer()*/)
      : logical_region(reg), parent_region(par), physical_region(phy), 
        predicate(pred), map_id(id), tag(t), map_arg(marg), provenance(prov),
        static_dependences(NULL), silence_warnings(false)
    //--------------------------------------------------------------------------
    {
    }

    /////////////////////////////////////////////////////////////
    // FillLauncher 
    /////////////////////////////////////////////////////////////

    //--------------------------------------------------------------------------
    FillLauncher::FillLauncher(void)
      : handle(LogicalRegion::NO_REGION), parent(LogicalRegion::NO_REGION),
        map_id(0), tag(0), point(DomainPoint(0)), static_dependences(NULL), 
        silence_warnings(false)
    //--------------------------------------------------------------------------
    {
    }

    //--------------------------------------------------------------------------
    FillLauncher::FillLauncher(LogicalRegion h, LogicalRegion p,
                               UntypedBuffer arg, 
                               Predicate pred /*= Predicate::TRUE_PRED*/,
                               MapperID id /*=0*/, MappingTagID t /*=0*/,
                               UntypedBuffer marg /*=UntypedBuffer()*/,
                               const char *prov /*=UntypedBuffer()*/)
      : handle(h), parent(p), argument(arg), predicate(pred), map_id(id), 
        tag(t), map_arg(marg), point(DomainPoint(0)), provenance(prov),
        static_dependences(NULL), silence_warnings(false)
    //--------------------------------------------------------------------------
    {
    }

    //--------------------------------------------------------------------------
    FillLauncher::FillLauncher(LogicalRegion h, LogicalRegion p, Future f,
                               Predicate pred /*= Predicate::TRUE_PRED*/,
                               MapperID id /*=0*/, MappingTagID t /*=0*/,
                               UntypedBuffer marg /*=UntypedBuffer()*/,
                               const char *prov /*=UntypedBuffer()*/)
      : handle(h), parent(p), future(f), predicate(pred), map_id(id), tag(t), 
        map_arg(marg), point(DomainPoint(0)), provenance(prov), 
        static_dependences(NULL), silence_warnings(false) 
    //--------------------------------------------------------------------------
    {
    }

    /////////////////////////////////////////////////////////////
    // IndexFillLauncher 
    /////////////////////////////////////////////////////////////

    //--------------------------------------------------------------------------
    IndexFillLauncher::IndexFillLauncher(void)
      : launch_domain(Domain::NO_DOMAIN), launch_space(IndexSpace::NO_SPACE),
        sharding_space(IndexSpace::NO_SPACE), region(LogicalRegion::NO_REGION),
        partition(LogicalPartition::NO_PART), projection(0), map_id(0), tag(0),
        static_dependences(NULL), silence_warnings(false) 
    //--------------------------------------------------------------------------
    {
    }

    //--------------------------------------------------------------------------
    IndexFillLauncher::IndexFillLauncher(Domain dom, LogicalRegion h, 
                               LogicalRegion p, UntypedBuffer arg, 
                               ProjectionID proj, Predicate pred,
                               MapperID id /*=0*/, MappingTagID t /*=0*/,
                               UntypedBuffer marg /*=UntypedBuffer()*/,
                               const char *prov /*=NULL*/)
      : launch_domain(dom), launch_space(IndexSpace::NO_SPACE), 
        partition(LogicalPartition::NO_PART), parent(p), projection(proj), 
        argument(arg), predicate(pred), map_id(id), tag(t), map_arg(marg),
        provenance(prov), static_dependences(NULL), silence_warnings(false)
    //--------------------------------------------------------------------------
    {
    }

    //--------------------------------------------------------------------------
    IndexFillLauncher::IndexFillLauncher(Domain dom, LogicalRegion h,
                                LogicalRegion p, Future f,
                                ProjectionID proj, Predicate pred,
                                MapperID id /*=0*/, MappingTagID t /*=0*/,
                                UntypedBuffer marg /*=UntypedBuffer()*/,
                                const char *prov /*=NULL*/)
      : launch_domain(dom), launch_space(IndexSpace::NO_SPACE), 
        sharding_space(IndexSpace::NO_SPACE), region(h), 
        partition(LogicalPartition::NO_PART), parent(p), projection(proj), 
        future(f), predicate(pred), map_id(id), tag(t), map_arg(marg),
        provenance(prov), static_dependences(NULL), silence_warnings(false)
    //--------------------------------------------------------------------------
    {
    }

    //--------------------------------------------------------------------------
    IndexFillLauncher::IndexFillLauncher(IndexSpace space, LogicalRegion h, 
                               LogicalRegion p, UntypedBuffer arg, 
                               ProjectionID proj, Predicate pred,
                               MapperID id /*=0*/, MappingTagID t /*=0*/,
                               UntypedBuffer marg /*=UntypedBuffer()*/,
                               const char *prov /*=NULL*/)
      : launch_domain(Domain::NO_DOMAIN), launch_space(space), 
        sharding_space(IndexSpace::NO_SPACE), region(h), 
        partition(LogicalPartition::NO_PART), parent(p), projection(proj), 
        argument(arg), predicate(pred), map_id(id), tag(t), map_arg(marg),
        provenance(prov), static_dependences(NULL), silence_warnings(false)
    //--------------------------------------------------------------------------
    {
    }

    //--------------------------------------------------------------------------
    IndexFillLauncher::IndexFillLauncher(IndexSpace space, LogicalRegion h,
                                LogicalRegion p, Future f,
                                ProjectionID proj, Predicate pred,
                                MapperID id /*=0*/, MappingTagID t /*=0*/,
                                UntypedBuffer marg /*=UntypedBuffer()*/,
                                const char *prov /*=NULL*/)
      : launch_domain(Domain::NO_DOMAIN), launch_space(space), 
        sharding_space(IndexSpace::NO_SPACE), region(h), 
        partition(LogicalPartition::NO_PART), parent(p), projection(proj), 
        future(f), predicate(pred), map_id(id), tag(t), map_arg(marg),
        provenance(prov), static_dependences(NULL), silence_warnings(false)
    //--------------------------------------------------------------------------
    {
    }

    //--------------------------------------------------------------------------
    IndexFillLauncher::IndexFillLauncher(Domain dom, LogicalPartition h,
                                         LogicalRegion p, UntypedBuffer arg,
                                         ProjectionID proj, Predicate pred,
                                         MapperID id /*=0*/, 
                                         MappingTagID t /*=0*/,
                                         UntypedBuffer marg/*=UntypedBuffer()*/,
                                         const char *prov /*=NULL*/)
      : launch_domain(dom), launch_space(IndexSpace::NO_SPACE), 
        sharding_space(IndexSpace::NO_SPACE), region(LogicalRegion::NO_REGION), 
        partition(h), parent(p), projection(proj),argument(arg),predicate(pred),
        map_id(id), tag(t), map_arg(marg), provenance(prov),
        static_dependences(NULL), silence_warnings(false)
    //--------------------------------------------------------------------------
    {
    }

    //--------------------------------------------------------------------------
    IndexFillLauncher::IndexFillLauncher(Domain dom, LogicalPartition h,
                                         LogicalRegion p, Future f,
                                         ProjectionID proj, Predicate pred,
                                         MapperID id /*=0*/, 
                                         MappingTagID t /*=0*/,
                                         UntypedBuffer marg/*=UntypedBuffer()*/,
                                         const char *prov/*=UntypedBuffer()*/)
      : launch_domain(dom), launch_space(IndexSpace::NO_SPACE), 
        sharding_space(IndexSpace::NO_SPACE), region(LogicalRegion::NO_REGION), 
        partition(h), parent(p), projection(proj), future(f), predicate(pred),
        map_id(id), tag(t), map_arg(marg), provenance(prov),
        static_dependences(NULL), silence_warnings(false)
    //--------------------------------------------------------------------------
    {
    }

    //--------------------------------------------------------------------------
    IndexFillLauncher::IndexFillLauncher(IndexSpace space, LogicalPartition h,
                                         LogicalRegion p, UntypedBuffer arg,
                                         ProjectionID proj, Predicate pred,
                                         MapperID id /*=0*/, 
                                         MappingTagID t /*=0*/,
                                         UntypedBuffer marg/*=UntypedBuffer()*/,
                                         const char *prov /*=NULL*/)
      : launch_domain(Domain::NO_DOMAIN), launch_space(space), 
        sharding_space(IndexSpace::NO_SPACE), region(LogicalRegion::NO_REGION), 
        partition(h), parent(p), projection(proj),argument(arg),predicate(pred),
        map_id(id), tag(t), map_arg(marg), provenance(prov),
        static_dependences(NULL), silence_warnings(false)
    //--------------------------------------------------------------------------
    {
    }

    //--------------------------------------------------------------------------
    IndexFillLauncher::IndexFillLauncher(IndexSpace space, LogicalPartition h,
                                         LogicalRegion p, Future f,
                                         ProjectionID proj, Predicate pred,
                                         MapperID id /*=0*/, 
                                         MappingTagID t /*=0*/,
                                         UntypedBuffer marg/*=UntypedBuffer()*/,
                                         const char *prov /*=NULL*/)
      : launch_domain(Domain::NO_DOMAIN), launch_space(space), 
        sharding_space(IndexSpace::NO_SPACE), region(LogicalRegion::NO_REGION),
        partition(h), parent(p), projection(proj), future(f), predicate(pred),
        map_id(id), tag(t), map_arg(marg), provenance(prov),
        static_dependences(NULL), silence_warnings(false)
    //--------------------------------------------------------------------------
    {
    }

    /////////////////////////////////////////////////////////////
    // DiscardLauncher 
    /////////////////////////////////////////////////////////////

    //--------------------------------------------------------------------------
    DiscardLauncher::DiscardLauncher(LogicalRegion h, LogicalRegion p)
      : handle(h), parent(p)
    //--------------------------------------------------------------------------
    {
    }

    LEGION_DISABLE_DEPRECATED_WARNINGS

    /////////////////////////////////////////////////////////////
    // AttachLauncher
    /////////////////////////////////////////////////////////////

    //--------------------------------------------------------------------------
    AttachLauncher::AttachLauncher(ExternalResource r, 
                                   LogicalRegion h, LogicalRegion p,
                                   const bool restr/*= true*/,
                                   const bool map/*= true*/)
      : resource(r), parent(p), handle(h), external_resource(NULL),
        restricted(restr), mapped(map),
        collective((r == LEGION_EXTERNAL_INSTANCE) ? true : false),
        deduplicate_across_shards(false), file_name(NULL),
        mode(LEGION_FILE_READ_ONLY), footprint(0), static_dependences(NULL)
    //--------------------------------------------------------------------------
    {
    }

    //--------------------------------------------------------------------------
    AttachLauncher::~AttachLauncher(void)
    //--------------------------------------------------------------------------
    {
    }

    /////////////////////////////////////////////////////////////
    // IndexAttachLauncher
    /////////////////////////////////////////////////////////////

    //--------------------------------------------------------------------------
    IndexAttachLauncher::IndexAttachLauncher(ExternalResource r,
                                             LogicalRegion p, const bool restr)
      : resource(r), parent(p), restricted(restr),
        deduplicate_across_shards(false), mode(LEGION_FILE_READ_ONLY),
        static_dependences(NULL)
    //--------------------------------------------------------------------------
    {
    }

    //--------------------------------------------------------------------------
    IndexAttachLauncher::~IndexAttachLauncher(void)
    //--------------------------------------------------------------------------
    {
    }

    LEGION_REENABLE_DEPRECATED_WARNINGS

    /////////////////////////////////////////////////////////////
    // PredicateLauncher
    /////////////////////////////////////////////////////////////


    //--------------------------------------------------------------------------
    PredicateLauncher::PredicateLauncher(bool and_)
      : and_op(and_)
    //--------------------------------------------------------------------------
    {
    }

    /////////////////////////////////////////////////////////////
    // TimingLauncher
    /////////////////////////////////////////////////////////////

    //--------------------------------------------------------------------------
    TimingLauncher::TimingLauncher(TimingMeasurement m)
      : measurement(m)
    //--------------------------------------------------------------------------
    {
    }

    /////////////////////////////////////////////////////////////
    // TunableLauncher
    /////////////////////////////////////////////////////////////

    //--------------------------------------------------------------------------
    TunableLauncher::TunableLauncher(TunableID tid, MapperID m, MappingTagID t,
                                     size_t return_size)
      : tunable(tid), mapper(m), tag(t), return_type_size(return_size)
    //--------------------------------------------------------------------------
    {
    }

    /////////////////////////////////////////////////////////////
    // MustEpochLauncher 
    /////////////////////////////////////////////////////////////

    //--------------------------------------------------------------------------
    MustEpochLauncher::MustEpochLauncher(MapperID id /*= 0*/,   
                                         MappingTagID tag/*= 0*/)
      : map_id(id), mapping_tag(tag), launch_domain(Domain::NO_DOMAIN),
        launch_space(IndexSpace::NO_SPACE), 
        sharding_space(IndexSpace::NO_SPACE), silence_warnings(false)
    //--------------------------------------------------------------------------
    {
    }

    /////////////////////////////////////////////////////////////
    // LayoutConstraintRegistrar
    /////////////////////////////////////////////////////////////

    //--------------------------------------------------------------------------
    LayoutConstraintRegistrar::LayoutConstraintRegistrar(void)
      : handle(FieldSpace::NO_SPACE), layout_name(NULL)
    //--------------------------------------------------------------------------
    {
    }

    //--------------------------------------------------------------------------
    LayoutConstraintRegistrar::LayoutConstraintRegistrar(FieldSpace h,
                                                  const char *layout/*= NULL*/)
      : handle(h), layout_name(layout)
    //--------------------------------------------------------------------------
    {
    }

    /////////////////////////////////////////////////////////////
    // TaskVariantRegistrar 
    /////////////////////////////////////////////////////////////

    //--------------------------------------------------------------------------
    TaskVariantRegistrar::TaskVariantRegistrar(void)
      : task_id(0), global_registration(true), 
        task_variant_name(NULL), leaf_variant(false), 
        inner_variant(false), idempotent_variant(false),
        replicable_variant(false), concurrent_variant(false)
    //--------------------------------------------------------------------------
    {
    }

    //--------------------------------------------------------------------------
    TaskVariantRegistrar::TaskVariantRegistrar(TaskID task_id, bool global,
                                               const char *variant_name)
      : task_id(task_id), global_registration(global), 
        task_variant_name(variant_name), leaf_variant(false), 
        inner_variant(false), idempotent_variant(false),
        replicable_variant(false), concurrent_variant(false)
    //--------------------------------------------------------------------------
    {
    }

    //--------------------------------------------------------------------------
    TaskVariantRegistrar::TaskVariantRegistrar(TaskID task_id,
					       const char *variant_name,
					       bool global/*=true*/)
      : task_id(task_id), global_registration(global), 
        task_variant_name(variant_name), leaf_variant(false), 
        inner_variant(false), idempotent_variant(false),
        replicable_variant(false), concurrent_variant(false)
    //--------------------------------------------------------------------------
    {
    }

    /////////////////////////////////////////////////////////////
    // LegionHandshake 
    /////////////////////////////////////////////////////////////

    //--------------------------------------------------------------------------
    LegionHandshake::LegionHandshake(void)
      : impl(NULL)
    //--------------------------------------------------------------------------
    {
    }

    //--------------------------------------------------------------------------
    LegionHandshake::LegionHandshake(const LegionHandshake &rhs)
      : impl(rhs.impl)
    //--------------------------------------------------------------------------
    {
      if (impl != NULL)
        impl->add_reference();
    }

    //--------------------------------------------------------------------------
    LegionHandshake::~LegionHandshake(void)
    //--------------------------------------------------------------------------
    {
      if (impl != NULL)
      {
        if (impl->remove_reference())
          delete impl;
        impl = NULL;
      }
    }

    //--------------------------------------------------------------------------
    LegionHandshake::LegionHandshake(Internal::LegionHandshakeImpl *i)
      : impl(i)
    //--------------------------------------------------------------------------
    {
      if (impl != NULL)
        impl->add_reference();
    }

    //--------------------------------------------------------------------------
    LegionHandshake& LegionHandshake::operator=(const LegionHandshake &rhs)
    //--------------------------------------------------------------------------
    {
      if (impl != NULL)
      {
        if (impl->remove_reference())
          delete impl;
      }
      impl = rhs.impl;
      if (impl != NULL)
        impl->add_reference();
      return *this;
    }

    //--------------------------------------------------------------------------
    void LegionHandshake::ext_handoff_to_legion(void) const
    //--------------------------------------------------------------------------
    {
#ifdef DEBUG_LEGION
      assert(impl != NULL);
#endif
      impl->ext_handoff_to_legion();
    }

    //--------------------------------------------------------------------------
    void LegionHandshake::ext_wait_on_legion(void) const
    //--------------------------------------------------------------------------
    {
#ifdef DEBUG_LEGION
      assert(impl != NULL);
#endif
      impl->ext_wait_on_legion();
    }

    //--------------------------------------------------------------------------
    void LegionHandshake::legion_handoff_to_ext(void) const
    //--------------------------------------------------------------------------
    {
#ifdef DEBUG_LEGION
      assert(impl != NULL);
#endif
      impl->legion_handoff_to_ext();
    }

    //--------------------------------------------------------------------------
    void LegionHandshake::legion_wait_on_ext(void) const
    //--------------------------------------------------------------------------
    {
#ifdef DEBUG_LEGION
      assert(impl != NULL);
#endif
      impl->legion_wait_on_ext();
    }

    //--------------------------------------------------------------------------
    PhaseBarrier LegionHandshake::get_legion_wait_phase_barrier(void) const
    //--------------------------------------------------------------------------
    {
#ifdef DEBUG_LEGION
      assert(impl != NULL);
#endif
      return impl->get_legion_wait_phase_barrier();
    }

    //--------------------------------------------------------------------------
    PhaseBarrier LegionHandshake::get_legion_arrive_phase_barrier(void) const
    //--------------------------------------------------------------------------
    {
#ifdef DEBUG_LEGION
      assert(impl != NULL);
#endif
      return impl->get_legion_arrive_phase_barrier();
    }
    
    //--------------------------------------------------------------------------
    void LegionHandshake::advance_legion_handshake(void) const
    //--------------------------------------------------------------------------
    {
#ifdef DEBUG_LEGION
      assert(impl != NULL);
#endif
      impl->advance_legion_handshake();
    }

    /////////////////////////////////////////////////////////////
    // MPILegionHandshake 
    /////////////////////////////////////////////////////////////

    //--------------------------------------------------------------------------
    MPILegionHandshake::MPILegionHandshake(void)
      : LegionHandshake()
    //--------------------------------------------------------------------------
    {
    }

    //--------------------------------------------------------------------------
    MPILegionHandshake::MPILegionHandshake(const MPILegionHandshake &rhs)
      : LegionHandshake(rhs)
    //--------------------------------------------------------------------------
    {
    }

    //--------------------------------------------------------------------------
    MPILegionHandshake::~MPILegionHandshake(void)
    //--------------------------------------------------------------------------
    {
    }

    //--------------------------------------------------------------------------
    MPILegionHandshake::MPILegionHandshake(Internal::LegionHandshakeImpl *i)
      : LegionHandshake(i)
    //--------------------------------------------------------------------------
    {
    }

    //--------------------------------------------------------------------------
    MPILegionHandshake& MPILegionHandshake::operator=(
                                                  const MPILegionHandshake &rhs)
    //--------------------------------------------------------------------------
    {
      if (impl != NULL)
      {
        if (impl->remove_reference())
          delete impl;
      }
      impl = rhs.impl;
      if (impl != NULL)
        impl->add_reference();
      return *this;
    }

    /////////////////////////////////////////////////////////////
    // Future 
    /////////////////////////////////////////////////////////////

    //--------------------------------------------------------------------------
    Future::Future(void)
      : impl(NULL)
    //--------------------------------------------------------------------------
    {
    }

    //--------------------------------------------------------------------------
    Future::Future(const Future &rhs)
      : impl(rhs.impl)
    //--------------------------------------------------------------------------
    {
      if (impl != NULL)
        impl->add_base_gc_ref(Internal::APPLICATION_REF);
    }

    //--------------------------------------------------------------------------
    Future::Future(Future &&rhs) noexcept
      : impl(rhs.impl)
    //--------------------------------------------------------------------------
    {
      rhs.impl = NULL;
    }

    //--------------------------------------------------------------------------
    Future::~Future(void)
    //--------------------------------------------------------------------------
    {
      if ((impl != NULL) && impl->remove_base_gc_ref(Internal::APPLICATION_REF))
        delete impl;
    }

    //--------------------------------------------------------------------------
    Future::Future(Internal::FutureImpl *i)
      : impl(i)
    //--------------------------------------------------------------------------
    {
      if (impl != NULL)
        impl->add_base_gc_ref(Internal::APPLICATION_REF);
    }

    //--------------------------------------------------------------------------
    Future& Future::operator=(const Future &rhs)
    //--------------------------------------------------------------------------
    {
      if ((impl != NULL) && impl->remove_base_gc_ref(Internal::APPLICATION_REF))
        delete impl;
      impl = rhs.impl;
      if (impl != NULL)
        impl->add_base_gc_ref(Internal::APPLICATION_REF);
      return *this;
    }

    //--------------------------------------------------------------------------
    Future& Future::operator=(Future &&rhs) noexcept
    //--------------------------------------------------------------------------
    {
      if ((impl != NULL) && impl->remove_base_gc_ref(Internal::APPLICATION_REF))
        delete impl;
      impl = rhs.impl;
      rhs.impl = NULL;
      return *this;
    }

    //--------------------------------------------------------------------------
    void Future::get_void_result(bool silence_warnings,
                                 const char *warning_string) const
    //--------------------------------------------------------------------------
    {
      if (impl != NULL)
        impl->wait(silence_warnings, warning_string);
    }

    //--------------------------------------------------------------------------
    bool Future::is_empty(bool block /*= true*/, 
                          bool silence_warnings/*=false*/,
                          const char *warning_string /*=NULL*/) const
    //--------------------------------------------------------------------------
    {
      if (impl != NULL)
        return impl->is_empty(block, silence_warnings, warning_string);
      return true;
    }

    //--------------------------------------------------------------------------
    bool Future::is_ready(bool subscribe) const
    //--------------------------------------------------------------------------
    {
      if (impl != NULL)
      {
        if (subscribe)
          impl->subscribe();
        const Internal::ApEvent ready = impl->get_ready_event();
        // Always subscribe to the Realm event to know when it triggers
        ready.subscribe();
        bool poisoned = false;
        if (ready.has_triggered_faultaware(poisoned))
          return true;
        if (poisoned && (Internal::implicit_context != NULL))
          Internal::implicit_context->raise_poison_exception();
        return false;
      }
      return true; // Empty futures are always ready
    }

    //--------------------------------------------------------------------------
    const void* Future::get_buffer(Memory::Kind memory, size_t *extent_in_bytes,
       bool check_size, bool silence_warnings, const char *warning_string) const
    //--------------------------------------------------------------------------
    {
      if (impl == NULL)
        REPORT_LEGION_ERROR(ERROR_REQUEST_FOR_EMPTY_FUTURE, 
                          "Illegal request for future value from empty future")
      if (Internal::implicit_context == NULL)
        return impl->get_buffer(Processor::NO_PROC, memory, extent_in_bytes, 
                                check_size, silence_warnings, warning_string);
      else
        return impl->get_buffer(
            Internal::implicit_context->get_executing_processor(), memory,
            extent_in_bytes, check_size, silence_warnings, warning_string);
    }

    //--------------------------------------------------------------------------
    size_t Future::get_untyped_size(void) const
    //--------------------------------------------------------------------------
    {
      if (impl == NULL)
        REPORT_LEGION_ERROR(ERROR_REQUEST_FOR_EMPTY_FUTURE, 
                          "Illegal request for future size from empty future");
      return impl->get_untyped_size();
    }

    //--------------------------------------------------------------------------
    const void* Future::get_metadata(size_t *size) const
    //--------------------------------------------------------------------------
    {
      if (impl == NULL)
        REPORT_LEGION_ERROR(ERROR_REQUEST_FOR_EMPTY_FUTURE, 
                          "Illegal request for metadata from empty future");
      return impl->get_metadata(size);
    }

    //--------------------------------------------------------------------------
    Realm::RegionInstance Future::get_instance(Memory::Kind memkind,
                        size_t field_size, bool check_field_size,
                        const char *warning_string, bool silence_warnings) const
    //--------------------------------------------------------------------------
    {
      if (impl == NULL)
        REPORT_LEGION_ERROR(ERROR_REQUEST_FOR_EMPTY_FUTURE, 
                          "Illegal request for accessor on an empty future");
      return impl->get_instance(memkind, field_size, check_field_size,
                                silence_warnings, warning_string);
    }

    //--------------------------------------------------------------------------
    void Future::report_incompatible_accessor(const char *accessor_kind,
                                           Realm::RegionInstance instance) const
    //--------------------------------------------------------------------------
    {
#ifdef DEBUG_LEGION
      assert(impl != NULL);
#endif
      impl->report_incompatible_accessor(accessor_kind, instance);
    }

    //--------------------------------------------------------------------------
    /*static*/ Future Future::from_untyped_pointer(Runtime *rt,
                               const void *value, size_t value_size, bool owned)
    //--------------------------------------------------------------------------
    {
      if (Internal::implicit_context == NULL)
        REPORT_LEGION_ERROR(ERROR_CONFUSED_USER,
            "Creating Legion Future objects from a buffer is only permitted "
            "to be performed inside of Legion tasks.")
      return Internal::implicit_context->from_value(value, value_size,
          owned, NULL/*provenance*/, false/*shard local*/);
    }

    //--------------------------------------------------------------------------
    /*static*/ Future Future::from_untyped_pointer(
             const void *value, size_t value_size, bool owned, 
             const char *prov, bool shard_local)
    //--------------------------------------------------------------------------
    {
      if (Internal::implicit_context == NULL)
        REPORT_LEGION_ERROR(ERROR_CONFUSED_USER,
            "Creating Legion Future objects from a buffer is only permitted "
            "to be performed inside of Legion tasks.")
      Internal::AutoProvenance provenance(prov);
      return Internal::implicit_context->from_value(value, value_size,
                                        owned, provenance, shard_local);
    }

    //--------------------------------------------------------------------------
    /*static*/ Future Future::from_value(const void *buffer, size_t size,
        bool owned, const Realm::ExternalInstanceResource &resource,
        void (*freefunc)(const Realm::ExternalInstanceResource&),
        const char *prov, bool shard_local)
    //--------------------------------------------------------------------------
    {
      if (Internal::implicit_context == NULL)
        REPORT_LEGION_ERROR(ERROR_CONFUSED_USER,
            "Creating Legion Future objects from a buffer is only permitted "
            "to be performed inside of Legion tasks.")
      Internal::AutoProvenance provenance(prov);
      return Internal::implicit_context->from_value(buffer, size, owned,
                            resource, freefunc, provenance, shard_local);
    }

    /////////////////////////////////////////////////////////////
    // Future Map 
    /////////////////////////////////////////////////////////////

    //--------------------------------------------------------------------------
    FutureMap::FutureMap(void)
      : impl(NULL)
    //--------------------------------------------------------------------------
    {
    }

    //--------------------------------------------------------------------------
    FutureMap::FutureMap(const FutureMap &map)
      : impl(map.impl)
    //--------------------------------------------------------------------------
    {
      if (impl != NULL)
        impl->add_base_gc_ref(Internal::APPLICATION_REF);
    }

    //--------------------------------------------------------------------------
    FutureMap::FutureMap(FutureMap &&map) noexcept
      : impl(map.impl)
    //--------------------------------------------------------------------------
    {
      map.impl = NULL;
    }

    //--------------------------------------------------------------------------
    FutureMap::FutureMap(Internal::FutureMapImpl *i)
      : impl(i)
    //--------------------------------------------------------------------------
    {
      if (impl != NULL)
        impl->add_base_gc_ref(Internal::APPLICATION_REF);
    }

    //--------------------------------------------------------------------------
    FutureMap::~FutureMap(void)
    //--------------------------------------------------------------------------
    {
      if ((impl != NULL) && impl->remove_base_gc_ref(Internal::APPLICATION_REF))
        delete impl;
    }

    //--------------------------------------------------------------------------
    FutureMap& FutureMap::operator=(const FutureMap &rhs)
    //--------------------------------------------------------------------------
    {
      if ((impl != NULL) && impl->remove_base_gc_ref(Internal::APPLICATION_REF))
        delete impl;
      impl = rhs.impl;
      if (impl != NULL)
        impl->add_base_gc_ref(Internal::APPLICATION_REF);
      return *this;
    }

    //--------------------------------------------------------------------------
    FutureMap& FutureMap::operator=(FutureMap &&rhs) noexcept
    //--------------------------------------------------------------------------
    {
      if ((impl != NULL) && impl->remove_base_gc_ref(Internal::APPLICATION_REF))
        delete impl;
      impl = rhs.impl;
      rhs.impl = NULL;
      return *this;
    }

    //--------------------------------------------------------------------------
    Future FutureMap::get_future(const DomainPoint &point) const
    //--------------------------------------------------------------------------
    {
#ifdef DEBUG_LEGION
      assert(impl != NULL);
#endif
      return impl->get_future(point, false/*internal*/);
    }

    //--------------------------------------------------------------------------
    void FutureMap::get_void_result(const DomainPoint &point, 
                                    bool silence_warnings,
                                    const char *warning_string) const
    //--------------------------------------------------------------------------
    {
      if (impl != NULL)
        impl->get_void_result(point, silence_warnings, warning_string);
    }

    //--------------------------------------------------------------------------
    void FutureMap::wait_all_results(bool silence_warnings,
                                     const char *warning_string) const
    //--------------------------------------------------------------------------
    {
      if (impl != NULL)
        impl->wait_all_results(silence_warnings, warning_string);
    }

    //--------------------------------------------------------------------------
    Domain FutureMap::get_future_map_domain(void) const
    //--------------------------------------------------------------------------
    {
      if (impl == NULL)
        return Domain::NO_DOMAIN;
      else
        return impl->get_domain();
    }

    /////////////////////////////////////////////////////////////
    // Physical Region 
    /////////////////////////////////////////////////////////////

    //--------------------------------------------------------------------------
    PhysicalRegion::PhysicalRegion(void)
      : impl(NULL)
    //--------------------------------------------------------------------------
    {
    }

    //--------------------------------------------------------------------------
    PhysicalRegion::PhysicalRegion(const PhysicalRegion &rhs)
      : impl(rhs.impl)
    //--------------------------------------------------------------------------
    {
      if (impl != NULL)
        impl->add_reference();
    }

    //--------------------------------------------------------------------------
    PhysicalRegion::PhysicalRegion(PhysicalRegion &&rhs) noexcept
      : impl(rhs.impl)
    //--------------------------------------------------------------------------
    {
      rhs.impl = NULL;
    }

    //--------------------------------------------------------------------------
    PhysicalRegion::PhysicalRegion(Internal::PhysicalRegionImpl *i)
      : impl(i)
    //--------------------------------------------------------------------------
    {
      if (impl != NULL)
        impl->add_reference();
    }

    //--------------------------------------------------------------------------
    PhysicalRegion::~PhysicalRegion(void)
    //--------------------------------------------------------------------------
    {
      if (impl != NULL)
      {
        if (impl->remove_reference())
          delete impl;
        impl = NULL;
      }
    }

    //--------------------------------------------------------------------------
    PhysicalRegion& PhysicalRegion::operator=(const PhysicalRegion &rhs)
    //--------------------------------------------------------------------------
    {
      if (impl != NULL)
      {
        if (impl->remove_reference())
          delete impl;
      }
      impl = rhs.impl;
      if (impl != NULL)
        impl->add_reference();
      return *this;
    }

    //--------------------------------------------------------------------------
    PhysicalRegion& PhysicalRegion::operator=(PhysicalRegion &&rhs) noexcept
    //--------------------------------------------------------------------------
    {
      if ((impl != NULL) && impl->remove_reference())
        delete impl;
      impl = rhs.impl;
      rhs.impl = NULL;
      return *this;
    }

    //--------------------------------------------------------------------------
    bool PhysicalRegion::is_mapped(void) const
    //--------------------------------------------------------------------------
    {
      if (impl == NULL)
        return false;
      return impl->is_mapped();
    }

    //--------------------------------------------------------------------------
    void PhysicalRegion::wait_until_valid(bool silence_warnings,
                                          const char *warning_string)
    //--------------------------------------------------------------------------
    {
#ifdef DEBUG_LEGION
      assert(impl != NULL);
#endif
      impl->wait_until_valid(silence_warnings, warning_string);
    }

    //--------------------------------------------------------------------------
    bool PhysicalRegion::is_valid(void) const
    //--------------------------------------------------------------------------
    {
      if (impl != NULL)
        return impl->is_valid();
      else
        return false;
    }

    //--------------------------------------------------------------------------
    LogicalRegion PhysicalRegion::get_logical_region(void) const
    //--------------------------------------------------------------------------
    {
      if (impl != NULL)
        return impl->get_logical_region();
      else
        return LogicalRegion::NO_REGION;
    }

    //--------------------------------------------------------------------------
    PrivilegeMode PhysicalRegion::get_privilege(void) const
    //--------------------------------------------------------------------------
    {
      if (impl != NULL)
        return impl->get_privilege();
      else
        return LEGION_NO_ACCESS;
    }

    //--------------------------------------------------------------------------
    LegionRuntime::Accessor::RegionAccessor<
      LegionRuntime::Accessor::AccessorType::Generic>
        PhysicalRegion::get_accessor(bool silence_warnings) const
    //--------------------------------------------------------------------------
    {
#ifdef DEBUG_LEGION
      assert(impl != NULL);
#endif
      return impl->get_accessor(silence_warnings);
    }

    //--------------------------------------------------------------------------
    LegionRuntime::Accessor::RegionAccessor<
      LegionRuntime::Accessor::AccessorType::Generic>
        PhysicalRegion::get_field_accessor(FieldID fid, 
                                           bool silence_warnings) const
    //--------------------------------------------------------------------------
    {
#ifdef DEBUG_LEGION
      assert(impl != NULL);
#endif
      return impl->get_field_accessor(fid, silence_warnings);
    }

    //--------------------------------------------------------------------------
    void PhysicalRegion::get_memories(std::set<Memory>& memories,
                        bool silence_warnings, const char *warning_string) const
    //--------------------------------------------------------------------------
    {
      impl->get_memories(memories, silence_warnings, warning_string);
    }

    //--------------------------------------------------------------------------
    void PhysicalRegion::get_fields(std::vector<FieldID>& fields) const
    //--------------------------------------------------------------------------
    {
      impl->get_fields(fields);
    }

    //--------------------------------------------------------------------------
    void PhysicalRegion::get_bounds(void *realm_is, TypeTag type_tag) const 
    //--------------------------------------------------------------------------
    {
      impl->get_bounds(realm_is, type_tag);
    }

    //--------------------------------------------------------------------------
    Realm::RegionInstance PhysicalRegion::get_instance_info(PrivilegeMode mode,
                              FieldID fid, size_t field_size, void *realm_is, 
                              TypeTag type_tag, const char *warning_string,
                              bool silence_warnings, bool generic_accessor, 
                              bool check_field_size, ReductionOpID redop) const
    //--------------------------------------------------------------------------
    {
      if (impl == NULL)
        REPORT_LEGION_ERROR(ERROR_PHYSICAL_REGION_UNMAPPED,
            "Illegal request to create an accessor for uninitialized physical "
            "region in task %s (UID %lld)",
            Internal::implicit_context->get_task_name(),
            Internal::implicit_context->get_unique_id())
      return impl->get_instance_info(mode, fid, field_size, realm_is, type_tag, 
                                     warning_string, silence_warnings, 
                                     generic_accessor, check_field_size, redop);
    }

    //--------------------------------------------------------------------------
    Realm::RegionInstance PhysicalRegion::get_padding_info(FieldID fid,
                            size_t field_size, Domain *inner, Domain &outer, 
                            const char *warning_string, bool silence_warnings,
                            bool generic_accessor, bool check_field_size) const
    //--------------------------------------------------------------------------
    {
      if (impl == NULL)
        REPORT_LEGION_ERROR(ERROR_PHYSICAL_REGION_UNMAPPED,
            "Illegal request to create a padding accessor for uninitialized "
            "physical region in task %s (UID %lld)",
            Internal::implicit_context->get_task_name(),
            Internal::implicit_context->get_unique_id())
      return impl->get_padding_info(fid, field_size, inner, outer,
          warning_string, silence_warnings, generic_accessor, check_field_size);
    }

    //--------------------------------------------------------------------------
    void PhysicalRegion::report_incompatible_accessor(const char *accessor_kind,
                              Realm::RegionInstance instance, FieldID fid) const
    //--------------------------------------------------------------------------
    {
      impl->report_incompatible_accessor(accessor_kind, instance, fid);
    }

    //--------------------------------------------------------------------------
    void PhysicalRegion::report_incompatible_multi_accessor(unsigned index,
    FieldID fid, Realm::RegionInstance inst1, Realm::RegionInstance inst2) const
    //--------------------------------------------------------------------------
    {
      impl->report_incompatible_multi_accessor(index, fid, inst1, inst2);
    }

    //--------------------------------------------------------------------------
    void PhysicalRegion::report_colocation_violation(const char *accessor_kind,
                 FieldID fid, Realm::RegionInstance inst1, 
                 Realm::RegionInstance inst2, 
                 const PhysicalRegion &other, bool reduction) const
    //--------------------------------------------------------------------------
    {
      impl->report_colocation_violation(accessor_kind, fid, inst1, inst2,
                                        other, reduction);
    }

    //--------------------------------------------------------------------------
    /*static*/ void PhysicalRegion::empty_colocation_regions(
                         const char *accessor_kind, FieldID fid, bool reduction)
    //--------------------------------------------------------------------------
    {
      Internal::PhysicalRegionImpl::empty_colocation_regions(accessor_kind,
                                                             fid, reduction);
    }

    //--------------------------------------------------------------------------
    /*static*/ void PhysicalRegion::fail_bounds_check(DomainPoint p,FieldID fid,
                                                 PrivilegeMode mode, bool multi)
    //--------------------------------------------------------------------------
    {
      Internal::PhysicalRegionImpl::fail_bounds_check(p, fid, mode, multi);
    }

    //--------------------------------------------------------------------------
    /*static*/ void PhysicalRegion::fail_bounds_check(Domain d, FieldID fid,
                                                 PrivilegeMode mode, bool multi)
    //--------------------------------------------------------------------------
    {
      Internal::PhysicalRegionImpl::fail_bounds_check(d, fid, mode, multi);
    }

    //--------------------------------------------------------------------------
    /*static*/ void PhysicalRegion::fail_privilege_check(DomainPoint p, 
                                                FieldID fid, PrivilegeMode mode)
    //--------------------------------------------------------------------------
    {
      Internal::PhysicalRegionImpl::fail_privilege_check(p, fid, mode);
    }

    //--------------------------------------------------------------------------
    /*static*/ void PhysicalRegion::fail_privilege_check(Domain d, FieldID fid, 
                                                         PrivilegeMode mode)
    //--------------------------------------------------------------------------
    {
      Internal::PhysicalRegionImpl::fail_privilege_check(d, fid, mode);
    }

    //--------------------------------------------------------------------------
    /*static*/ void PhysicalRegion::fail_padding_check(DomainPoint p, 
                                                       FieldID fid)
    //--------------------------------------------------------------------------
    {
      Internal::PhysicalRegionImpl::fail_padding_check(p, fid);
    }

    /////////////////////////////////////////////////////////////
    // UntypedDeferredValue
    /////////////////////////////////////////////////////////////

    //--------------------------------------------------------------------------
    UntypedDeferredValue::UntypedDeferredValue(void)
      : instance(Realm::RegionInstance::NO_INST), field_size(0)
    //--------------------------------------------------------------------------
    {
    }

    //--------------------------------------------------------------------------
    UntypedDeferredValue::UntypedDeferredValue(size_t fs, Memory memory,
                                    const void *initial_value, size_t alignment) 
      : field_size(fs)
    //--------------------------------------------------------------------------
    {
      const Realm::Point<1,coord_t> zero(0);
      Realm::IndexSpace<1,coord_t> bounds = Realm::Rect<1,coord_t>(zero, zero);
      const std::vector<size_t> field_sizes(1, field_size);
      Realm::InstanceLayoutConstraints constraints(field_sizes, 0/*blocking*/);
      int dim_order[1];
      dim_order[0] = 0;
      Realm::InstanceLayoutGeneric *layout = 
        Realm::InstanceLayoutGeneric::choose_instance_layout(bounds, 
            constraints, dim_order);
      layout->alignment_reqd = alignment;
      Runtime *runtime = Runtime::get_runtime();
      instance = runtime->create_task_local_instance(memory, layout);
      if (initial_value != NULL)
      {
        Realm::ProfilingRequestSet no_requests; 
        std::vector<Realm::CopySrcDstField> dsts(1);
        dsts[0].set_field(instance, 0/*field id*/, field_size);
        const Internal::LgEvent wait_on(
            bounds.fill(dsts, no_requests, initial_value, field_size));
        if (wait_on.exists())
          wait_on.wait();
      }
    }

    //--------------------------------------------------------------------------
    UntypedDeferredValue::UntypedDeferredValue(size_t fs, Memory::Kind memkind,
                                    const void *initial_value, size_t alignment) 
      : field_size(fs)
    //--------------------------------------------------------------------------
    {
      Machine machine = Realm::Machine::get_machine();
      Machine::MemoryQuery finder(machine);
      const Processor exec_proc = Processor::get_executing_processor();
      finder.best_affinity_to(exec_proc);
      finder.only_kind(memkind);
      if (finder.count() == 0)
      {
        finder = Machine::MemoryQuery(machine);
        finder.has_affinity_to(exec_proc);
        finder.only_kind(memkind);
      }
      if (finder.count() == 0)
      {
        const char *mem_names[] = {
#define MEM_NAMES(name, desc) desc,
          REALM_MEMORY_KINDS(MEM_NAMES) 
#undef MEM_NAMES
        };
        const char *proc_names[] = {
#define PROC_NAMES(name, desc) desc,
          REALM_PROCESSOR_KINDS(PROC_NAMES)
#undef PROC_NAMES
        };
        Context ctx = Runtime::get_context();
        REPORT_LEGION_ERROR(ERROR_DEFERRED_ALLOCATION_FAILURE,
            "Unable to find associated %s memory for %s processor when "
            "performing an UntypedDeferredValue creation in task %s (UID %lld)",
            mem_names[memkind], proc_names[exec_proc.kind()],
            ctx->get_task_name(), ctx->get_unique_id());
      }
      const Memory memory = finder.first();
      const Realm::Point<1,coord_t> zero(0);
      Realm::IndexSpace<1,coord_t> bounds = Realm::Rect<1,coord_t>(zero, zero);
      const std::vector<size_t> field_sizes(1, field_size);
      Realm::InstanceLayoutConstraints constraints(field_sizes, 0/*blocking*/);
      int dim_order[1];
      dim_order[0] = 0;
      Realm::InstanceLayoutGeneric *layout = 
        Realm::InstanceLayoutGeneric::choose_instance_layout(bounds, 
            constraints, dim_order);
      layout->alignment_reqd = alignment;
      Runtime *runtime = Runtime::get_runtime();
      instance = runtime->create_task_local_instance(memory, layout);
      if (initial_value != NULL)
      {
        Realm::ProfilingRequestSet no_requests; 
        std::vector<Realm::CopySrcDstField> dsts(1);
        dsts[0].set_field(instance, 0/*field id*/, field_size);
        const Internal::LgEvent wait_on(
            bounds.fill(dsts, no_requests, initial_value, field_size));
        if (wait_on.exists())
          wait_on.wait();
      }
    }

    //--------------------------------------------------------------------------
    void UntypedDeferredValue::finalize(Context ctx) const
    //--------------------------------------------------------------------------
    {
      Runtime::legion_task_postamble(ctx,instance.pointer_untyped(0,field_size),
                                     field_size, true/*owner*/, instance);
    }

    //--------------------------------------------------------------------------
    Realm::RegionInstance UntypedDeferredValue::get_instance() const
    //--------------------------------------------------------------------------
    {
      return instance;
    }

    /////////////////////////////////////////////////////////////
    // Output Region
    /////////////////////////////////////////////////////////////

    //--------------------------------------------------------------------------
    OutputRegion::OutputRegion(void)
      : impl(NULL)
    //--------------------------------------------------------------------------
    {
    }

    //--------------------------------------------------------------------------
    OutputRegion::OutputRegion(const OutputRegion &rhs)
      : impl(rhs.impl)
    //--------------------------------------------------------------------------
    {
      if (impl != NULL)
        impl->add_reference();
    }

    //--------------------------------------------------------------------------
    OutputRegion::OutputRegion(Internal::OutputRegionImpl *i)
      : impl(i)
    //--------------------------------------------------------------------------
    {
      if (impl != NULL)
        impl->add_reference();
    }

    //--------------------------------------------------------------------------
    OutputRegion::~OutputRegion(void)
    //--------------------------------------------------------------------------
    {
      if (impl != NULL)
      {
        if (impl->remove_reference())
          delete impl;
        impl = NULL;
      }
    }

    //--------------------------------------------------------------------------
    OutputRegion& OutputRegion::operator=(const OutputRegion &rhs)
    //--------------------------------------------------------------------------
    {
      if (impl != NULL)
      {
        if (impl->remove_reference())
          delete impl;
      }
      impl = rhs.impl;
      if (impl != NULL)
        impl->add_reference();
      return *this;
    }

    //--------------------------------------------------------------------------
    Memory OutputRegion::target_memory(void) const
    //--------------------------------------------------------------------------
    {
      return impl->target_memory();
    }

    //--------------------------------------------------------------------------
    LogicalRegion OutputRegion::get_logical_region(void) const
    //--------------------------------------------------------------------------
    {
      return impl->get_logical_region();
    }

    //--------------------------------------------------------------------------
    bool OutputRegion::is_valid_output_region(void) const
    //--------------------------------------------------------------------------
    {
      return impl->is_valid_output_region();
    }

    //--------------------------------------------------------------------------
    void OutputRegion::check_type_tag(TypeTag type_tag) const
    //--------------------------------------------------------------------------
    {
      assert(impl != NULL);
      impl->check_type_tag(type_tag);
    }

    //--------------------------------------------------------------------------
    void OutputRegion::check_field_size(
                                      FieldID field_id, size_t field_size) const
    //--------------------------------------------------------------------------
    {
      assert(impl != NULL);
      impl->check_field_size(field_id, field_size);
    }

    //--------------------------------------------------------------------------
    void OutputRegion::get_layout(FieldID field_id,
                                  std::vector<DimensionKind> &ordering,
                                  size_t &alignment) const
    //--------------------------------------------------------------------------
    {
#ifdef DEBUG_LEGION
      assert(impl != NULL);
#endif
      impl->get_layout(field_id, ordering, alignment);
    }

    //--------------------------------------------------------------------------
    void OutputRegion::return_data(const DomainPoint &extents,
                                   FieldID field_id,
                                   Realm::RegionInstance instance,
                                   bool check_constraints /*= true */)
    //--------------------------------------------------------------------------
    {
      return_data(extents, field_id, instance, NULL, check_constraints);
    }

    //--------------------------------------------------------------------------
    void OutputRegion::return_data(const DomainPoint &extents,
                                   FieldID field_id,
                                   Realm::RegionInstance instance,
                                   const LayoutConstraintSet *constraints,
                                   bool check_constraints)
    //--------------------------------------------------------------------------
    {
#ifdef DEBUG_LEGION
      assert(impl != NULL);
#endif
      impl->return_data(
          extents, field_id, instance, constraints, check_constraints);
    }

    /////////////////////////////////////////////////////////////
    // ExternalResources
    /////////////////////////////////////////////////////////////

    //--------------------------------------------------------------------------
    ExternalResources::ExternalResources(void)
      : impl(NULL)
    //--------------------------------------------------------------------------
    {
    }

    ExternalResources::ExternalResources(const ExternalResources &rhs)
      : impl(rhs.impl)
    //--------------------------------------------------------------------------
    {
      if (impl != NULL)
        impl->add_reference();
    }

    //--------------------------------------------------------------------------
    ExternalResources::ExternalResources(Internal::ExternalResourcesImpl *i)
      : impl(i)
    //--------------------------------------------------------------------------
    {
      if (impl != NULL)
        impl->add_reference();
    }

    //--------------------------------------------------------------------------
    ExternalResources::ExternalResources(ExternalResources &&rhs) noexcept
      : impl(rhs.impl)
    //--------------------------------------------------------------------------
    {
      rhs.impl = NULL;
    }

    //--------------------------------------------------------------------------
    ExternalResources::~ExternalResources(void)
    //--------------------------------------------------------------------------
    {
      if ((impl != NULL) && impl->remove_reference())
        delete impl;
    }

    //--------------------------------------------------------------------------
    ExternalResources& ExternalResources::operator=(
                                                   const ExternalResources &rhs)
    //--------------------------------------------------------------------------
    {
      if ((impl != NULL) && impl->remove_reference())
        delete impl;
      impl = rhs.impl;
      if (impl != NULL)
        impl->add_reference();
      return *this;
    }

    //--------------------------------------------------------------------------
    ExternalResources& ExternalResources::operator=(
                                               ExternalResources &&rhs) noexcept
    //--------------------------------------------------------------------------
    {
      if ((impl != NULL) && impl->remove_reference())
        delete impl;
      impl = rhs.impl;
      rhs.impl = NULL;
      return *this;
    }

    //--------------------------------------------------------------------------
    size_t ExternalResources::size(void) const
    //--------------------------------------------------------------------------
    {
      if (impl == NULL)
        return 0;
      return impl->size();
    }

    //--------------------------------------------------------------------------
    PhysicalRegion ExternalResources::operator[](unsigned index) const
    //--------------------------------------------------------------------------
    {
      if (impl == NULL)
        return PhysicalRegion();
      return impl->get_region(index);
    }

    /////////////////////////////////////////////////////////////
    // Piece Iterator
    /////////////////////////////////////////////////////////////

    //--------------------------------------------------------------------------
    PieceIterator::PieceIterator(void)
      : impl(NULL), index(-1)
    //--------------------------------------------------------------------------
    {
    }

    //--------------------------------------------------------------------------
    PieceIterator::PieceIterator(const PhysicalRegion &region, FieldID fid,
                                 bool privilege_only, bool silence_warnings,
                                 const char *warning_string)
      : impl(NULL), index(-1)
    //--------------------------------------------------------------------------
    {
      if (region.impl != NULL)
        impl = region.impl->get_piece_iterator(fid, privilege_only,
                                  silence_warnings, warning_string);
      if (impl != NULL)
      {
        impl->add_reference();
        index = impl->get_next(index, current_piece);
      }
    }

    //--------------------------------------------------------------------------
    PieceIterator::PieceIterator(const PieceIterator &rhs)
      : impl(rhs.impl), index(rhs.index), current_piece(rhs.current_piece)
    //--------------------------------------------------------------------------
    {
      if (impl != NULL)
        impl->add_reference();
    }

    //--------------------------------------------------------------------------
    PieceIterator::PieceIterator(PieceIterator &&rhs) noexcept
      : impl(rhs.impl), index(rhs.index), current_piece(rhs.current_piece)
    //--------------------------------------------------------------------------
    {
      rhs.impl = NULL;
    }

    //--------------------------------------------------------------------------
    PieceIterator::~PieceIterator(void)
    //--------------------------------------------------------------------------
    {
      if ((impl != NULL) && impl->remove_reference())
        delete impl;
    }

    //--------------------------------------------------------------------------
    PieceIterator& PieceIterator::operator=(const PieceIterator &rhs)
    //--------------------------------------------------------------------------
    {
      if ((impl != NULL) && impl->remove_reference())
        delete impl;
      impl = rhs.impl;
      index = rhs.index;
      current_piece = rhs.current_piece;
      if (impl != NULL)
        impl->add_reference();
      return *this;
    }

    //--------------------------------------------------------------------------
    PieceIterator& PieceIterator::operator=(PieceIterator &&rhs) noexcept
    //--------------------------------------------------------------------------
    {
      if ((impl != NULL) && impl->remove_reference())
        delete impl;
      impl = rhs.impl;
      rhs.impl = NULL;
      index = rhs.index;
      current_piece = rhs.current_piece;
      return *this;
    }

    //--------------------------------------------------------------------------
    bool PieceIterator::step(void)
    //--------------------------------------------------------------------------
    {
      if ((impl != NULL) && (index >= 0))
        index = impl->get_next(index, current_piece);
      return valid();
    }

    LEGION_DISABLE_DEPRECATED_WARNINGS

    /////////////////////////////////////////////////////////////
    // Index Iterator  
    /////////////////////////////////////////////////////////////

    //--------------------------------------------------------------------------
    IndexIterator::IndexIterator(void)
    //--------------------------------------------------------------------------
    {
    }

    //--------------------------------------------------------------------------
    IndexIterator::IndexIterator(const Domain &dom, ptr_t start)
    //--------------------------------------------------------------------------
    {
#ifdef DEBUG_LEGION
      assert(dom.get_dim() == 1);
#endif
      const DomainT<1,coord_t> is = dom;
      is_iterator = Realm::IndexSpaceIterator<1,coord_t>(is);
    }

    //--------------------------------------------------------------------------
    IndexIterator::IndexIterator(Runtime *rt, Context ctx,
                                 IndexSpace space, ptr_t start)
    //--------------------------------------------------------------------------
    {
      Domain dom = rt->get_index_space_domain(ctx, space);
#ifdef DEBUG_LEGION
      assert(dom.get_dim() == 1);
#endif
      const DomainT<1,coord_t> is = dom;
      is_iterator = Realm::IndexSpaceIterator<1,coord_t>(is);
    }

    //--------------------------------------------------------------------------
    IndexIterator::IndexIterator(Runtime *rt, Context ctx,
                                 LogicalRegion handle, ptr_t start)
    //--------------------------------------------------------------------------
    {
      Domain dom = rt->get_index_space_domain(ctx, handle.get_index_space());
#ifdef DEBUG_LEGION
      assert(dom.get_dim() == 1);
#endif
      const DomainT<1,coord_t> is = dom;
      is_iterator = Realm::IndexSpaceIterator<1,coord_t>(is);
    }

    //--------------------------------------------------------------------------
    IndexIterator::IndexIterator(Runtime *rt, IndexSpace space, ptr_t start)
    //--------------------------------------------------------------------------
    {
      Domain dom = rt->get_index_space_domain(space);
#ifdef DEBUG_LEGION
      assert(dom.get_dim() == 1);
#endif
      const DomainT<1,coord_t> is = dom;
      is_iterator = Realm::IndexSpaceIterator<1,coord_t>(is);
    }

    //--------------------------------------------------------------------------
    IndexIterator::IndexIterator(const IndexIterator &rhs)
      : is_iterator(rhs.is_iterator), rect_iterator(rhs.rect_iterator)
    //--------------------------------------------------------------------------
    {
    }

    //--------------------------------------------------------------------------
    IndexIterator::~IndexIterator(void)
    //--------------------------------------------------------------------------
    {
    }

    //--------------------------------------------------------------------------
    IndexIterator& IndexIterator::operator=(const IndexIterator &rhs)
    //--------------------------------------------------------------------------
    {
      is_iterator = rhs.is_iterator;
      rect_iterator = rhs.rect_iterator;
      return *this;
    }

    /////////////////////////////////////////////////////////////
    // IndexAllocator 
    /////////////////////////////////////////////////////////////

    //--------------------------------------------------------------------------
    IndexAllocator::IndexAllocator(void)
      : index_space(IndexSpace::NO_SPACE)
    //--------------------------------------------------------------------------
    {
    }

    //--------------------------------------------------------------------------
    IndexAllocator::IndexAllocator(const IndexAllocator &rhs)
      : index_space(rhs.index_space), iterator(rhs.iterator)
    //--------------------------------------------------------------------------
    {
    }

    //--------------------------------------------------------------------------
    IndexAllocator::IndexAllocator(IndexSpace is, IndexIterator itr)
      : index_space(is), iterator(itr)
    //--------------------------------------------------------------------------
    {
    }

    //--------------------------------------------------------------------------
    IndexAllocator::~IndexAllocator(void)
    //--------------------------------------------------------------------------
    {
    }

    //--------------------------------------------------------------------------
    IndexAllocator& IndexAllocator::operator=(const IndexAllocator &rhs)
    //--------------------------------------------------------------------------
    {
      index_space = rhs.index_space;
      iterator = rhs.iterator;
      return *this;
    }

    //--------------------------------------------------------------------------
    ptr_t IndexAllocator::alloc(unsigned num_elements)
    //--------------------------------------------------------------------------
    {
      size_t allocated = 0;
      ptr_t result = iterator.next_span(allocated, num_elements);
      if (allocated == num_elements)
        return result;
      else
        return ptr_t::nil();
    }

    //--------------------------------------------------------------------------
    void IndexAllocator::free(ptr_t ptr, unsigned num_elements)
    //--------------------------------------------------------------------------
    {
      Internal::log_run.error("Dynamic free of index space points is "
                              "no longer supported");
      assert(false);
    }

    LEGION_REENABLE_DEPRECATED_WARNINGS

    /////////////////////////////////////////////////////////////
    // Field Allocator
    /////////////////////////////////////////////////////////////

    //--------------------------------------------------------------------------
    FieldAllocator::FieldAllocator(void)
      : impl(NULL)
    //--------------------------------------------------------------------------
    {
    }
    
    //--------------------------------------------------------------------------
    FieldAllocator::FieldAllocator(const FieldAllocator &rhs)
      : impl(rhs.impl)
    //--------------------------------------------------------------------------
    {
      if (impl != NULL)
        impl->add_reference();
    }

    //--------------------------------------------------------------------------
    FieldAllocator::FieldAllocator(FieldAllocator &&rhs) noexcept
      : impl(rhs.impl)
    //--------------------------------------------------------------------------
    {
      rhs.impl = NULL;
    }

    //--------------------------------------------------------------------------
    FieldAllocator::~FieldAllocator(void)
    //--------------------------------------------------------------------------
    {
      if (impl != NULL)
      {
        if (impl->remove_reference())
          delete impl;
        impl = NULL;
      }
    }

    //--------------------------------------------------------------------------
    FieldAllocator::FieldAllocator(Internal::FieldAllocatorImpl *i)
      : impl(i)
    //--------------------------------------------------------------------------
    {
      if (impl != NULL)
        impl->add_reference();
    }

    //--------------------------------------------------------------------------
    FieldAllocator& FieldAllocator::operator=(const FieldAllocator &rhs)
    //--------------------------------------------------------------------------
    {
      if ((impl != NULL) && impl->remove_reference())
        delete impl;
      impl = rhs.impl;
      if (impl != NULL)
        impl->add_reference();
      return *this;
    }

    //--------------------------------------------------------------------------
    FieldAllocator& FieldAllocator::operator=(FieldAllocator &&rhs) noexcept
    //--------------------------------------------------------------------------
    {
      if ((impl != NULL) && impl->remove_reference())
        delete impl;
      impl = rhs.impl;
      rhs.impl = NULL;
      return *this;
    }

    //--------------------------------------------------------------------------
    FieldID FieldAllocator::allocate_field(size_t field_size,
                                           FieldID desired_fieldid,
                                           CustomSerdezID serdez_id, bool local,
                                           const char *prov)
    //--------------------------------------------------------------------------
    {
#ifdef DEBUG_LEGION
      assert(impl != NULL);
#endif
      Internal::AutoProvenance provenance(prov);
      return impl->allocate_field(field_size, desired_fieldid, serdez_id,
                                  local, provenance);
    }

    //--------------------------------------------------------------------------
    FieldID FieldAllocator::allocate_field(const Future &field_size,
                                           FieldID desired_fieldid,
                                           CustomSerdezID serdez_id, bool local,
                                           const char *prov)
    //--------------------------------------------------------------------------
    {
#ifdef DEBUG_LEGION
      assert(impl != NULL);
#endif
      Internal::AutoProvenance provenance(prov);
      return impl->allocate_field(field_size, desired_fieldid, serdez_id,
                                  local, provenance);
    }

    //--------------------------------------------------------------------------
    void FieldAllocator::free_field(FieldID fid, const bool unordered,
                                    const char *prov)
    //--------------------------------------------------------------------------
    {
#ifdef DEBUG_LEGION
      assert(impl != NULL);
#endif     
      Internal::AutoProvenance provenance(prov);
      impl->free_field(fid, unordered, provenance);
    }

    //--------------------------------------------------------------------------
    FieldID FieldAllocator::allocate_local_field(size_t field_size,
                                                 FieldID desired_fieldid,
                                                 CustomSerdezID serdez_id,
                                                 const char *prov)
    //--------------------------------------------------------------------------
    {
#ifdef DEBUG_LEGION
      assert(impl != NULL);
#endif
      Internal::AutoProvenance provenance(prov);
      return impl->allocate_field(field_size, desired_fieldid, 
                                  serdez_id, true/*local*/, provenance);
    }

    //--------------------------------------------------------------------------
    void FieldAllocator::allocate_fields(const std::vector<size_t> &field_sizes,
                                         std::vector<FieldID> &resulting_fields,
                                         CustomSerdezID serdez_id, bool local,
                                         const char *prov)
    //--------------------------------------------------------------------------
    {
#ifdef DEBUG_LEGION
      assert(impl != NULL);
#endif
      Internal::AutoProvenance provenance(prov);
      impl->allocate_fields(field_sizes, resulting_fields, serdez_id,
                            local, provenance);
    }

    //--------------------------------------------------------------------------
    void FieldAllocator::allocate_fields(const std::vector<Future> &field_sizes,
                                         std::vector<FieldID> &resulting_fields,
                                         CustomSerdezID serdez_id, bool local,
                                         const char *prov)
    //--------------------------------------------------------------------------
    {
#ifdef DEBUG_LEGION
      assert(impl != NULL);
#endif
      Internal::AutoProvenance provenance(prov);
      impl->allocate_fields(field_sizes, resulting_fields,
                            serdez_id, local, provenance);
    }

    //--------------------------------------------------------------------------
    void FieldAllocator::free_fields(const std::set<FieldID> &to_free,
                                     const bool unordered, const char *prov)
    //--------------------------------------------------------------------------
    {
#ifdef DEBUG_LEGION
      assert(impl != NULL);
#endif
      Internal::AutoProvenance provenance(prov);
      impl->free_fields(to_free, unordered, provenance);
    }

    //--------------------------------------------------------------------------
    void FieldAllocator::allocate_local_fields(
                                        const std::vector<size_t> &field_sizes,
                                        std::vector<FieldID> &resulting_fields,
                                        CustomSerdezID serdez_id,
                                        const char *prov)
    //--------------------------------------------------------------------------
    {
#ifdef DEBUG_LEGION
      assert(impl != NULL);
#endif
      Internal::AutoProvenance provenance(prov);
      impl->allocate_fields(field_sizes, resulting_fields, 
                            serdez_id, true/*local*/, provenance);
    }

    //--------------------------------------------------------------------------
    FieldSpace FieldAllocator::get_field_space(void) const
    //--------------------------------------------------------------------------
    {
      if (impl == NULL)
        return FieldSpace::NO_SPACE;
      else
        return impl->get_field_space();
    }

    /////////////////////////////////////////////////////////////
    // Task Config Options 
    /////////////////////////////////////////////////////////////

    //--------------------------------------------------------------------------
    TaskConfigOptions::TaskConfigOptions(bool l /*=false*/,
                                         bool in /*=false*/,
                                         bool idem /*=false*/)
      : leaf(l), inner(in), idempotent(idem)
    //--------------------------------------------------------------------------
    {
    }

    /////////////////////////////////////////////////////////////
    // ProjectionFunctor 
    /////////////////////////////////////////////////////////////

    //--------------------------------------------------------------------------
    ProjectionFunctor::ProjectionFunctor(void)
      : runtime(NULL)
    //--------------------------------------------------------------------------
    {
    }

    //--------------------------------------------------------------------------
    ProjectionFunctor::ProjectionFunctor(Runtime *rt)
      : runtime(rt)
    //--------------------------------------------------------------------------
    {
    }

    //--------------------------------------------------------------------------
    ProjectionFunctor::~ProjectionFunctor(void)
    //--------------------------------------------------------------------------
    {
    }

    // FIXME: This exists for backwards compatibility but it is tripping
    // over our own deprecation warnings. Turn those off inside this method.
    LEGION_DISABLE_DEPRECATED_WARNINGS

    //--------------------------------------------------------------------------
    LogicalRegion ProjectionFunctor::project(const Mappable *mappable, 
            unsigned index, LogicalRegion upper_bound, const DomainPoint &point)
    //--------------------------------------------------------------------------
    {
      if (is_functional())
      {
        switch (mappable->get_mappable_type())
        {
          case LEGION_TASK_MAPPABLE:
            {
              const Task *task = mappable->as_task();
              return project(upper_bound, point, task->index_domain);
            }
          case LEGION_COPY_MAPPABLE:
            {
              const Copy *copy = mappable->as_copy();
              return project(upper_bound, point, copy->index_domain);
            }
          case LEGION_INLINE_MAPPABLE:
          case LEGION_ACQUIRE_MAPPABLE:
          case LEGION_RELEASE_MAPPABLE:
          case LEGION_CLOSE_MAPPABLE:
            {
              const Domain launch_domain(point, point);
              return project(upper_bound, point, launch_domain);
            }
          case LEGION_FILL_MAPPABLE:
            {
              const Fill *fill = mappable->as_fill();
              return project(upper_bound, point, fill->index_domain);
            }
          case LEGION_PARTITION_MAPPABLE:
            {
              const Partition *part = mappable->as_partition();
              return project(upper_bound, point, part->index_domain);
            }
          case LEGION_MUST_EPOCH_MAPPABLE:
            {
              const MustEpoch *must = mappable->as_must_epoch();
              return project(upper_bound, point, must->launch_domain);
            }
          default:
            REPORT_LEGION_ERROR(ERROR_UNKNOWN_MAPPABLE, 
                                "Unknown mappable type passed to projection "
                                "functor! You must override the default "
                                "implementations of the non-deprecated "
                                "'project' methods!");
        }
      }
      else
      {
#ifdef DEBUG_LEGION
        REPORT_LEGION_WARNING(LEGION_WARNING_NEW_PROJECTION_FUNCTORS, 
                              "THERE ARE NEW METHODS FOR PROJECTION FUNCTORS "
                              "THAT MUST BE OVERRIDEN! CALLING DEPRECATED "
                              "METHODS FOR NOW!");
#endif
        switch (mappable->get_mappable_type())
        {
          case LEGION_TASK_MAPPABLE:
            return project(0/*dummy ctx*/, 
                           const_cast<Task*>(mappable->as_task()),
                           index, upper_bound, point);
          default:
            REPORT_LEGION_ERROR(ERROR_UNKNOWN_MAPPABLE, 
                                "Unknown mappable type passed to projection "
                                "functor! You must override the default "
                                "implementations of the non-deprecated "
                                "'project' methods!");
        }
      }
      return LogicalRegion::NO_REGION;
    }

    //--------------------------------------------------------------------------
    LogicalRegion ProjectionFunctor::project(const Mappable *mappable,
         unsigned index, LogicalPartition upper_bound, const DomainPoint &point)
    //--------------------------------------------------------------------------
    {
      if (is_functional())
      {
        switch (mappable->get_mappable_type())
        {
          case LEGION_TASK_MAPPABLE:
            {
              const Task *task = mappable->as_task();
              return project(upper_bound, point, task->index_domain);
            }
          case LEGION_COPY_MAPPABLE:
            {
              const Copy *copy = mappable->as_copy();
              return project(upper_bound, point, copy->index_domain);
            }
          case LEGION_INLINE_MAPPABLE:
          case LEGION_ACQUIRE_MAPPABLE:
          case LEGION_RELEASE_MAPPABLE:
          case LEGION_CLOSE_MAPPABLE:
            {
              const Domain launch_domain(point, point);
              return project(upper_bound, point, launch_domain);
            }
          case LEGION_FILL_MAPPABLE:
            {
              const Fill *fill = mappable->as_fill();
              return project(upper_bound, point, fill->index_domain);
            }
          case LEGION_PARTITION_MAPPABLE:
            {
              const Partition *part = mappable->as_partition();
              return project(upper_bound, point, part->index_domain);
            }
          case LEGION_MUST_EPOCH_MAPPABLE:
            {
              const MustEpoch *must = mappable->as_must_epoch();
              return project(upper_bound, point, must->launch_domain);
            }
          default:
            REPORT_LEGION_ERROR(ERROR_UNKNOWN_MAPPABLE, 
                                "Unknown mappable type passed to projection "
                                "functor! You must override the default "
                                "implementations of the non-deprecated "
                                "'project' methods!");
        }
      }
      else
      {
#ifdef DEBUG_LEGION
        REPORT_LEGION_WARNING(LEGION_WARNING_NEW_PROJECTION_FUNCTORS, 
                              "THERE ARE NEW METHODS FOR PROJECTION FUNCTORS "
                              "THAT MUST BE OVERRIDEN! CALLING DEPRECATED "
                              "METHODS FOR NOW!");
#endif
        switch (mappable->get_mappable_type())
        {
          case LEGION_TASK_MAPPABLE:
            return project(0/*dummy ctx*/, 
                           const_cast<Task*>(mappable->as_task()),
                           index, upper_bound, point);
          default:
            REPORT_LEGION_ERROR(ERROR_UNKNOWN_MAPPABLE, 
                                "Unknown mappable type passed to projection "
                                "functor! You must override the default "
                                "implementations of the non-deprecated "
                                "'project' methods!");
                assert(false);
        }
      }
      return LogicalRegion::NO_REGION;
    }

    LEGION_REENABLE_DEPRECATED_WARNINGS

    //--------------------------------------------------------------------------
    LogicalRegion ProjectionFunctor::project(LogicalRegion upper_bound,
                          const DomainPoint &point, const Domain &launch_domain)
    //--------------------------------------------------------------------------
    {
      // Must be override by derived classes
      assert(false);
      return LogicalRegion::NO_REGION;
    }

    //--------------------------------------------------------------------------
    LogicalRegion ProjectionFunctor::project(LogicalPartition upper_bound,
                          const DomainPoint &point, const Domain &launch_domain)
    //--------------------------------------------------------------------------
    {
      // Must be override by derived classes
      assert(false);
      return LogicalRegion::NO_REGION;
    }

    //--------------------------------------------------------------------------
    LogicalRegion ProjectionFunctor::project(LogicalRegion upper_bound,
                          const DomainPoint &point, const Domain &launch_domain, 
                          const void *args, size_t size)
    //--------------------------------------------------------------------------
    {
      return project(upper_bound, point, launch_domain);
    }

    //--------------------------------------------------------------------------
    LogicalRegion ProjectionFunctor::project(LogicalPartition upper_bound,
                          const DomainPoint &point, const Domain &launch_domain, 
                          const void *args, size_t size)
    //--------------------------------------------------------------------------
    {
      return project(upper_bound, point, launch_domain);
    }

    //--------------------------------------------------------------------------
    LogicalRegion ProjectionFunctor::project(Context ctx, Task *task,
            unsigned index, LogicalRegion upper_bound, const DomainPoint &point)
    //--------------------------------------------------------------------------
    {
      REPORT_LEGION_ERROR(ERROR_DEPRECATED_PROJECTION, 
                          "INVOCATION OF DEPRECATED PROJECTION "
                          "FUNCTOR METHOD WITHOUT AN OVERRIDE!");
      return LogicalRegion::NO_REGION;
    }

    //--------------------------------------------------------------------------
    LogicalRegion ProjectionFunctor::project(Context ctx, Task *task,
         unsigned index, LogicalPartition upper_bound, const DomainPoint &point)
    //--------------------------------------------------------------------------
    {
      REPORT_LEGION_ERROR(ERROR_DEPRECATED_PROJECTION, 
                          "INVOCATION OF DEPRECATED PROJECTION "
                          "FUNCTOR METHOD WITHOUT AN OVERRIDE!");
      return LogicalRegion::NO_REGION;
    }

    //--------------------------------------------------------------------------
    void ProjectionFunctor::invert(LogicalRegion region, LogicalRegion upper, 
          const Domain &launch_domain, std::vector<DomainPoint> &ordered_points)
    //--------------------------------------------------------------------------
    {
      // Must be override by derived classes
      assert(false);
    }

    //--------------------------------------------------------------------------
    void ProjectionFunctor::invert(LogicalRegion region, LogicalPartition upper, 
          const Domain &launch_domain, std::vector<DomainPoint> &ordered_points)
    //--------------------------------------------------------------------------
    {
      // Must be override by derived classes
      assert(false);
    }

    //--------------------------------------------------------------------------
    bool ProjectionFunctor::is_complete(LogicalRegion upper_bound,
                                        const Domain &launch_domain)
    //--------------------------------------------------------------------------
    {
      return false;
    }

    //--------------------------------------------------------------------------
    bool ProjectionFunctor::is_complete(LogicalPartition upper_bound,
                                        const Domain &launch_domain)
    //--------------------------------------------------------------------------
    {
      return false;
    }

    //--------------------------------------------------------------------------
    bool ProjectionFunctor::is_complete(Mappable *mappable, unsigned index,
                         LogicalRegion upper_bound, const Domain &launch_domain)
    //--------------------------------------------------------------------------
    {
      return false;
    }

    //--------------------------------------------------------------------------
    bool ProjectionFunctor::is_complete(Mappable *mappable, unsigned index,
                      LogicalPartition upper_bound, const Domain &launch_domain)
    //--------------------------------------------------------------------------
    {
      return false;
    }

    /////////////////////////////////////////////////////////////
    // ShardingFunctor 
    /////////////////////////////////////////////////////////////

    //--------------------------------------------------------------------------
    ShardingFunctor::ShardingFunctor(void)
    //--------------------------------------------------------------------------
    {
    }

    //--------------------------------------------------------------------------
    ShardingFunctor::~ShardingFunctor(void)
    //--------------------------------------------------------------------------
    {
    }

    //--------------------------------------------------------------------------
    ShardID ShardingFunctor::shard(const DomainPoint &index_point,
                                   const Domain &index_domain,
                                   const size_t total_shards)
    //--------------------------------------------------------------------------
    {
      REPORT_LEGION_ERROR(ERROR_DEPRECATED_SHARDING,
          "Invocation of 'ShardingFunctor::shard' method "
          "without a user-provided override");
      return 0;
    }

    //--------------------------------------------------------------------------
    DomainPoint ShardingFunctor::shard_points(const DomainPoint &index_point,
                                   const Domain &index_domain,
                                   const std::vector<DomainPoint> &shard_points,
                                   const Domain &shard_domain)
    //--------------------------------------------------------------------------
    {
      REPORT_LEGION_ERROR(ERROR_DEPRECATED_SHARDING,
          "Invocation of 'ShardingFunctor::shard_points' method "
          "without a user-provided override");
      return DomainPoint();
    }

    //--------------------------------------------------------------------------
    void ShardingFunctor::invert(ShardID shard,
                                 const Domain &sharding_domain,
                                 const Domain &index_domain,
                                 const size_t total_shards,
                                 std::vector<DomainPoint> &points)
    //--------------------------------------------------------------------------
    {
      REPORT_LEGION_ERROR(ERROR_DEPRECATED_SHARDING,
          "Invocation of 'ShardingFunctor::invert' method "
          "without a user-provided override");
    }

    //--------------------------------------------------------------------------
    void ShardingFunctor::invert_points(const DomainPoint &shard_point,
                                 const std::vector<DomainPoint> &shard_points,
                                 const Domain &shard_domain,
                                 const Domain &index_domain,
                                 const Domain &sharding_domain,
                                 std::vector<DomainPoint> &index_points)
    //--------------------------------------------------------------------------
    {
      REPORT_LEGION_ERROR(ERROR_DEPRECATED_SHARDING,
          "Invocation of 'ShardingFunctor::invert_points' method "
          "without a user-provided override");
    }
    
    /////////////////////////////////////////////////////////////
    // Coloring Serializer 
    /////////////////////////////////////////////////////////////

    //--------------------------------------------------------------------------
    ColoringSerializer::ColoringSerializer(const Coloring &c)
      : coloring(c)
    //--------------------------------------------------------------------------
    {
    }

    //--------------------------------------------------------------------------
    size_t ColoringSerializer::legion_buffer_size(void) const
    //--------------------------------------------------------------------------
    {
      size_t result = sizeof(size_t); // number of elements
      for (Coloring::const_iterator it = coloring.begin();
            it != coloring.end(); it++)
      {
        result += sizeof(Color);
        result += 2*sizeof(size_t); // number of each kind of pointer
        result += (it->second.points.size() * sizeof(ptr_t));
        result += (it->second.ranges.size() * 2 * sizeof(ptr_t));
      }
      return result;
    }

    //--------------------------------------------------------------------------
    size_t ColoringSerializer::legion_serialize(void *buffer) const
    //--------------------------------------------------------------------------
    {
      char *target = (char*)buffer; 
      *((size_t*)target) = coloring.size();
      target += sizeof(size_t);
      for (Coloring::const_iterator it = coloring.begin();
            it != coloring.end(); it++)
      {
        *((Color*)target) = it->first;
        target += sizeof(it->first);
        *((size_t*)target) = it->second.points.size();
        target += sizeof(size_t);
        for (std::set<ptr_t>::const_iterator ptr_it = it->second.points.begin();
              ptr_it != it->second.points.end(); ptr_it++)
        {
          *((ptr_t*)target) = *ptr_it;
          target += sizeof(ptr_t);
        }
        *((size_t*)target) = it->second.ranges.size();
        target += sizeof(size_t);
        for (std::set<std::pair<ptr_t,ptr_t> >::const_iterator range_it = 
              it->second.ranges.begin(); range_it != it->second.ranges.end();
              range_it++)
        {
          *((ptr_t*)target) = range_it->first;
          target += sizeof(range_it->first);
          *((ptr_t*)target) = range_it->second;
          target += sizeof(range_it->second);
        }
      }
      return (size_t(target) - size_t(buffer));
    }

    //--------------------------------------------------------------------------
    size_t ColoringSerializer::legion_deserialize(const void *buffer)
    //--------------------------------------------------------------------------
    {
      const char *source = (const char*)buffer;
      size_t num_colors = *((const size_t*)source);
      source += sizeof(num_colors);
      for (unsigned idx = 0; idx < num_colors; idx++)
      {
        Color c = *((const Color*)source);
        source += sizeof(c);
        coloring[c]; // Force coloring to exist even if empty.
        size_t num_points = *((const size_t*)source);
        source += sizeof(num_points);
        for (unsigned p = 0; p < num_points; p++)
        {
          ptr_t ptr = *((const ptr_t*)source);
          source += sizeof(ptr);
          coloring[c].points.insert(ptr);
        }
        size_t num_ranges = *((const size_t*)source);
        source += sizeof(num_ranges);
        for (unsigned r = 0; r < num_ranges; r++)
        {
          ptr_t start = *((const ptr_t*)source);
          source += sizeof(start);
          ptr_t stop = *((const ptr_t*)source);
          source += sizeof(stop);
          coloring[c].ranges.insert(std::pair<ptr_t,ptr_t>(start,stop));
        }
      }
      // Return the number of bytes consumed
      return (size_t(source) - size_t(buffer));
    }

    /////////////////////////////////////////////////////////////
    // Domain Coloring Serializer 
    /////////////////////////////////////////////////////////////

    //--------------------------------------------------------------------------
    DomainColoringSerializer::DomainColoringSerializer(const DomainColoring &d)
      : coloring(d)
    //--------------------------------------------------------------------------
    {
    }

    //--------------------------------------------------------------------------
    size_t DomainColoringSerializer::legion_buffer_size(void) const
    //--------------------------------------------------------------------------
    {
      size_t result = sizeof(size_t); // number of elements
      result += (coloring.size() * (sizeof(Color) + sizeof(Domain)));
      return result;
    }

    //--------------------------------------------------------------------------
    size_t DomainColoringSerializer::legion_serialize(void *buffer) const
    //--------------------------------------------------------------------------
    {
      char *target = (char*)buffer;
      *((size_t*)target) = coloring.size();
      target += sizeof(size_t);
      for (DomainColoring::const_iterator it = coloring.begin();
            it != coloring.end(); it++)
      {
        *((Color*)target) = it->first; 
        target += sizeof(it->first);
        *((Domain*)target) = it->second;
        target += sizeof(it->second);
      }
      return (size_t(target) - size_t(buffer));
    }

    //--------------------------------------------------------------------------
    size_t DomainColoringSerializer::legion_deserialize(const void *buffer)
    //--------------------------------------------------------------------------
    {
      const char *source = (const char*)buffer;
      size_t num_elements = *((const size_t*)source);
      source += sizeof(size_t);
      for (unsigned idx = 0; idx < num_elements; idx++)
      {
        Color c = *((const Color*)source);
        source += sizeof(c);
        Domain d = *((const Domain*)source);
        source += sizeof(d);
        coloring[c] = d;
      }
      // Return the number of bytes consumed
      return (size_t(source) - size_t(buffer));
    }

    /////////////////////////////////////////////////////////////
    // Legion Runtime 
    /////////////////////////////////////////////////////////////

    //--------------------------------------------------------------------------
    Runtime::Runtime(Internal::Runtime *rt)
      : runtime(rt)
    //--------------------------------------------------------------------------
    {
    }

    //--------------------------------------------------------------------------
    IndexSpace Runtime::create_index_space(Context ctx, size_t max_num_elmts)
    //--------------------------------------------------------------------------
    {
      const Rect<1,coord_t> bounds((Point<1,coord_t>(0)),
                                   (Point<1,coord_t>(max_num_elmts-1)));
      const Domain domain(bounds);
      return create_index_space(ctx, domain, TYPE_TAG_1D, NULL/*provenance*/);
    }

    //--------------------------------------------------------------------------
    IndexSpace Runtime::create_index_space(Context ctx, const Domain &domain,
                                           TypeTag type_tag, const char *prov)
    //--------------------------------------------------------------------------
    {
      Internal::AutoProvenance provenance(prov);
      switch (domain.get_dim())
      {
#define DIMFUNC(DIM) \
        case DIM:                       \
          {                             \
            if (type_tag == 0) \
              type_tag = TYPE_TAG_##DIM##D; \
            return ctx->create_index_space(domain, type_tag, provenance); \
          }
        LEGION_FOREACH_N(DIMFUNC)
#undef DIMFUNC
        default:
          assert(false);
      }
      return IndexSpace::NO_SPACE;
    }

    //--------------------------------------------------------------------------
    IndexSpace Runtime::create_index_space(Context ctx, size_t dimensions,
                       const Future &future, TypeTag type_tag, const char *prov)
    //--------------------------------------------------------------------------
    {
      Internal::AutoProvenance provenance(prov);
      if (type_tag == 0)
      {
        switch (dimensions)
        {
#define DIMFUNC(DIM) \
        case DIM:                       \
          {                             \
            type_tag = TYPE_TAG_##DIM##D; \
            break; \
          }
        LEGION_FOREACH_N(DIMFUNC)
#undef DIMFUNC
        default:
          assert(false);
        }
      }
      return ctx->create_index_space(future, type_tag, provenance);
    }

    //--------------------------------------------------------------------------
    IndexSpace Runtime::create_index_space(Context ctx, 
                                           const std::set<Domain> &domains)
    //--------------------------------------------------------------------------
    {
      std::vector<Domain> rects(domains.begin(), domains.end());
      return create_index_space(ctx, rects, NULL/*provenance*/); 
    }

    //--------------------------------------------------------------------------
    IndexSpace Runtime::create_index_space(Context ctx,
                       const std::vector<DomainPoint> &points, const char *prov)
    //--------------------------------------------------------------------------
    {
      Internal::AutoProvenance provenance(prov);
      return ctx->create_index_space(points, provenance); 
    }

    //--------------------------------------------------------------------------
    IndexSpace Runtime::create_index_space(Context ctx,
                              const std::vector<Domain> &rects, const char *prov)
    //--------------------------------------------------------------------------
    {
      Internal::AutoProvenance provenance(prov);
      return ctx->create_index_space(rects, provenance); 
    }

    //--------------------------------------------------------------------------
    IndexSpace Runtime::union_index_spaces(Context ctx,
                        const std::vector<IndexSpace> &spaces, const char *prov)
    //--------------------------------------------------------------------------
    {
      Internal::AutoProvenance provenance(prov);
      return ctx->union_index_spaces(spaces, provenance);
    }

    //--------------------------------------------------------------------------
    IndexSpace Runtime::intersect_index_spaces(Context ctx,
                        const std::vector<IndexSpace> &spaces, const char *prov)
    //--------------------------------------------------------------------------
    {
      Internal::AutoProvenance provenance(prov);
      return ctx->intersect_index_spaces(spaces, provenance);
    }

    //--------------------------------------------------------------------------
    IndexSpace Runtime::subtract_index_spaces(Context ctx,
                            IndexSpace left, IndexSpace right, const char *prov)
    //--------------------------------------------------------------------------
    {
      Internal::AutoProvenance provenance(prov);
      return ctx->subtract_index_spaces(left, right, provenance);
    }

    //--------------------------------------------------------------------------
    void Runtime::create_shared_ownership(Context ctx, IndexSpace handle)
    //--------------------------------------------------------------------------
    {
      ctx->create_shared_ownership(handle);
    }

    //--------------------------------------------------------------------------
    void Runtime::destroy_index_space(Context ctx, IndexSpace handle,
                     const bool unordered, const bool recurse, const char *prov)
    //--------------------------------------------------------------------------
    {
      Internal::AutoProvenance provenance(prov);
      ctx->destroy_index_space(handle, unordered, recurse, provenance);
    } 

    //--------------------------------------------------------------------------
    IndexPartition Runtime::create_index_partition(Context ctx,
                                          IndexSpace parent,
                                          const Domain &color_space,
                                          const PointColoring &coloring,
                                          PartitionKind part_kind,
                                          Color color, bool allocable)
    //--------------------------------------------------------------------------
    {
      if (allocable)
        Internal::log_run.warning("WARNING: allocable index partitions are "
                                  "no longer supported");
      std::map<DomainPoint,Domain> domains;
      for (PointColoring::const_iterator cit = 
            coloring.begin(); cit != coloring.end(); cit++)
      {
        if (cit->second.ranges.empty())
        {
          std::vector<Realm::Point<1,coord_t> > 
            points(cit->second.points.size());
          unsigned index = 0;
          for (std::set<ptr_t>::const_iterator it = 
                cit->second.points.begin(); it != 
                cit->second.points.end(); it++)
            points[index++] = Realm::Point<1,coord_t>(*it);
          const Realm::IndexSpace<1,coord_t> space(points);
          domains[cit->first] = DomainT<1,coord_t>(space);
        }
        else
        {
          std::vector<Realm::Rect<1,coord_t> >
            ranges(cit->second.points.size() + cit->second.ranges.size());
          unsigned index = 0;
          for (std::set<ptr_t>::const_iterator it = 
                cit->second.points.begin(); it != 
                cit->second.points.end(); it++)
          {
            Realm::Point<1,coord_t> point(*it);
            ranges[index++] = Realm::Rect<1,coord_t>(point, point);
          }
          for (std::set<std::pair<ptr_t,ptr_t> >::iterator it = 
                cit->second.ranges.begin(); it !=
                cit->second.ranges.end(); it++)
          {
            Realm::Point<1,coord_t> lo(it->first);
            Realm::Point<1,coord_t> hi(it->second);
            ranges[index++] = Realm::Rect<1,coord_t>(lo, hi);
          }
          const Realm::IndexSpace<1,coord_t> space(ranges);
          domains[cit->first] = DomainT<1,coord_t>(space);
        }
      }
      // Make an index space for the color space
      IndexSpace index_color_space = create_index_space(ctx, color_space);
      IndexPartition result = create_partition_by_domain(ctx, parent, domains,
          index_color_space, true/*perform intersections*/, part_kind, color);
      return result;
    }

    //--------------------------------------------------------------------------
    IndexPartition Runtime::create_index_partition(
                                          Context ctx, IndexSpace parent,
                                          const Coloring &coloring,
                                          bool disjoint,
                                          Color part_color)
    //--------------------------------------------------------------------------
    {
      std::map<DomainPoint,Domain> domains;
      Color lower_bound = UINT_MAX, upper_bound = 0;
      for (Coloring::const_iterator cit = 
            coloring.begin(); cit != coloring.end(); cit++)
      {
        if (cit->first < lower_bound)
          lower_bound = cit->first;
        if (cit->first > upper_bound)
          upper_bound = cit->first;
        const DomainPoint color = Point<1,coord_t>(cit->first);
        if (cit->second.ranges.empty())
        {
          std::vector<Realm::Point<1,coord_t> > 
            points(cit->second.points.size());
          unsigned index = 0;
          for (std::set<ptr_t>::const_iterator it = 
                cit->second.points.begin(); it != 
                cit->second.points.end(); it++)
            points[index++] = Realm::Point<1,coord_t>(*it);
          const Realm::IndexSpace<1,coord_t> space(points);
          domains[color] = DomainT<1,coord_t>(space);
        }
        else
        {
          std::vector<Realm::Rect<1,coord_t> >
            ranges(cit->second.points.size() + cit->second.ranges.size());
          unsigned index = 0;
          for (std::set<ptr_t>::const_iterator it = 
                cit->second.points.begin(); it != 
                cit->second.points.end(); it++)
          {
            Realm::Point<1,coord_t> point(*it);
            ranges[index++] = Realm::Rect<1,coord_t>(point, point);
          }
          for (std::set<std::pair<ptr_t,ptr_t> >::iterator it = 
                cit->second.ranges.begin(); it !=
                cit->second.ranges.end(); it++)
          {
            Realm::Point<1,coord_t> lo(it->first);
            Realm::Point<1,coord_t> hi(it->second);
            ranges[index++] = Realm::Rect<1,coord_t>(lo, hi);
          }
          const Realm::IndexSpace<1,coord_t> space(ranges);
          domains[color] = DomainT<1,coord_t>(space);
        }
      }
#ifdef DEBUG_LEGION
      assert(lower_bound <= upper_bound);
#endif
      // Make the color space
      Rect<1,coord_t> 
        color_space((Point<1,coord_t>(lower_bound)),
                    (Point<1,coord_t>(upper_bound)));
      // Make an index space for the color space
      IndexSpaceT<1,coord_t> index_color_space = 
                                  create_index_space(ctx, color_space);
      IndexPartition result = create_partition_by_domain(ctx, parent, domains,
          index_color_space, true/*perform intersections*/,
          (disjoint ? LEGION_DISJOINT_KIND : LEGION_ALIASED_KIND), part_color);
      return result;
    }

    //--------------------------------------------------------------------------
    IndexPartition Runtime::create_index_partition(Context ctx,
                                          IndexSpace parent, 
                                          const Domain &color_space,
                                          const DomainPointColoring &coloring,
                                          PartitionKind part_kind, Color color)
    //--------------------------------------------------------------------------
    {
      // Make an index space for the color space
      IndexSpace index_color_space = create_index_space(ctx, color_space);
      IndexPartition result = create_partition_by_domain(ctx, parent, coloring,
          index_color_space, true/*perform intersections*/, part_kind, color);
      return result;
    }

    //--------------------------------------------------------------------------
    IndexPartition Runtime::create_index_partition(
                                          Context ctx, IndexSpace parent,
                                          Domain color_space,
                                          const DomainColoring &coloring,
                                          bool disjoint, Color part_color)
    //--------------------------------------------------------------------------
    {
      std::map<DomainPoint,Domain> domains;
      for (DomainColoring::const_iterator it = 
            coloring.begin(); it != coloring.end(); it++)
      {
        Point<1,coord_t> color(it->first);
        domains[color] = it->second;
      }
      // Make an index space for the color space
      IndexSpace index_color_space = create_index_space(ctx, color_space);
      IndexPartition result = create_partition_by_domain(ctx, parent, domains,
          index_color_space, true/*perform intersections*/,
          (disjoint ? LEGION_DISJOINT_KIND : LEGION_ALIASED_KIND), part_color);
      return result;
    }

    //--------------------------------------------------------------------------
    IndexPartition Runtime::create_index_partition(Context ctx,
                                       IndexSpace parent,
                                       const Domain &color_space,
                                       const MultiDomainPointColoring &coloring,
                                       PartitionKind part_kind, Color color)
    //--------------------------------------------------------------------------
    {
      const int dim = parent.get_dim();
      std::map<DomainPoint,Domain> domains;
      Realm::ProfilingRequestSet no_reqs;
      switch (dim)
      {
#define DIMFUNC(DIM) \
        case DIM:                                                       \
          {                                                             \
            for (MultiDomainPointColoring::const_iterator cit =         \
                  coloring.begin(); cit != coloring.end(); cit++)       \
            {                                                           \
              std::vector<Realm::IndexSpace<DIM,coord_t> >              \
                  subspaces(cit->second.size());                        \
              unsigned index = 0;                                       \
              for (std::set<Domain>::const_iterator it =                \
                    cit->second.begin(); it != cit->second.end(); it++) \
              {                                                         \
                const DomainT<DIM,coord_t> domaint = *it;               \
                subspaces[index++] = domaint;                           \
              }                                                         \
              Realm::IndexSpace<DIM,coord_t> summary;                   \
              Internal::LgEvent wait_on(                                \
                  Realm::IndexSpace<DIM,coord_t>::compute_union(        \
                    subspaces, summary, no_reqs));                      \
              if (wait_on.exists())                                     \
                wait_on.wait();                                         \
              summary = summary.tighten();                              \
              domains[cit->first] = DomainT<DIM,coord_t>(summary);      \
            }                                                           \
            break;                                                      \
          }
        LEGION_FOREACH_N(DIMFUNC)
#undef DIMFUNC
        default:
          assert(false);
      }
      // Make an index space for the color space
      IndexSpace index_color_space = create_index_space(ctx, color_space);
      IndexPartition result = create_partition_by_domain(ctx, parent, domains,
        index_color_space, true/*perform intersections*/, part_kind, color);
      return result;
    }

    //--------------------------------------------------------------------------
    IndexPartition Runtime::create_index_partition(
                                          Context ctx, IndexSpace parent,
                                          Domain color_space,
                                          const MultiDomainColoring &coloring,
                                          bool disjoint, Color part_color)
    //--------------------------------------------------------------------------
    {
      const int dim = parent.get_dim();
      std::map<DomainPoint,Domain> domains;
      Realm::ProfilingRequestSet no_reqs;
      switch (dim)
      {
#define DIMFUNC(DIM) \
        case DIM:                                                       \
          {                                                             \
            for (MultiDomainColoring::const_iterator cit =              \
                  coloring.begin(); cit != coloring.end(); cit++)       \
            {                                                           \
              std::vector<Realm::IndexSpace<DIM,coord_t> >              \
                  subspaces(cit->second.size());                        \
              unsigned index = 0;                                       \
              for (std::set<Domain>::const_iterator it =                \
                    cit->second.begin(); it != cit->second.end(); it++) \
              {                                                         \
                const DomainT<DIM,coord_t> domaint = *it;               \
                subspaces[index++] = domaint;                           \
              }                                                         \
              Realm::IndexSpace<DIM,coord_t> summary;                   \
              Internal::LgEvent wait_on(                                \
                  Realm::IndexSpace<DIM,coord_t>::compute_union(        \
                    subspaces, summary, no_reqs));                      \
              const Point<1,coord_t> color(cit->first);                 \
              if (wait_on.exists())                                     \
                wait_on.wait();                                         \
              summary = summary.tighten();                              \
              domains[color] = DomainT<DIM,coord_t>(summary);           \
            }                                                           \
            break;                                                      \
          }
        LEGION_FOREACH_N(DIMFUNC)
#undef DIMFUNC
        default:
          assert(false);
      }
      // Make an index space for the color space
      IndexSpace index_color_space = create_index_space(ctx, color_space);
      IndexPartition result = create_partition_by_domain(ctx, parent, domains,
        index_color_space, true/*perform intersections*/, 
        (disjoint ? LEGION_DISJOINT_KIND : LEGION_ALIASED_KIND), part_color);
      return result;
    }

    //--------------------------------------------------------------------------
    IndexPartition Runtime::create_index_partition(
                                          Context ctx, IndexSpace parent,
    LegionRuntime::Accessor::RegionAccessor<
      LegionRuntime::Accessor::AccessorType::Generic> field_accessor,
                                                      Color part_color)
    //--------------------------------------------------------------------------
    {
      Internal::log_run.error("Call to deprecated 'create_index_partition' "
                    "method with an accessor in task %s (UID %lld) should be "
                    "replaced with a call to create_partition_by_field.",
                    ctx->get_task_name(), ctx->get_unique_id());
      assert(false);
      return IndexPartition::NO_PART;
    }

    //--------------------------------------------------------------------------
    void Runtime::create_shared_ownership(Context ctx, IndexPartition handle)
    //--------------------------------------------------------------------------
    {
      ctx->create_shared_ownership(handle);
    }

    //--------------------------------------------------------------------------
    void Runtime::destroy_index_partition(Context ctx, IndexPartition handle,
                     const bool unordered, const bool recurse, const char *prov)
    //--------------------------------------------------------------------------
    {
      Internal::AutoProvenance provenance(prov);
      ctx->destroy_index_partition(handle, unordered, recurse, provenance);
    }

    //--------------------------------------------------------------------------
    IndexPartition Runtime::create_equal_partition(Context ctx, 
                                                   IndexSpace parent,
                                                   IndexSpace color_space,
                                                   size_t granularity,
                                                   Color color,
                                                   const char *prov)
    //--------------------------------------------------------------------------
    {
      Internal::AutoProvenance provenance(prov);
      return ctx->create_equal_partition(parent, color_space, granularity,
                                         color, provenance);
    }

    //--------------------------------------------------------------------------
    IndexPartition Runtime::create_partition_by_weights(Context ctx,
                                       IndexSpace parent,
                                       const std::map<DomainPoint,int> &weights,
                                       IndexSpace color_space,
                                       size_t granularity, Color color,
                                       const char *prov)
    //--------------------------------------------------------------------------
    {
      std::map<DomainPoint,UntypedBuffer> data;
      for (std::map<DomainPoint,int>::const_iterator it = 
            weights.begin(); it != weights.end(); it++)
        data.emplace(std::make_pair(it->first, 
              UntypedBuffer(&it->second, sizeof(it->second))));
      FutureMap future_map = construct_future_map(ctx, color_space, data,
          false/*collective*/, 0/*sid*/, false/*implicit*/, prov);
      Internal::AutoProvenance provenance(prov);
      return ctx->create_partition_by_weights(parent, future_map, color_space,
                                              granularity, color, provenance);
    }

    //--------------------------------------------------------------------------
    IndexPartition Runtime::create_partition_by_weights(Context ctx,
                                    IndexSpace parent,
                                    const std::map<DomainPoint,size_t> &weights,
                                    IndexSpace color_space,
                                    size_t granularity, Color color,
                                    const char *prov)
    //--------------------------------------------------------------------------
    {
      std::map<DomainPoint,UntypedBuffer> data;
      for (std::map<DomainPoint,size_t>::const_iterator it = 
            weights.begin(); it != weights.end(); it++)
        data.emplace(std::make_pair(it->first, 
              UntypedBuffer(&it->second, sizeof(it->second))));
      FutureMap future_map = construct_future_map(ctx, color_space, data,
          false/*collective*/, 0/*sid*/, false/*implicit*/, prov);
      Internal::AutoProvenance provenance(prov);
      return ctx->create_partition_by_weights(parent, future_map, color_space,
                                              granularity, color, provenance);
    }

    //--------------------------------------------------------------------------
    IndexPartition Runtime::create_partition_by_weights(Context ctx,
                                                IndexSpace parent,
                                                const FutureMap &weights,
                                                IndexSpace color_space,
                                                size_t granularity, Color color,
                                                const char *prov)
    //--------------------------------------------------------------------------
    {
      Internal::AutoProvenance provenance(prov);
      return ctx->create_partition_by_weights(parent, weights, color_space, 
                                              granularity, color, provenance);
    }

    //--------------------------------------------------------------------------
    IndexPartition Runtime::create_partition_by_union(Context ctx,
                                    IndexSpace parent, IndexPartition handle1,
                                    IndexPartition handle2, 
                                    IndexSpace color_space, PartitionKind kind,
                                    Color color, const char *prov)
    //--------------------------------------------------------------------------
    {
      Internal::AutoProvenance provenance(prov);
      return ctx->create_partition_by_union(parent, handle1, handle2, 
                                color_space, kind, color, provenance);
    }

    //--------------------------------------------------------------------------
    IndexPartition Runtime::create_partition_by_intersection(
                                                Context ctx, IndexSpace parent,
                                                IndexPartition handle1, 
                                                IndexPartition handle2,
                                                IndexSpace color_space,
                                                PartitionKind kind, Color color,
                                                const char *prov) 
    //--------------------------------------------------------------------------
    {
      Internal::AutoProvenance provenance(prov);
      return ctx->create_partition_by_intersection(parent, handle1, handle2, 
                                       color_space, kind, color, provenance);
    }

    //--------------------------------------------------------------------------
    IndexPartition Runtime::create_partition_by_intersection(Context ctx,
                           IndexSpace parent, IndexPartition partition,
                           PartitionKind part_kind, Color color, 
                           bool dominates, const char *prov)
    //--------------------------------------------------------------------------
    {
      Internal::AutoProvenance provenance(prov);
      return ctx->create_partition_by_intersection(parent, partition, part_kind,
                                                   color, dominates,provenance);
    }

    //--------------------------------------------------------------------------
    IndexPartition Runtime::create_partition_by_difference(
                                                Context ctx, IndexSpace parent,
                                                IndexPartition handle1,
                                                IndexPartition handle2,
                                                IndexSpace color_space,
                                                PartitionKind kind, Color color,
                                                const char *prov)
    //--------------------------------------------------------------------------
    {
      Internal::AutoProvenance provenance(prov);
      return ctx->create_partition_by_difference(parent, handle1, handle2, 
                                     color_space, kind, color, provenance);
    }

    //--------------------------------------------------------------------------
    Color Runtime::create_cross_product_partitions(Context ctx,
                              IndexPartition handle1, IndexPartition handle2,
                              std::map<IndexSpace,IndexPartition> &handles,
                              PartitionKind kind, Color color, const char *prov)
    //--------------------------------------------------------------------------
    {
      Internal::AutoProvenance provenance(prov);
      return ctx->create_cross_product_partitions(handle1, handle2, handles, 
                                                  kind, color, provenance);
    }

    //--------------------------------------------------------------------------
    void Runtime::create_association(Context ctx,
                                     LogicalRegion domain,
                                     LogicalRegion domain_parent,
                                     FieldID domain_fid,
                                     IndexSpace range,
                                     MapperID id, MappingTagID tag,
                                     UntypedBuffer marg, const char *prov)
    //--------------------------------------------------------------------------
    {
      Internal::AutoProvenance provenance(prov);
      ctx->create_association(domain, domain_parent, domain_fid, range,
                              id, tag, marg, provenance);
    }

    //--------------------------------------------------------------------------
    void Runtime::create_bidirectional_association(Context ctx,
                                      LogicalRegion domain,
                                      LogicalRegion domain_parent,
                                      FieldID domain_fid,
                                      LogicalRegion range,
                                      LogicalRegion range_parent,
                                      FieldID range_fid,
                                      MapperID id, MappingTagID tag,
                                      UntypedBuffer marg,const char *provenance)
    //--------------------------------------------------------------------------
    {
      // Realm guarantees that creating association in either direction
      // will produce the same result, so we can do these separately
      create_association(ctx, domain, domain_parent, domain_fid, 
                         range.get_index_space(), id, tag, marg, provenance);
      create_association(ctx, range, range_parent, range_fid, 
                         domain.get_index_space(), id, tag, marg, provenance);
    }

    //--------------------------------------------------------------------------
    IndexPartition Runtime::create_partition_by_restriction(Context ctx,
                                                        IndexSpace par,
                                                        IndexSpace cs,
                                                        DomainTransform tran,
                                                        Domain ext,
                                                        PartitionKind part_kind,
                                                        Color color,
                                                        const char *provenance)
    //--------------------------------------------------------------------------
    {
      switch ((ext.get_dim()-1) * LEGION_MAX_DIM + (tran.n-1))
      {
#define DIMFUNC(D1,D2) \
        case (D1-1)*LEGION_MAX_DIM+(D2-1): \
          { \
            const IndexSpaceT<D1,coord_t> parent(par); \
            const Rect<D1,coord_t> extent(ext); \
            const Transform<D1,D2> transform(tran); \
            const IndexSpaceT<D2,coord_t> color_space(cs); \
            return create_partition_by_restriction<D1,D2,coord_t>(ctx, parent, \
              color_space, transform, extent, part_kind, color, provenance); \
          }
        LEGION_FOREACH_NN(DIMFUNC)
#undef DIMFUNC
      }
      return IndexPartition::NO_PART;
    }

    //--------------------------------------------------------------------------
    IndexPartition Runtime::create_partition_by_blockify(Context ctx,
                                                         IndexSpace par,
                                                         DomainPoint bf,
                                                         Color color,
                                                         const char *provenance)
    //--------------------------------------------------------------------------
    {
      switch (bf.get_dim())
      {
#define DIMFUNC(DIM) \
        case DIM: \
          { \
            const IndexSpaceT<DIM,coord_t> parent(par); \
            const Point<DIM,coord_t> blocking_factor(bf); \
            return create_partition_by_blockify<DIM,coord_t>(ctx, parent, \
                                      blocking_factor, color, provenance); \
          }
        LEGION_FOREACH_N(DIMFUNC)
#undef DIMFUNC
        default:
          assert(false);
      }
      return IndexPartition::NO_PART;
    }

    //--------------------------------------------------------------------------
    IndexPartition Runtime::create_partition_by_blockify(Context ctx,
                                                         IndexSpace par,
                                                         DomainPoint bf,
                                                         DomainPoint orig,
                                                         Color color,
                                                         const char *provenance)
    //--------------------------------------------------------------------------
    {
      switch (bf.get_dim())
      {
#define DIMFUNC(DIM) \
        case DIM: \
          { \
            const IndexSpaceT<DIM,coord_t> parent(par); \
            const Point<DIM,coord_t> blocking_factor(bf); \
            const Point<DIM,coord_t> origin(orig); \
            return create_partition_by_blockify<DIM,coord_t>(ctx, parent, \
                              blocking_factor, origin, color, provenance); \
          }
        LEGION_FOREACH_N(DIMFUNC)
#undef DIMFUNC
        default:
          assert(false);
      }
      return IndexPartition::NO_PART;
    }

    //--------------------------------------------------------------------------
    IndexPartition Runtime::create_restricted_partition(Context ctx,
                                                        IndexSpace parent, 
                                                        IndexSpace color_space,
                                                        const void *transform,
                                                        size_t transform_size,
                                                        const void *extent, 
                                                        size_t extent_size,
                                                        PartitionKind part_kind,
                                                        Color color,
                                                        const char *prov)
    //--------------------------------------------------------------------------
    {
      Internal::AutoProvenance provenance(prov);
      return ctx->create_restricted_partition(parent, color_space, transform, 
            transform_size, extent, extent_size, part_kind, color, provenance);
    }

    //--------------------------------------------------------------------------
    IndexPartition Runtime::create_partition_by_domain(Context ctx,
                 IndexSpace parent, const std::map<DomainPoint,Domain> &domains,
                 IndexSpace color_space, bool perform_intersections,
                 PartitionKind part_kind, Color color, const char *prov)
    //--------------------------------------------------------------------------
    {
      Internal::AutoProvenance provenance(prov);
      return ctx->create_partition_by_domain(parent, domains, color_space,
                      perform_intersections, part_kind, color, provenance);
    }

    //--------------------------------------------------------------------------
    IndexPartition Runtime::create_partition_by_domain(Context ctx,
                         IndexSpace parent, const FutureMap &domains,
                         IndexSpace color_space, bool perform_intersections,
                         PartitionKind part_kind, Color color, const char *prov)
    //--------------------------------------------------------------------------
    {
      Internal::AutoProvenance provenance(prov);
      return ctx->create_partition_by_domain(parent, domains, color_space, 
                           perform_intersections, part_kind, color, provenance);
    }

    //--------------------------------------------------------------------------
    IndexPartition Runtime::create_partition_by_field(Context ctx,
                  LogicalRegion handle, LogicalRegion parent, FieldID fid, 
                  IndexSpace color_space, Color color, MapperID id,
                  MappingTagID tag, PartitionKind part_kind, 
                  UntypedBuffer marg, const char *prov)
    //--------------------------------------------------------------------------
    {
      Internal::AutoProvenance provenance(prov);
      return ctx->create_partition_by_field(handle, parent, fid, color_space, 
                                color, id, tag, part_kind, marg, provenance);
    }

    //--------------------------------------------------------------------------
    IndexPartition Runtime::create_partition_by_image(Context ctx,
                  IndexSpace handle, LogicalPartition projection,
                  LogicalRegion parent, FieldID fid, IndexSpace color_space,
                  PartitionKind part_kind, Color color, MapperID id,
                  MappingTagID tag, UntypedBuffer marg, const char *prov)
    //--------------------------------------------------------------------------
    {
      Internal::AutoProvenance provenance(prov);
      return ctx->create_partition_by_image(handle, projection, parent, fid, 
                  color_space, part_kind, color, id, tag, marg, provenance);
    }

    //--------------------------------------------------------------------------
    IndexPartition Runtime::create_partition_by_image_range(Context ctx,
                  IndexSpace handle, LogicalPartition projection,
                  LogicalRegion parent, FieldID fid, IndexSpace color_space,
                  PartitionKind part_kind, Color color, MapperID id,
                  MappingTagID tag, UntypedBuffer marg, const char *prov)
    //--------------------------------------------------------------------------
    {
      Internal::AutoProvenance provenance(prov);
      return ctx->create_partition_by_image_range(handle, projection, parent, 
              fid, color_space, part_kind, color, id, tag, marg, provenance);
    }

    //--------------------------------------------------------------------------
    IndexPartition Runtime::create_partition_by_preimage(Context ctx,
                  IndexPartition projection, LogicalRegion handle,
                  LogicalRegion parent, FieldID fid, IndexSpace color_space,
                  PartitionKind part_kind, Color color, MapperID id, 
                  MappingTagID tag, UntypedBuffer marg, const char *prov)
    //--------------------------------------------------------------------------
    {
      Internal::AutoProvenance provenance(prov);
      return ctx->create_partition_by_preimage(projection, handle, parent,
            fid, color_space, part_kind, color, id, tag, marg, provenance);
    }

    //--------------------------------------------------------------------------
    IndexPartition Runtime::create_partition_by_preimage_range(Context ctx,
                  IndexPartition projection, LogicalRegion handle,
                  LogicalRegion parent, FieldID fid, IndexSpace color_space,
                  PartitionKind part_kind, Color color, MapperID id,
                  MappingTagID tag, UntypedBuffer marg, const char *prov)
    //--------------------------------------------------------------------------
    {
      Internal::AutoProvenance provenance(prov);
      return ctx->create_partition_by_preimage_range(projection, handle, parent,
                 fid, color_space, part_kind, color, id, tag, marg, provenance);
    } 

    //--------------------------------------------------------------------------
    IndexPartition Runtime::create_pending_partition(Context ctx,
                             IndexSpace parent, IndexSpace color_space, 
                             PartitionKind part_kind, Color color,
                             const char *prov)
    //--------------------------------------------------------------------------
    {
      Internal::AutoProvenance provenance(prov);
      return ctx->create_pending_partition(parent, color_space,
                                           part_kind, color, provenance);
    }

    //--------------------------------------------------------------------------
    IndexSpace Runtime::create_index_space_union(Context ctx,
                      IndexPartition parent, const DomainPoint &color,
                      const std::vector<IndexSpace> &handles, const char *prov) 
    //--------------------------------------------------------------------------
    {
      Internal::AutoProvenance provenance(prov);
      switch (color.get_dim())
      {
#define DIMFUNC(DIM) \
        case DIM: \
          { \
            Point<DIM,coord_t> point = color; \
            return ctx->create_index_space_union(parent, &point, sizeof(point),\
                                      TYPE_TAG_##DIM##D, handles, provenance); \
          }
        LEGION_FOREACH_N(DIMFUNC)
#undef DIMFUNC
        default:
          assert(false);
      }
      return IndexSpace::NO_SPACE;
    }

    //--------------------------------------------------------------------------
    IndexSpace Runtime::create_index_space_union_internal(Context ctx,
                    IndexPartition parent, const void *color, size_t color_size,
                    TypeTag type_tag, const char *prov,
                    const std::vector<IndexSpace> &handles)
    //--------------------------------------------------------------------------
    {
      Internal::AutoProvenance provenance(prov);
      return ctx->create_index_space_union(parent, color, color_size, 
                                           type_tag, handles, provenance);
    }

    //--------------------------------------------------------------------------
    IndexSpace Runtime::create_index_space_union(Context ctx,
                                IndexPartition parent, const DomainPoint &color,
                                IndexPartition handle, const char *prov)
    //--------------------------------------------------------------------------
    {
      Internal::AutoProvenance provenance(prov);
      switch (color.get_dim())
      {
#define DIMFUNC(DIM) \
        case DIM: \
          { \
            Point<DIM,coord_t> point = color; \
            return ctx->create_index_space_union(parent, &point, sizeof(point),\
                                       TYPE_TAG_##DIM##D, handle, provenance); \
          }
        LEGION_FOREACH_N(DIMFUNC)
#undef DIMFUNC
        default:
          assert(false);
      }
      return IndexSpace::NO_SPACE;
    }

    //--------------------------------------------------------------------------
    IndexSpace Runtime::create_index_space_union_internal(Context ctx,
                        IndexPartition parent, const void *realm_color, 
                        size_t size, TypeTag type_tag,
                        const char *prov, IndexPartition handle)
    //--------------------------------------------------------------------------
    {
      Internal::AutoProvenance provenance(prov);
      return ctx->create_index_space_union(parent, realm_color, size,
                                           type_tag, handle, provenance);
    }

    //--------------------------------------------------------------------------
    IndexSpace Runtime::create_index_space_intersection(Context ctx,
                      IndexPartition parent, const DomainPoint &color,
                      const std::vector<IndexSpace> &handles,
                      const char *prov) 
    //--------------------------------------------------------------------------
    {
      Internal::AutoProvenance provenance(prov);
      switch (color.get_dim())
      {
#define DIMFUNC(DIM) \
      case DIM: \
        { \
          Point<DIM,coord_t> point = color; \
          return ctx->create_index_space_intersection(parent, &point, \
              sizeof(point), TYPE_TAG_##DIM##D, handles, provenance); \
        }
        LEGION_FOREACH_N(DIMFUNC)
#undef DIMFUNC
        default:
          assert(false);
      }
      return IndexSpace::NO_SPACE;
    }

    //--------------------------------------------------------------------------
    IndexSpace Runtime::create_index_space_intersection_internal(Context ctx,
                    IndexPartition parent, const void *color, size_t color_size,
                    TypeTag type_tag, const char *prov,
                    const std::vector<IndexSpace> &handles)
    //--------------------------------------------------------------------------
    {
      Internal::AutoProvenance provenance(prov);
      return ctx->create_index_space_intersection(parent, color, color_size, 
                                              type_tag, handles, provenance);
    }

    //--------------------------------------------------------------------------
    IndexSpace Runtime::create_index_space_intersection(Context ctx,
                      IndexPartition parent, const DomainPoint &color,
                      IndexPartition handle, const char *prov)
    //--------------------------------------------------------------------------
    {
      Internal::AutoProvenance provenance(prov);
      switch (color.get_dim())
      {
#define DIMFUNC(DIM) \
      case DIM: \
        { \
          Point<DIM,coord_t> point = color; \
          return ctx->create_index_space_intersection(parent, &point, \
                sizeof(point), TYPE_TAG_##DIM##D, handle, provenance); \
        }
        LEGION_FOREACH_N(DIMFUNC)
#undef DIMFUNC
        default:
          assert(false);
      }
      return IndexSpace::NO_SPACE;
    }

    //--------------------------------------------------------------------------
    IndexSpace Runtime::create_index_space_intersection_internal(Context ctx,
                     IndexPartition parent, const void *realm_color, 
                     size_t color_size, TypeTag type_tag,
                     const char *prov, IndexPartition handle)
    //--------------------------------------------------------------------------
    {
      Internal::AutoProvenance provenance(prov);
      return ctx->create_index_space_intersection(parent, realm_color, 
                            color_size, type_tag, handle, provenance);
    }

    //--------------------------------------------------------------------------
    IndexSpace Runtime::create_index_space_difference(Context ctx,
          IndexPartition parent, const DomainPoint &color, IndexSpace initial, 
          const std::vector<IndexSpace> &handles, const char *prov)
    //--------------------------------------------------------------------------
    {
      Internal::AutoProvenance provenance(prov);
      switch (color.get_dim())
      {
#define DIMFUNC(DIM) \
      case DIM: \
        { \
          Point<DIM,coord_t> point = color; \
          return ctx->create_index_space_difference(parent, &point, \
            sizeof(point), TYPE_TAG_##DIM##D, initial, handles, provenance); \
        }
        LEGION_FOREACH_N(DIMFUNC)
#undef DIMFUNC
        default:
          assert(false);
      }
      return IndexSpace::NO_SPACE;
    }

    //--------------------------------------------------------------------------
    IndexSpace Runtime::create_index_space_difference_internal(Context ctx,
        IndexPartition parent, const void *realm_color, size_t color_size,
        TypeTag type_tag, const char *prov, IndexSpace initial, 
        const std::vector<IndexSpace> &handles)
    //--------------------------------------------------------------------------
    {
      Internal::AutoProvenance provenance(prov);
      return ctx->create_index_space_difference(parent, realm_color, color_size,
                                        type_tag, initial, handles, provenance);
    }

    //--------------------------------------------------------------------------
    IndexPartition Runtime::get_index_partition(Context ctx, 
                                                IndexSpace parent, Color color)
    //--------------------------------------------------------------------------
    {
      return runtime->get_index_partition(ctx, parent, color);
    }

    //--------------------------------------------------------------------------
    IndexPartition Runtime::get_index_partition(Context ctx,
                                    IndexSpace parent, const DomainPoint &color)
    //--------------------------------------------------------------------------
    {
      return get_index_partition(ctx, parent, color.get_color());
    }

    //--------------------------------------------------------------------------
    IndexPartition Runtime::get_index_partition(IndexSpace parent, Color color)
    //--------------------------------------------------------------------------
    {
      return runtime->get_index_partition(parent, color);
    }

    //--------------------------------------------------------------------------
    IndexPartition Runtime::get_index_partition(IndexSpace parent,
                                                const DomainPoint &color)
    //--------------------------------------------------------------------------
    {
      return get_index_partition(parent, color.get_color());
    }

    //--------------------------------------------------------------------------
    bool Runtime::has_index_partition(Context ctx, IndexSpace parent, Color c)
    //--------------------------------------------------------------------------
    {
      return runtime->has_index_partition(ctx, parent, c);
    }

    //--------------------------------------------------------------------------
    bool Runtime::has_index_partition(Context ctx, IndexSpace parent,
                                               const DomainPoint &color)
    //--------------------------------------------------------------------------
    {
      return runtime->has_index_partition(ctx, parent, color.get_color());
    }

    //--------------------------------------------------------------------------
    bool Runtime::has_index_partition(IndexSpace parent, Color c)
    //--------------------------------------------------------------------------
    {
      return runtime->has_index_partition(parent, c);
    }

    //--------------------------------------------------------------------------
    bool Runtime::has_index_partition(IndexSpace parent,
                                      const DomainPoint &color)
    //--------------------------------------------------------------------------
    {
      return runtime->has_index_partition(parent, color.get_color());
    }

    //--------------------------------------------------------------------------
    IndexSpace Runtime::get_index_subspace(Context ctx, 
                                                  IndexPartition p, Color color)
    //--------------------------------------------------------------------------
    {
      Point<1,coord_t> point = color;
      return runtime->get_index_subspace(ctx, p, &point, TYPE_TAG_1D);
    }

    //--------------------------------------------------------------------------
    IndexSpace Runtime::get_index_subspace(Context ctx,
                                     IndexPartition p, const DomainPoint &color)
    //--------------------------------------------------------------------------
    {
      switch (color.get_dim())
      {
#define DIMFUNC(DIM) \
    case DIM: \
      { \
        Point<DIM,coord_t> point = color; \
        return runtime->get_index_subspace(ctx, p, &point, TYPE_TAG_##DIM##D); \
      }
        LEGION_FOREACH_N(DIMFUNC)
#undef DIMFUNC
        default:
          assert(false);
      }
      return IndexSpace::NO_SPACE;
    }

    //--------------------------------------------------------------------------
    IndexSpace Runtime::get_index_subspace(IndexPartition p, Color color)
    //--------------------------------------------------------------------------
    {
      Point<1,coord_t> point = color;
      return runtime->get_index_subspace(p, &point, TYPE_TAG_1D);
    }

    //--------------------------------------------------------------------------
    IndexSpace Runtime::get_index_subspace(IndexPartition p, 
                                           const DomainPoint &color)
    //--------------------------------------------------------------------------
    {
      switch (color.get_dim())
      {
#define DIMFUNC(DIM) \
        case DIM: \
          { \
            Point<DIM,coord_t> point = color; \
            return runtime->get_index_subspace(p, &point, TYPE_TAG_##DIM##D); \
          }
        LEGION_FOREACH_N(DIMFUNC)
#undef DIMFUNC
        default:
          assert(false);
      }
      return IndexSpace::NO_SPACE;
    }

    //--------------------------------------------------------------------------
    IndexSpace Runtime::get_index_subspace_internal(IndexPartition p,
                                      const void *realm_color, TypeTag type_tag)
    //--------------------------------------------------------------------------
    {
      return runtime->get_index_subspace(p, realm_color, type_tag);
    }

    //--------------------------------------------------------------------------
    bool Runtime::has_index_subspace(Context ctx, 
                                     IndexPartition p, const DomainPoint &color)
    //--------------------------------------------------------------------------
    {
      switch (color.get_dim())
      {
#define DIMFUNC(DIM) \
    case DIM: \
      { \
        Point<DIM,coord_t> point = color; \
        return runtime->has_index_subspace(ctx, p, &point, TYPE_TAG_##DIM##D); \
      }
        LEGION_FOREACH_N(DIMFUNC)
#undef DIMFUNC
        default:
          assert(false);
      }
      return false;
    }

    //--------------------------------------------------------------------------
    bool Runtime::has_index_subspace(IndexPartition p, const DomainPoint &color)
    //--------------------------------------------------------------------------
    {
      switch (color.get_dim())
      {
#define DIMFUNC(DIM) \
        case DIM: \
          { \
            Point<DIM,coord_t> point = color; \
            return runtime->has_index_subspace(p, &point, TYPE_TAG_##DIM##D); \
          }
        LEGION_FOREACH_N(DIMFUNC)
#undef DIMFUNC
        default:
          assert(false);
      }
      return false;
    }

    //--------------------------------------------------------------------------
    bool Runtime::has_index_subspace_internal(IndexPartition p,
                                      const void *realm_color, TypeTag type_tag)
    //--------------------------------------------------------------------------
    {
      return runtime->has_index_subspace(p, realm_color, type_tag);
    }

    //--------------------------------------------------------------------------
    bool Runtime::has_multiple_domains(Context ctx, IndexSpace handle)
    //--------------------------------------------------------------------------
    {
      // Multiple domains supported implicitly
      return false;
    }

    //--------------------------------------------------------------------------
    bool Runtime::has_multiple_domains(IndexSpace handle)
    //--------------------------------------------------------------------------
    {
      // Multiple domains supported implicitly
      return false;
    }

    //--------------------------------------------------------------------------
    Domain Runtime::get_index_space_domain(Context ctx, IndexSpace handle)
    //--------------------------------------------------------------------------
    {
      const TypeTag type_tag = handle.get_type_tag();
      switch (Internal::NT_TemplateHelper::get_dim(type_tag))
      {
#define DIMFUNC(DIM) \
        case DIM: \
          { \
            DomainT<DIM,coord_t> realm_is; \
            runtime->get_index_space_domain(ctx, handle, &realm_is, type_tag); \
            return Domain(realm_is); \
          }
        LEGION_FOREACH_N(DIMFUNC)
#undef DIMFUNC
        default:
          assert(false);
      }
      return Domain::NO_DOMAIN;
    }

    //--------------------------------------------------------------------------
    Domain Runtime::get_index_space_domain(IndexSpace handle)
    //--------------------------------------------------------------------------
    {
      const TypeTag type_tag = handle.get_type_tag();
      switch (Internal::NT_TemplateHelper::get_dim(type_tag))
      {
#define DIMFUNC(DIM) \
        case DIM: \
          { \
            DomainT<DIM,coord_t> realm_is; \
            runtime->get_index_space_domain(handle, &realm_is, type_tag); \
            return Domain(realm_is); \
          }
        LEGION_FOREACH_N(DIMFUNC)
#undef DIMFUNC
        default:
          assert(false);
      }
      return Domain::NO_DOMAIN;
    }

    //--------------------------------------------------------------------------
    void Runtime::get_index_space_domain_internal(IndexSpace handle,
                                         void *realm_is, TypeTag type_tag)
    //--------------------------------------------------------------------------
    {
      runtime->get_index_space_domain(handle, realm_is, type_tag);
    }

    //--------------------------------------------------------------------------
    void Runtime::get_index_space_domains(Context ctx, 
                                IndexSpace handle, std::vector<Domain> &domains)
    //--------------------------------------------------------------------------
    {
      domains.push_back(get_index_space_domain(ctx, handle));
    }

    //--------------------------------------------------------------------------
    void Runtime::get_index_space_domains(IndexSpace handle,
                                          std::vector<Domain> &domains)
    //--------------------------------------------------------------------------
    {
      domains.push_back(get_index_space_domain(handle));
    }

    //--------------------------------------------------------------------------
    Domain Runtime::get_index_partition_color_space(Context ctx, 
                                                    IndexPartition p)
    //--------------------------------------------------------------------------
    {
      return runtime->get_index_partition_color_space(ctx, p);
    }

    //--------------------------------------------------------------------------
    Domain Runtime::get_index_partition_color_space(IndexPartition p)
    //--------------------------------------------------------------------------
    {
      return runtime->get_index_partition_color_space(p);
    }

    //--------------------------------------------------------------------------
    void Runtime::get_index_partition_color_space_internal(IndexPartition p,
                                               void *realm_is, TypeTag type_tag)
    //--------------------------------------------------------------------------
    {
      runtime->get_index_partition_color_space(p, realm_is, type_tag);
    }

    //--------------------------------------------------------------------------
    IndexSpace Runtime::get_index_partition_color_space_name(Context ctx,
                                                             IndexPartition p)
    //--------------------------------------------------------------------------
    {
      return runtime->get_index_partition_color_space_name(ctx, p);
    }

    //--------------------------------------------------------------------------
    IndexSpace Runtime::get_index_partition_color_space_name(IndexPartition p)
    //--------------------------------------------------------------------------
    {
      return runtime->get_index_partition_color_space_name(p);
    }

    //--------------------------------------------------------------------------
    void Runtime::get_index_space_partition_colors(Context ctx, IndexSpace sp,
                                                        std::set<Color> &colors)
    //--------------------------------------------------------------------------
    {
      runtime->get_index_space_partition_colors(ctx, sp, colors);
    }

    //--------------------------------------------------------------------------
    void Runtime::get_index_space_partition_colors(Context ctx, IndexSpace sp,
                                                  std::set<DomainPoint> &colors)
    //--------------------------------------------------------------------------
    {
      std::set<Color> temp_colors;
      runtime->get_index_space_partition_colors(ctx, sp, temp_colors);
      for (std::set<Color>::const_iterator it = temp_colors.begin();
            it != temp_colors.end(); it++)
        colors.insert(DomainPoint(*it));
    }
    
    //--------------------------------------------------------------------------
    void Runtime::get_index_space_partition_colors(IndexSpace sp,
                                                   std::set<Color> &colors)
    //--------------------------------------------------------------------------
    {
      runtime->get_index_space_partition_colors(sp, colors);
    }

    //--------------------------------------------------------------------------
    void Runtime::get_index_space_partition_colors(IndexSpace sp,
                                                  std::set<DomainPoint> &colors)
    //--------------------------------------------------------------------------
    {
      std::set<Color> temp_colors;
      runtime->get_index_space_partition_colors(sp, temp_colors);
      for (std::set<Color>::const_iterator it = temp_colors.begin();
            it != temp_colors.end(); it++)
        colors.insert(DomainPoint(*it));
    }

    //--------------------------------------------------------------------------
    bool Runtime::is_index_partition_disjoint(Context ctx, IndexPartition p)
    //--------------------------------------------------------------------------
    {
      return runtime->is_index_partition_disjoint(ctx, p);
    }

    //--------------------------------------------------------------------------
    bool Runtime::is_index_partition_disjoint(IndexPartition p)
    //--------------------------------------------------------------------------
    {
      return runtime->is_index_partition_disjoint(p);
    }

    //--------------------------------------------------------------------------
    bool Runtime::is_index_partition_complete(Context ctx, IndexPartition p)
    //--------------------------------------------------------------------------
    {
      return runtime->is_index_partition_complete(ctx, p);
    }

    //--------------------------------------------------------------------------
    bool Runtime::is_index_partition_complete(IndexPartition p)
    //--------------------------------------------------------------------------
    {
      return runtime->is_index_partition_complete(p);
    }

    //--------------------------------------------------------------------------
    Color Runtime::get_index_space_color(Context ctx, 
                                                  IndexSpace handle)
    //--------------------------------------------------------------------------
    {
      Point<1,coord_t> point;
      runtime->get_index_space_color_point(ctx, handle, &point, TYPE_TAG_1D);
      return point[0];
    }

    //--------------------------------------------------------------------------
    Color Runtime::get_index_space_color(IndexSpace handle)
    //--------------------------------------------------------------------------
    {
      Point<1,coord_t> point;
      runtime->get_index_space_color_point(handle, &point, TYPE_TAG_1D);
      return point[0];
    }

    //--------------------------------------------------------------------------
    DomainPoint Runtime::get_index_space_color_point(Context ctx,
                                                              IndexSpace handle)
    //--------------------------------------------------------------------------
    {
      return runtime->get_index_space_color_point(ctx, handle); 
    }

    //--------------------------------------------------------------------------
    DomainPoint Runtime::get_index_space_color_point(IndexSpace handle)
    //--------------------------------------------------------------------------
    {
      return runtime->get_index_space_color_point(handle);
    }

    //--------------------------------------------------------------------------
    void Runtime::get_index_space_color_internal(IndexSpace handle,
                                            void *realm_color, TypeTag type_tag)
    //--------------------------------------------------------------------------
    {
      runtime->get_index_space_color_point(handle, realm_color, type_tag);
    }

    //--------------------------------------------------------------------------
    Color Runtime::get_index_partition_color(Context ctx,
                                                      IndexPartition handle)
    //--------------------------------------------------------------------------
    {
      return runtime->get_index_partition_color(ctx, handle);
    }

    //--------------------------------------------------------------------------
    Color Runtime::get_index_partition_color(IndexPartition handle)
    //--------------------------------------------------------------------------
    {
      return runtime->get_index_partition_color(handle);
    }

    //--------------------------------------------------------------------------
    DomainPoint Runtime::get_index_partition_color_point(Context ctx,
                                                          IndexPartition handle)
    //--------------------------------------------------------------------------
    {
      return DomainPoint(runtime->get_index_partition_color(ctx, handle));
    }
    
    //--------------------------------------------------------------------------
    DomainPoint Runtime::get_index_partition_color_point(IndexPartition handle)
    //--------------------------------------------------------------------------
    {
      return DomainPoint(runtime->get_index_partition_color(handle));
    }

    //--------------------------------------------------------------------------
    IndexSpace Runtime::get_parent_index_space(Context ctx,
                                                        IndexPartition handle)
    //--------------------------------------------------------------------------
    {
      return runtime->get_parent_index_space(ctx, handle);
    }

    //--------------------------------------------------------------------------
    IndexSpace Runtime::get_parent_index_space(IndexPartition handle)
    //--------------------------------------------------------------------------
    {
      return runtime->get_parent_index_space(handle);
    }

    //--------------------------------------------------------------------------
    bool Runtime::has_parent_index_partition(Context ctx,
                                                      IndexSpace handle)
    //--------------------------------------------------------------------------
    {
      return runtime->has_parent_index_partition(ctx, handle);
    }

    //--------------------------------------------------------------------------
    bool Runtime::has_parent_index_partition(IndexSpace handle)
    //--------------------------------------------------------------------------
    {
      return runtime->has_parent_index_partition(handle);
    }

    //--------------------------------------------------------------------------
    IndexPartition Runtime::get_parent_index_partition(Context ctx,
                                                              IndexSpace handle)
    //--------------------------------------------------------------------------
    {
      return runtime->get_parent_index_partition(ctx, handle);
    }

    //--------------------------------------------------------------------------
    IndexPartition Runtime::get_parent_index_partition(IndexSpace handle)
    //--------------------------------------------------------------------------
    {
      return runtime->get_parent_index_partition(handle);
    }

    //--------------------------------------------------------------------------
    unsigned Runtime::get_index_space_depth(Context ctx, IndexSpace handle)
    //--------------------------------------------------------------------------
    {
      return runtime->get_index_space_depth(ctx, handle);
    }

    //--------------------------------------------------------------------------
    unsigned Runtime::get_index_space_depth(IndexSpace handle)
    //--------------------------------------------------------------------------
    {
      return runtime->get_index_space_depth(handle);
    }

    //--------------------------------------------------------------------------
    unsigned Runtime::get_index_partition_depth(Context ctx,  
                                                IndexPartition handle)
    //--------------------------------------------------------------------------
    {
      return runtime->get_index_partition_depth(ctx, handle);
    }

    //--------------------------------------------------------------------------
    unsigned Runtime::get_index_partition_depth(IndexPartition handle)
    //--------------------------------------------------------------------------
    {
      return runtime->get_index_partition_depth(handle);
    }

    //--------------------------------------------------------------------------
    ptr_t Runtime::safe_cast(Context ctx, ptr_t pointer, 
                                      LogicalRegion region)
    //--------------------------------------------------------------------------
    {
      if (pointer.is_null())
        return pointer;
      Point<1,coord_t> p(pointer.value);
      if (runtime->safe_cast(ctx, region, &p, TYPE_TAG_1D))
        return pointer;
      return ptr_t::nil();
    }

    //--------------------------------------------------------------------------
    DomainPoint Runtime::safe_cast(Context ctx, DomainPoint point, 
                                            LogicalRegion region)
    //--------------------------------------------------------------------------
    {
      switch (point.get_dim())
      {
#define DIMFUNC(DIM) \
        case DIM: \
          { \
            Point<DIM,coord_t> p(point); \
            if (runtime->safe_cast(ctx, region, &p, TYPE_TAG_##DIM##D)) \
              return point; \
            break; \
          }
        LEGION_FOREACH_N(DIMFUNC)
#undef DIMFUNC
        default:
          assert(false);
      }
      return DomainPoint::nil();
    }

    //--------------------------------------------------------------------------
    bool Runtime::safe_cast_internal(Context ctx, LogicalRegion region,
                                     const void *realm_point, TypeTag type_tag) 
    //--------------------------------------------------------------------------
    {
      return runtime->safe_cast(ctx, region, realm_point, type_tag);
    }

    //--------------------------------------------------------------------------
    FieldSpace Runtime::create_field_space(Context ctx, const char *prov)
    //--------------------------------------------------------------------------
    {
      Internal::AutoProvenance provenance(prov);
      return ctx->create_field_space(provenance);
    }

    //--------------------------------------------------------------------------
    FieldSpace Runtime::create_field_space(Context ctx,
                                         const std::vector<size_t> &field_sizes,
                                         std::vector<FieldID> &resulting_fields,
                                         CustomSerdezID serdez_id, 
                                         const char *prov)
    //--------------------------------------------------------------------------
    {
      Internal::AutoProvenance provenance(prov);
      return ctx->create_field_space(field_sizes, resulting_fields,
                                     serdez_id, provenance);
    }

    //--------------------------------------------------------------------------
    FieldSpace Runtime::create_field_space(Context ctx,
                                         const std::vector<Future> &field_sizes,
                                         std::vector<FieldID> &resulting_fields,
                                         CustomSerdezID serdez_id,
                                         const char *prov)
    //--------------------------------------------------------------------------
    {
      Internal::AutoProvenance provenance(prov);
      return ctx->create_field_space(field_sizes, resulting_fields,
                                     serdez_id, provenance);
    }

    //--------------------------------------------------------------------------
    void Runtime::create_shared_ownership(Context ctx, FieldSpace handle)
    //--------------------------------------------------------------------------
    {
      ctx->create_shared_ownership(handle);
    }

    //--------------------------------------------------------------------------
    void Runtime::destroy_field_space(Context ctx, FieldSpace handle,
                                      const bool unordered, const char *prov)
    //--------------------------------------------------------------------------
    {
      Internal::AutoProvenance provenance(prov);
      ctx->destroy_field_space(handle, unordered, provenance);
    }

    //--------------------------------------------------------------------------
    size_t Runtime::get_field_size(Context ctx, FieldSpace handle,
                                            FieldID fid)
    //--------------------------------------------------------------------------
    {
      return runtime->get_field_size(ctx, handle, fid);
    }

    //--------------------------------------------------------------------------
    size_t Runtime::get_field_size(FieldSpace handle, FieldID fid)
    //--------------------------------------------------------------------------
    {
      return runtime->get_field_size(handle, fid);
    }

    //--------------------------------------------------------------------------
    void Runtime::get_field_space_fields(Context ctx, FieldSpace handle,
                                         std::vector<FieldID> &fields)
    //--------------------------------------------------------------------------
    {
      runtime->get_field_space_fields(ctx, handle, fields);
    }

    //--------------------------------------------------------------------------
    void Runtime::get_field_space_fields(FieldSpace handle,
                                         std::vector<FieldID> &fields)
    //--------------------------------------------------------------------------
    {
      runtime->get_field_space_fields(handle, fields);
    }

    //--------------------------------------------------------------------------
    void Runtime::get_field_space_fields(Context ctx, FieldSpace handle,
                                         std::set<FieldID> &fields)
    //--------------------------------------------------------------------------
    {
      std::vector<FieldID> local;
      runtime->get_field_space_fields(ctx, handle, local);
      fields.insert(local.begin(), local.end());
    }

    //--------------------------------------------------------------------------
    void Runtime::get_field_space_fields(FieldSpace handle,
                                         std::set<FieldID> &fields)
    //--------------------------------------------------------------------------
    {
      std::vector<FieldID> local;
      runtime->get_field_space_fields(handle, local);
      fields.insert(local.begin(), local.end());
    }

    //--------------------------------------------------------------------------
    LogicalRegion Runtime::create_logical_region(Context ctx, 
                           IndexSpace index, FieldSpace fields,
                           bool task_local, const char *prov)
    //--------------------------------------------------------------------------
    {
      Internal::AutoProvenance provenance(prov);
      return ctx->create_logical_region(index, fields, task_local, provenance);
    }

    //--------------------------------------------------------------------------
    void Runtime::create_shared_ownership(Context ctx, LogicalRegion handle)
    //--------------------------------------------------------------------------
    {
      ctx->create_shared_ownership(handle);
    }

    //--------------------------------------------------------------------------
    void Runtime::destroy_logical_region(Context ctx, LogicalRegion handle,
                                         const bool unordered, const char *prov)
    //--------------------------------------------------------------------------
    {
      Internal::AutoProvenance provenance(prov);
      ctx->destroy_logical_region(handle, unordered, provenance);
    }

    //--------------------------------------------------------------------------
    void Runtime::destroy_logical_partition(Context ctx,LogicalPartition handle,
                                            const bool unordered)
    //--------------------------------------------------------------------------
    {
      // This is a no-op now
    }

    //--------------------------------------------------------------------------
    void Runtime::reset_equivalence_sets(Context ctx, LogicalRegion parent,
                                         LogicalRegion region,
                                         const std::set<FieldID> &fields)
    //--------------------------------------------------------------------------
    {
      ctx->reset_equivalence_sets(parent, region, fields);
    }

    //--------------------------------------------------------------------------
    LogicalPartition Runtime::get_logical_partition(Context ctx, 
                                    LogicalRegion parent, IndexPartition handle)
    //--------------------------------------------------------------------------
    {
      return runtime->get_logical_partition(ctx, parent, handle);
    }

    //--------------------------------------------------------------------------
    LogicalPartition Runtime::get_logical_partition(LogicalRegion parent, 
                                                    IndexPartition handle)
    //--------------------------------------------------------------------------
    {
      return runtime->get_logical_partition(parent, handle);
    }

    //--------------------------------------------------------------------------
    LogicalPartition Runtime::get_logical_partition_by_color(
                                    Context ctx, LogicalRegion parent, Color c)
    //--------------------------------------------------------------------------
    {
      return runtime->get_logical_partition_by_color(ctx, parent, c);
    }

    //--------------------------------------------------------------------------
    LogicalPartition Runtime::get_logical_partition_by_color(
                        Context ctx, LogicalRegion parent, const DomainPoint &c)
    //--------------------------------------------------------------------------
    {
      return runtime->get_logical_partition_by_color(ctx, parent,c.get_color());
    }

    //--------------------------------------------------------------------------
    LogicalPartition Runtime::get_logical_partition_by_color(
                                                  LogicalRegion parent, Color c)
    //--------------------------------------------------------------------------
    {
      return runtime->get_logical_partition_by_color(parent, c);
    }

    //--------------------------------------------------------------------------
    LogicalPartition Runtime::get_logical_partition_by_color(
                                     LogicalRegion parent, const DomainPoint &c)
    //--------------------------------------------------------------------------
    {
      return runtime->get_logical_partition_by_color(parent, c.get_color());
    }

    //--------------------------------------------------------------------------
    bool Runtime::has_logical_partition_by_color(Context ctx,
                                     LogicalRegion parent, const DomainPoint &c)
    //--------------------------------------------------------------------------
    {
      return runtime->has_logical_partition_by_color(ctx, parent,c.get_color());
    }

    //--------------------------------------------------------------------------
    bool Runtime::has_logical_partition_by_color(LogicalRegion parent, 
                                                 const DomainPoint &c)
    //--------------------------------------------------------------------------
    {
      return runtime->has_logical_partition_by_color(parent, c.get_color());
    }

    //--------------------------------------------------------------------------
    LogicalPartition Runtime::get_logical_partition_by_tree(
                                            Context ctx, IndexPartition handle, 
                                            FieldSpace fspace, RegionTreeID tid) 
    //--------------------------------------------------------------------------
    {
      return runtime->get_logical_partition_by_tree(ctx, handle, fspace, tid);
    }

    //--------------------------------------------------------------------------
    LogicalPartition Runtime::get_logical_partition_by_tree(
                                            IndexPartition handle, 
                                            FieldSpace fspace, RegionTreeID tid) 
    //--------------------------------------------------------------------------
    {
      return runtime->get_logical_partition_by_tree(handle, fspace, tid);
    }

    //--------------------------------------------------------------------------
    LogicalRegion Runtime::get_logical_subregion(Context ctx, 
                                    LogicalPartition parent, IndexSpace handle)
    //--------------------------------------------------------------------------
    {
      return runtime->get_logical_subregion(ctx, parent, handle);
    }

    //--------------------------------------------------------------------------
    LogicalRegion Runtime::get_logical_subregion(LogicalPartition parent, 
                                                 IndexSpace handle)
    //--------------------------------------------------------------------------
    {
      return runtime->get_logical_subregion(parent, handle);
    }

    //--------------------------------------------------------------------------
    LogicalRegion Runtime::get_logical_subregion_by_color(Context ctx, 
                                             LogicalPartition parent, Color c)
    //--------------------------------------------------------------------------
    {
      Point<1,coord_t> point(c);
      return runtime->get_logical_subregion_by_color(ctx, parent, 
                                                     &point, TYPE_TAG_1D);
    }

    //--------------------------------------------------------------------------
    LogicalRegion Runtime::get_logical_subregion_by_color(Context ctx,
                                  LogicalPartition parent, const DomainPoint &c)
    //--------------------------------------------------------------------------
    {
      switch (c.get_dim())
      {
#define DIMFUNC(DIM) \
        case DIM: \
          { \
            Point<DIM,coord_t> point(c); \
            return runtime->get_logical_subregion_by_color(ctx, parent,  \
                                             &point, TYPE_TAG_##DIM##D); \
          }
        LEGION_FOREACH_N(DIMFUNC)
#undef DIMFUNC
        default:
          assert(false);
      }
      return LogicalRegion::NO_REGION;
    }

    //--------------------------------------------------------------------------
    LogicalRegion Runtime::get_logical_subregion_by_color(
                                               LogicalPartition parent, Color c)
    //--------------------------------------------------------------------------
    {
      Point<1,coord_t> point(c);
      return runtime->get_logical_subregion_by_color(parent,&point,TYPE_TAG_1D);
    }

    //--------------------------------------------------------------------------
    LogicalRegion Runtime::get_logical_subregion_by_color(
                                  LogicalPartition parent, const DomainPoint &c)
    //--------------------------------------------------------------------------
    {
      switch (c.get_dim())
      {
#define DIMFUNC(DIM) \
        case DIM: \
          { \
            Point<DIM,coord_t> point(c); \
            return runtime->get_logical_subregion_by_color(parent, &point, \
                                                       TYPE_TAG_##DIM##D); \
          }
        LEGION_FOREACH_N(DIMFUNC)
#undef DIMFUNC
        default:
          assert(false);
      }
      return LogicalRegion::NO_REGION;
    }

    //--------------------------------------------------------------------------
    LogicalRegion Runtime::get_logical_subregion_by_color_internal(
             LogicalPartition parent, const void *realm_color, TypeTag type_tag)
    //--------------------------------------------------------------------------
    {
      return runtime->get_logical_subregion_by_color(parent, 
                                                     realm_color, type_tag);
    }
    
    //--------------------------------------------------------------------------
    bool Runtime::has_logical_subregion_by_color(Context ctx,
                                  LogicalPartition parent, const DomainPoint &c)
    //--------------------------------------------------------------------------
    {
      switch (c.get_dim())
      {
#define DIMFUNC(DIM) \
      case DIM: \
        { \
          Point<DIM,coord_t> point(c); \
          return runtime->has_logical_subregion_by_color(ctx, parent, &point, \
                                                         TYPE_TAG_##DIM##D); \
        }
        LEGION_FOREACH_N(DIMFUNC)
#undef DIMFUNC
        default:
          assert(false);
      }
      return false;
    }

    //--------------------------------------------------------------------------
    bool Runtime::has_logical_subregion_by_color(LogicalPartition parent, 
                                                 const DomainPoint &c)
    //--------------------------------------------------------------------------
    {
      switch (c.get_dim())
      {
#define DIMFUNC(DIM) \
        case DIM: \
          { \
            Point<DIM,coord_t> point(c); \
            return runtime->has_logical_subregion_by_color(parent, &point, \
                                                       TYPE_TAG_##DIM##D); \
          }
        LEGION_FOREACH_N(DIMFUNC)
#undef DIMFUNC
        default:
          assert(false);
      }
      return false;
    }

    //--------------------------------------------------------------------------
    bool Runtime::has_logical_subregion_by_color_internal(
             LogicalPartition parent, const void *realm_color, TypeTag type_tag)
    //--------------------------------------------------------------------------
    {
      return runtime->has_logical_subregion_by_color(parent, 
                                                     realm_color, type_tag);
    }

    //--------------------------------------------------------------------------
    LogicalRegion Runtime::get_logical_subregion_by_tree(Context ctx, 
                        IndexSpace handle, FieldSpace fspace, RegionTreeID tid)
    //--------------------------------------------------------------------------
    {
      return runtime->get_logical_subregion_by_tree(ctx, handle, fspace, tid);
    }

    //--------------------------------------------------------------------------
    LogicalRegion Runtime::get_logical_subregion_by_tree(IndexSpace handle, 
                                            FieldSpace fspace, RegionTreeID tid)
    //--------------------------------------------------------------------------
    {
      return runtime->get_logical_subregion_by_tree(handle, fspace, tid);
    }

    //--------------------------------------------------------------------------
    Color Runtime::get_logical_region_color(Context ctx, LogicalRegion handle)
    //--------------------------------------------------------------------------
    {
      Point<1,coord_t> point;
      runtime->get_logical_region_color(ctx, handle, &point, TYPE_TAG_1D);
      return point[0];
    }

    //--------------------------------------------------------------------------
    DomainPoint Runtime::get_logical_region_color_point(Context ctx,
                                                        LogicalRegion handle)
    //--------------------------------------------------------------------------
    {
      return runtime->get_logical_region_color_point(ctx, handle); 
    }

    //--------------------------------------------------------------------------
    Color Runtime::get_logical_region_color(LogicalRegion handle)
    //--------------------------------------------------------------------------
    {
      Point<1,coord_t> point;
      runtime->get_logical_region_color(handle, &point, TYPE_TAG_1D);
      return point[0];
    }

    //--------------------------------------------------------------------------
    DomainPoint Runtime::get_logical_region_color_point(LogicalRegion handle)
    //--------------------------------------------------------------------------
    {
      return runtime->get_logical_region_color_point(handle); 
    }

    //--------------------------------------------------------------------------
    Color Runtime::get_logical_partition_color(Context ctx,
                                                        LogicalPartition handle)
    //--------------------------------------------------------------------------
    {
      return runtime->get_logical_partition_color(ctx, handle);
    }

    //--------------------------------------------------------------------------
    DomainPoint Runtime::get_logical_partition_color_point(Context ctx,
                                                        LogicalPartition handle)
    //--------------------------------------------------------------------------
    {
      return DomainPoint(runtime->get_logical_partition_color(ctx, handle));
    }

    //--------------------------------------------------------------------------
    Color Runtime::get_logical_partition_color(LogicalPartition handle)
    //--------------------------------------------------------------------------
    {
      return runtime->get_logical_partition_color(handle);
    }

    //--------------------------------------------------------------------------
    DomainPoint Runtime::get_logical_partition_color_point(
                                                        LogicalPartition handle)
    //--------------------------------------------------------------------------
    {
      return DomainPoint(runtime->get_logical_partition_color(handle));
    }

    //--------------------------------------------------------------------------
    LogicalRegion Runtime::get_parent_logical_region(Context ctx,
                                                        LogicalPartition handle)
    //--------------------------------------------------------------------------
    {
      return runtime->get_parent_logical_region(ctx, handle);
    }

    //--------------------------------------------------------------------------
    LogicalRegion Runtime::get_parent_logical_region(LogicalPartition handle)
    //--------------------------------------------------------------------------
    {
      return runtime->get_parent_logical_region(handle);
    }

    //--------------------------------------------------------------------------
    bool Runtime::has_parent_logical_partition(Context ctx,
                                                        LogicalRegion handle)
    //--------------------------------------------------------------------------
    {
      return runtime->has_parent_logical_partition(ctx, handle);
    }

    //--------------------------------------------------------------------------
    bool Runtime::has_parent_logical_partition(LogicalRegion handle)
    //--------------------------------------------------------------------------
    {
      return runtime->has_parent_logical_partition(handle);
    }

    //--------------------------------------------------------------------------
    LogicalPartition Runtime::get_parent_logical_partition(Context ctx,
                                                           LogicalRegion handle)
    //--------------------------------------------------------------------------
    {
      return runtime->get_parent_logical_partition(ctx, handle);
    }

    //--------------------------------------------------------------------------
    LogicalPartition Runtime::get_parent_logical_partition(LogicalRegion handle)
    //--------------------------------------------------------------------------
    {
      return runtime->get_parent_logical_partition(handle);
    }

    LEGION_DISABLE_DEPRECATED_WARNINGS

    //--------------------------------------------------------------------------
    IndexAllocator Runtime::create_index_allocator(Context ctx, IndexSpace is)
    //--------------------------------------------------------------------------
    {
      Internal::log_run.warning("Dynamic index space allocation is no longer "
                                "supported. You can only make one allocator "
                                "per index space and it must always be in the "
                                "same task that created the index space.");
      return IndexAllocator(is, IndexIterator(this, ctx, is));
    }

    LEGION_REENABLE_DEPRECATED_WARNINGS

    //--------------------------------------------------------------------------
    FieldAllocator Runtime::create_field_allocator(Context ctx,FieldSpace space)
    //--------------------------------------------------------------------------
    {
      return FieldAllocator(
          ctx->create_field_allocator(space, false/*unordered*/));
    }

    //--------------------------------------------------------------------------
    ArgumentMap Runtime::create_argument_map(Context ctx)
    //--------------------------------------------------------------------------
    {
      return runtime->create_argument_map();
    }

    //--------------------------------------------------------------------------
    Future Runtime::execute_task(Context ctx, const TaskLauncher &launcher,
                             std::vector<OutputRequirement> *outputs /*= NULL*/)
    //--------------------------------------------------------------------------
    {
      return runtime->execute_task(ctx, launcher, outputs);
    }

    //--------------------------------------------------------------------------
    FutureMap Runtime::execute_index_space(
                             Context ctx, const IndexTaskLauncher &launcher,
                             std::vector<OutputRequirement> *outputs /*= NULL*/)
    //--------------------------------------------------------------------------
    {
      return runtime->execute_index_space(ctx, launcher, outputs);
    }

    //--------------------------------------------------------------------------
    Future Runtime::execute_index_space(
                             Context ctx, const IndexTaskLauncher &launcher,
                             ReductionOpID redop, bool deterministic,
                             std::vector<OutputRequirement> *outputs /*= NULL*/)
    //--------------------------------------------------------------------------
    {
      return runtime->execute_index_space(
                                  ctx, launcher, redop, deterministic, outputs);
    }

    //--------------------------------------------------------------------------
    Future Runtime::reduce_future_map(Context ctx, const FutureMap &future_map,
                                      ReductionOpID redop, bool deterministic,
                                      MapperID map, MappingTagID tag,
                                      const char *prov,
                                      Future initial_value)
    //--------------------------------------------------------------------------
    {
      Internal::AutoProvenance provenance(prov);
      return ctx->reduce_future_map(future_map, redop, deterministic,
                                    map, tag, provenance, initial_value);
    }

    //--------------------------------------------------------------------------
    FutureMap Runtime::construct_future_map(Context ctx, IndexSpace domain,
                                const std::map<DomainPoint,UntypedBuffer> &data,
                                bool collective, ShardingID sid, bool implicit,
                                const char *prov)
    //--------------------------------------------------------------------------
    {
      Internal::AutoProvenance provenance(prov);
      return ctx->construct_future_map(domain, data, provenance,
                                       collective, sid, implicit);
    }

    //--------------------------------------------------------------------------
    FutureMap Runtime::construct_future_map(Context ctx, const Domain &domain,
                                const std::map<DomainPoint,UntypedBuffer> &data,
                                bool collective, ShardingID sid, bool implicit)
    //--------------------------------------------------------------------------
    {
      return ctx->construct_future_map(domain, data, collective, sid, implicit);
    }

    //--------------------------------------------------------------------------
    FutureMap Runtime::construct_future_map(Context ctx, IndexSpace domain,
                                 const std::map<DomainPoint,Future> &futures,
                                 bool collective, ShardingID sid, bool implicit,
                                 const char *prov)
    //--------------------------------------------------------------------------
    {
      Internal::AutoProvenance provenance(prov);
      return ctx->construct_future_map(domain, futures, provenance, false,
                                       collective, sid, implicit);
    }

    //--------------------------------------------------------------------------
    FutureMap Runtime::construct_future_map(Context ctx, const Domain &domain,
                                 const std::map<DomainPoint,Future> &futures,
                                 bool collective, ShardingID sid, bool implicit)
    //--------------------------------------------------------------------------
    {
      return ctx->construct_future_map(domain, futures, false/*internal*/,
                                       collective, sid, implicit);
    }

    //--------------------------------------------------------------------------
    FutureMap Runtime::transform_future_map(Context ctx, const FutureMap &fm,
             IndexSpace new_domain, PointTransformFnptr fnptr, const char *prov)
    //--------------------------------------------------------------------------
    {
      Internal::AutoProvenance provenance(prov);
      return ctx->transform_future_map(fm, new_domain, fnptr, provenance);
    }

    //--------------------------------------------------------------------------
    FutureMap Runtime::transform_future_map(Context ctx, const FutureMap &fm,
                          IndexSpace new_domain, PointTransformFunctor *functor,
                          bool own, const char *prov)
    //--------------------------------------------------------------------------
    {
      Internal::AutoProvenance provenance(prov);
      return ctx->transform_future_map(fm, new_domain, functor, own,provenance);
    }

    //--------------------------------------------------------------------------
    Future Runtime::execute_task(Context ctx, 
                        TaskID task_id,
                        const std::vector<IndexSpaceRequirement> &indexes,
                        const std::vector<FieldSpaceRequirement> &fields,
                        const std::vector<RegionRequirement> &regions,
                        const UntypedBuffer &arg, 
                        const Predicate &predicate,
                        MapperID id, 
                        MappingTagID tag)
    //--------------------------------------------------------------------------
    {
      TaskLauncher launcher(task_id, arg, predicate, id, tag);
      launcher.index_requirements = indexes;
      launcher.region_requirements = regions;
      return runtime->execute_task(ctx, launcher, NULL);
    }

    //--------------------------------------------------------------------------
    FutureMap Runtime::execute_index_space(Context ctx, 
                        TaskID task_id,
                        const Domain domain,
                        const std::vector<IndexSpaceRequirement> &indexes,
                        const std::vector<FieldSpaceRequirement> &fields,
                        const std::vector<RegionRequirement> &regions,
                        const UntypedBuffer &global_arg, 
                        const ArgumentMap &arg_map,
                        const Predicate &predicate,
                        bool must_parallelism, 
                        MapperID id, 
                        MappingTagID tag)
    //--------------------------------------------------------------------------
    {
      IndexTaskLauncher launcher(task_id, domain, global_arg, arg_map,
                                 predicate, must_parallelism, id, tag);
      launcher.index_requirements = indexes;
      launcher.region_requirements = regions;
      return runtime->execute_index_space(ctx, launcher, NULL);
    }


    //--------------------------------------------------------------------------
    Future Runtime::execute_index_space(Context ctx, 
                        TaskID task_id,
                        const Domain domain,
                        const std::vector<IndexSpaceRequirement> &indexes,
                        const std::vector<FieldSpaceRequirement> &fields,
                        const std::vector<RegionRequirement> &regions,
                        const UntypedBuffer &global_arg, 
                        const ArgumentMap &arg_map,
                        ReductionOpID reduction, 
                        const UntypedBuffer &initial_value,
                        const Predicate &predicate,
                        bool must_parallelism, 
                        MapperID id, 
                        MappingTagID tag)
    //--------------------------------------------------------------------------
    {
      IndexTaskLauncher launcher(task_id, domain, global_arg, arg_map,
                                 predicate, must_parallelism, id, tag);
      launcher.index_requirements = indexes;
      launcher.region_requirements = regions;
      return runtime->execute_index_space(ctx, launcher, reduction, false,NULL);
    }

    //--------------------------------------------------------------------------
    PhysicalRegion Runtime::map_region(Context ctx, 
                                                const InlineLauncher &launcher)
    //--------------------------------------------------------------------------
    {
      return runtime->map_region(ctx, launcher);
    }

    //--------------------------------------------------------------------------
    PhysicalRegion Runtime::map_region(Context ctx, 
                                    const RegionRequirement &req, MapperID id,
                                    MappingTagID tag, const char *provenance)
    //--------------------------------------------------------------------------
    {
      InlineLauncher launcher(req, id, tag);
      if (provenance != NULL)
        launcher.provenance = provenance;
      return runtime->map_region(ctx, launcher);
    }

    //--------------------------------------------------------------------------
    PhysicalRegion Runtime::map_region(Context ctx, unsigned idx, 
                        MapperID id, MappingTagID tag, const char *prov)
    //--------------------------------------------------------------------------
    {
      Internal::AutoProvenance provenance(prov);
      return runtime->map_region(ctx, idx, id, tag, provenance);
    }

    //--------------------------------------------------------------------------
    void Runtime::remap_region(Context ctx, PhysicalRegion region,
                               const char *prov)
    //--------------------------------------------------------------------------
    {
      Internal::AutoProvenance provenance(prov);
      runtime->remap_region(ctx, region, provenance);
    }

    //--------------------------------------------------------------------------
    void Runtime::unmap_region(Context ctx, PhysicalRegion region)
    //--------------------------------------------------------------------------
    {
      runtime->unmap_region(ctx, region);
    }

    //--------------------------------------------------------------------------
    void Runtime::unmap_all_regions(Context ctx)
    //--------------------------------------------------------------------------
    {
      ctx->unmap_all_regions(true/*external*/);
    }

    //--------------------------------------------------------------------------
    OutputRegion Runtime::get_output_region(Context ctx, unsigned index)
    //--------------------------------------------------------------------------
    {
      return ctx->get_output_region(index);
    }

    //--------------------------------------------------------------------------
    void Runtime::get_output_regions(
                                Context ctx, std::vector<OutputRegion> &regions)
    //--------------------------------------------------------------------------
    {
      regions = ctx->get_output_regions();
    }

    //--------------------------------------------------------------------------
    void Runtime::fill_field(Context ctx, LogicalRegion handle,
                                      LogicalRegion parent, FieldID fid,
                                      const void *value, size_t size,
                                      Predicate pred)
    //--------------------------------------------------------------------------
    {
      FillLauncher launcher(handle, parent, UntypedBuffer(value, size), pred);
      launcher.add_field(fid);
      runtime->fill_fields(ctx, launcher);
    }

    //--------------------------------------------------------------------------
    void Runtime::fill_field(Context ctx, LogicalRegion handle,
                                      LogicalRegion parent, FieldID fid,
                                      Future f, Predicate pred)
    //--------------------------------------------------------------------------
    {
      FillLauncher launcher(handle, parent, UntypedBuffer(), pred);
      launcher.set_future(f);
      launcher.add_field(fid);
      runtime->fill_fields(ctx, launcher);
    }

    //--------------------------------------------------------------------------
    void Runtime::fill_fields(Context ctx, LogicalRegion handle,
                                       LogicalRegion parent,
                                       const std::set<FieldID> &fields,
                                       const void *value, size_t size,
                                       Predicate pred)
    //--------------------------------------------------------------------------
    {
      FillLauncher launcher(handle, parent, UntypedBuffer(value, size), pred);
      launcher.fields = fields;
      runtime->fill_fields(ctx, launcher);
    }

    //--------------------------------------------------------------------------
    void Runtime::fill_fields(Context ctx, LogicalRegion handle,
                                       LogicalRegion parent, 
                                       const std::set<FieldID> &fields,
                                       Future f, Predicate pred)
    //--------------------------------------------------------------------------
    {
      FillLauncher launcher(handle, parent, UntypedBuffer(), pred);
      launcher.set_future(f);
      launcher.fields = fields;
      runtime->fill_fields(ctx, launcher);
    }

    //--------------------------------------------------------------------------
    void Runtime::fill_fields(Context ctx, const FillLauncher &launcher)
    //--------------------------------------------------------------------------
    {
      runtime->fill_fields(ctx, launcher);
    }

    //--------------------------------------------------------------------------
    void Runtime::fill_fields(Context ctx, const IndexFillLauncher &launcher)
    //--------------------------------------------------------------------------
    {
      runtime->fill_fields(ctx, launcher);
    }

    //--------------------------------------------------------------------------
    void Runtime::discard_fields(Context ctx, const DiscardLauncher &launcher)
    //--------------------------------------------------------------------------
    {
      ctx->discard_fields(launcher);
    }

    //--------------------------------------------------------------------------
    PhysicalRegion Runtime::attach_external_resource(Context ctx, 
                                                 const AttachLauncher &launcher)
    //--------------------------------------------------------------------------
    {
      if (launcher.mapped)
      {
        PhysicalRegion region = ctx->attach_resource(launcher);
        Internal::AutoProvenance provenance(launcher.provenance);
        ctx->remap_region(region, provenance);
        return region;
      }
      else
        return ctx->attach_resource(launcher);
    }

    //--------------------------------------------------------------------------
    ExternalResources Runtime::attach_external_resources(Context ctx,
                                            const IndexAttachLauncher &launcher)
    //--------------------------------------------------------------------------
    {
      return ctx->attach_resources(launcher);
    }

    //--------------------------------------------------------------------------
    Future Runtime::detach_external_resource(Context ctx, PhysicalRegion region,
                                             const bool flush /*= true*/,
                                             const bool unordered/*= false*/,
                                             const char *prov)
    //--------------------------------------------------------------------------
    {
      Internal::AutoProvenance provenance(prov);
      return ctx->detach_resource(region, flush, unordered, provenance);
    }

    //--------------------------------------------------------------------------
    Future Runtime::detach_external_resources(Context ctx,
                                              ExternalResources resources,
                                              const bool flush /*= true*/,
                                              const bool unordered /*= false*/,
                                              const char *prov)
    //--------------------------------------------------------------------------
    {
      Internal::AutoProvenance provenance(prov);
      return ctx->detach_resources(resources, flush, unordered, provenance);
    }

    //--------------------------------------------------------------------------
    void Runtime::progress_unordered_operations(Context ctx)
    //--------------------------------------------------------------------------
    {
      ctx->progress_unordered_operations();
    }

    LEGION_DISABLE_DEPRECATED_WARNINGS

    //--------------------------------------------------------------------------
    PhysicalRegion Runtime::attach_hdf5(Context ctx, 
                                                 const char *file_name,
                                                 LogicalRegion handle,
                                                 LogicalRegion parent,
                                 const std::map<FieldID,const char*> &field_map,
                                                 LegionFileMode mode)
    //--------------------------------------------------------------------------
    {
      AttachLauncher launcher(LEGION_EXTERNAL_HDF5_FILE, handle, parent);
      launcher.attach_hdf5(file_name, field_map, mode);
      PhysicalRegion region = ctx->attach_resource(launcher);
      if (launcher.mapped)
        ctx->remap_region(region, NULL/*no provenance because deprecated*/);
      return region;
    }

    //--------------------------------------------------------------------------
    void Runtime::detach_hdf5(Context ctx, PhysicalRegion region)
    //--------------------------------------------------------------------------
    {
      ctx->detach_resource(region, true/*flush*/, false/*unordered*/);
    }

    //--------------------------------------------------------------------------
    PhysicalRegion Runtime::attach_file(Context ctx,
                                                 const char *file_name,
                                                 LogicalRegion handle,
                                                 LogicalRegion parent,
                                 const std::vector<FieldID> &field_vec,
                                                 LegionFileMode mode)
    //--------------------------------------------------------------------------
    {
      AttachLauncher launcher(LEGION_EXTERNAL_POSIX_FILE, handle, parent);
      launcher.attach_file(file_name, field_vec, mode);
      PhysicalRegion region = ctx->attach_resource(launcher);
      if (launcher.mapped)
        ctx->remap_region(region, NULL/*no provenance because deprecated*/);
      return region;
    }

    LEGION_REENABLE_DEPRECATED_WARNINGS

    //--------------------------------------------------------------------------
    void Runtime::detach_file(Context ctx, PhysicalRegion region)
    //--------------------------------------------------------------------------
    {
      ctx->detach_resource(region, true/*flush*/, false/*unordered*/);
    }
    
    //--------------------------------------------------------------------------
    void Runtime::issue_copy_operation(Context ctx,const CopyLauncher &launcher)
    //--------------------------------------------------------------------------
    {
      runtime->issue_copy_operation(ctx, launcher);
    }

    //--------------------------------------------------------------------------
    void Runtime::issue_copy_operation(Context ctx,
                                       const IndexCopyLauncher &launcher)
    //--------------------------------------------------------------------------
    {
      runtime->issue_copy_operation(ctx, launcher);
    }

    //--------------------------------------------------------------------------
    Predicate Runtime::create_predicate(Context ctx, const Future &f,
                                        const char *prov)
    //--------------------------------------------------------------------------
    {
      Internal::AutoProvenance provenance(prov);
      return ctx->create_predicate(f, provenance);
    }

    //--------------------------------------------------------------------------
    Predicate Runtime::predicate_not(Context ctx, const Predicate &p,
                                     const char *prov) 
    //--------------------------------------------------------------------------
    {
      Internal::AutoProvenance provenance(prov);
      return ctx->predicate_not(p, provenance);
    }

    //--------------------------------------------------------------------------
    Predicate Runtime::predicate_and(Context ctx, 
                                     const Predicate &p1, const Predicate &p2,
                                     const char *provenance)
    //--------------------------------------------------------------------------
    {
      PredicateLauncher launcher(true/*and*/);
      launcher.add_predicate(p1);
      launcher.add_predicate(p2);
      if (provenance != NULL)
        launcher.provenance.assign(provenance);
      return ctx->create_predicate(launcher);
    }

    //--------------------------------------------------------------------------
    Predicate Runtime::predicate_or(Context ctx,
                                    const Predicate &p1, const Predicate &p2,
                                    const char *provenance)  
    //--------------------------------------------------------------------------
    {
      PredicateLauncher launcher(false/*and*/);
      launcher.add_predicate(p1);
      launcher.add_predicate(p2);
      if (provenance != NULL)
        launcher.provenance.assign(provenance);
      return ctx->create_predicate(launcher);
    }

    //--------------------------------------------------------------------------
    Predicate Runtime::create_predicate(Context ctx, 
                                        const PredicateLauncher &launcher)
    //--------------------------------------------------------------------------
    {
      return ctx->create_predicate(launcher);
    }

    //--------------------------------------------------------------------------
    Future Runtime::get_predicate_future(Context ctx, const Predicate &p,
                                         const char *prov)
    //--------------------------------------------------------------------------
    {
      Internal::AutoProvenance provenance(prov);
      return ctx->get_predicate_future(p, provenance);
    }

    //--------------------------------------------------------------------------
    Lock Runtime::create_lock(Context ctx)
    //--------------------------------------------------------------------------
    {
      return ctx->create_lock();
    }

    //--------------------------------------------------------------------------
    void Runtime::destroy_lock(Context ctx, Lock l)
    //--------------------------------------------------------------------------
    {
      ctx->destroy_lock(l);
    }

    //--------------------------------------------------------------------------
    Grant Runtime::acquire_grant(Context ctx,
                                      const std::vector<LockRequest> &requests)
    //--------------------------------------------------------------------------
    {
      return ctx->acquire_grant(requests);
    }

    //--------------------------------------------------------------------------
    void Runtime::release_grant(Context ctx, Grant grant)
    //--------------------------------------------------------------------------
    {
      ctx->release_grant(grant);
    }

    //--------------------------------------------------------------------------
    PhaseBarrier Runtime::create_phase_barrier(Context ctx, 
                                                        unsigned arrivals)
    //--------------------------------------------------------------------------
    {
      return ctx->create_phase_barrier(arrivals);
    }

    //--------------------------------------------------------------------------
    void Runtime::destroy_phase_barrier(Context ctx, PhaseBarrier pb)
    //--------------------------------------------------------------------------
    {
      ctx->destroy_phase_barrier(pb);
    }

    //--------------------------------------------------------------------------
    PhaseBarrier Runtime::advance_phase_barrier(Context ctx, 
                                                         PhaseBarrier pb)
    //--------------------------------------------------------------------------
    {
      return ctx->advance_phase_barrier(pb);
    }

    //--------------------------------------------------------------------------
    DynamicCollective Runtime::create_dynamic_collective(Context ctx,
                                                        unsigned arrivals,
                                                        ReductionOpID redop,
                                                        const void *init_value,
                                                        size_t init_size)
    //--------------------------------------------------------------------------
    {
      return ctx->create_dynamic_collective(arrivals, redop, 
                                            init_value, init_size);
    }
    
    //--------------------------------------------------------------------------
    void Runtime::destroy_dynamic_collective(Context ctx, 
                                                      DynamicCollective dc)
    //--------------------------------------------------------------------------
    {
      ctx->destroy_dynamic_collective(dc);
    }

    //--------------------------------------------------------------------------
    void Runtime::arrive_dynamic_collective(Context ctx,
                                                     DynamicCollective dc,
                                                     const void *buffer,
                                                     size_t size, 
                                                     unsigned count)
    //--------------------------------------------------------------------------
    {
      ctx->arrive_dynamic_collective(dc, buffer, size, count);
    }

    //--------------------------------------------------------------------------
    void Runtime::defer_dynamic_collective_arrival(Context ctx,
                                                   DynamicCollective dc,
                                                   const Future &f, 
                                                   unsigned count)
    //--------------------------------------------------------------------------
    {
      ctx->defer_dynamic_collective_arrival(dc, f, count);
    }

    //--------------------------------------------------------------------------
    Future Runtime::get_dynamic_collective_result(Context ctx,
                                         DynamicCollective dc, const char *prov)
    //--------------------------------------------------------------------------
    {
      Internal::AutoProvenance provenance(prov);
      return ctx->get_dynamic_collective_result(dc, provenance);
    }

    //--------------------------------------------------------------------------
    DynamicCollective Runtime::advance_dynamic_collective(Context ctx,
                                                           DynamicCollective dc)
    //--------------------------------------------------------------------------
    {
      return ctx->advance_dynamic_collective(dc);
    }

    //--------------------------------------------------------------------------
    void Runtime::issue_acquire(Context ctx,
                                         const AcquireLauncher &launcher)
    //--------------------------------------------------------------------------
    {
      runtime->issue_acquire(ctx, launcher);
    }

    //--------------------------------------------------------------------------
    void Runtime::issue_release(Context ctx,
                                         const ReleaseLauncher &launcher)
    //--------------------------------------------------------------------------
    {
      runtime->issue_release(ctx, launcher);
    }

    //--------------------------------------------------------------------------
    Future Runtime::issue_mapping_fence(Context ctx, const char *prov)
    //--------------------------------------------------------------------------
    {
      Internal::AutoProvenance provenance(prov);
      return ctx->issue_mapping_fence(provenance);
    }

    //--------------------------------------------------------------------------
    Future Runtime::issue_execution_fence(Context ctx, const char *prov)
    //--------------------------------------------------------------------------
    {
      Internal::AutoProvenance provenance(prov);
      return ctx->issue_execution_fence(provenance);
    }

    //--------------------------------------------------------------------------
    void Runtime::begin_trace(
       Context ctx, TraceID tid, bool logical_only /*= false*/,
       bool static_trace, const std::set<RegionTreeID> *trees, const char *prov)
    //--------------------------------------------------------------------------
    {
      Internal::AutoProvenance provenance(prov);
      ctx->begin_trace(tid, logical_only, static_trace, trees, 
                       false/*dep*/, provenance);
    }

    //--------------------------------------------------------------------------
    void Runtime::end_trace(Context ctx, TraceID tid, const char *prov)
    //--------------------------------------------------------------------------
    {
      Internal::AutoProvenance provenance(prov);
      ctx->end_trace(tid, false/*deprecated*/, provenance);
    }

    //--------------------------------------------------------------------------
    void Runtime::begin_static_trace(Context ctx,
                                     const std::set<RegionTreeID> *managed)
    //--------------------------------------------------------------------------
    {
      ctx->begin_trace(0, true/*logical only*/, true/*static*/, managed,
                       true/*deprecated*/, NULL/*provenance*/);
    }

    //--------------------------------------------------------------------------
    void Runtime::end_static_trace(Context ctx)
    //--------------------------------------------------------------------------
    {
      ctx->end_trace(0, true/*deprecated*/, NULL/*provenance*/);
    }

    //--------------------------------------------------------------------------
    TraceID Runtime::generate_dynamic_trace_id(void)
    //--------------------------------------------------------------------------
    {
      return runtime->generate_dynamic_trace_id();
    }

    //--------------------------------------------------------------------------
    TraceID Runtime::generate_library_trace_ids(const char *name, size_t count)
    //--------------------------------------------------------------------------
    {
      return runtime->generate_library_trace_ids(name, count);
    }

    //--------------------------------------------------------------------------
    /*static*/ TraceID Runtime::generate_static_trace_id(void)
    //--------------------------------------------------------------------------
    {
      return Internal::Runtime::generate_static_trace_id();
    }

    //--------------------------------------------------------------------------
    void Runtime::complete_frame(Context ctx, const char *prov)
    //--------------------------------------------------------------------------
    {
      Internal::AutoProvenance provenance(prov);
      ctx->complete_frame(provenance);
    }

    //--------------------------------------------------------------------------
    FutureMap Runtime::execute_must_epoch(Context ctx,
                                              const MustEpochLauncher &launcher)
    //--------------------------------------------------------------------------
    {
      return runtime->execute_must_epoch(ctx, launcher);
    }

    //--------------------------------------------------------------------------
    Future Runtime::select_tunable_value(Context ctx, TunableID tid,
                                         MapperID mid, MappingTagID tag,
                                         const void *args, size_t argsize)
    //--------------------------------------------------------------------------
    {
      TunableLauncher launcher(tid, mid, tag);
      launcher.arg = UntypedBuffer(args, argsize);
      return select_tunable_value(ctx, launcher);
    }

    //--------------------------------------------------------------------------
    Future Runtime::select_tunable_value(Context ctx, 
                                         const TunableLauncher &launcher)
    //--------------------------------------------------------------------------
    {
      return ctx->select_tunable_value(launcher);
    }

    //--------------------------------------------------------------------------
    int Runtime::get_tunable_value(Context ctx, TunableID tid,
                                            MapperID mid, MappingTagID tag)
    //--------------------------------------------------------------------------
    {
      TunableLauncher launcher(tid, mid, tag);
      Future f = select_tunable_value(ctx, launcher);
      return f.get_result<int>();
    }

    //--------------------------------------------------------------------------
    const Task* Runtime::get_local_task(Context ctx)
    //--------------------------------------------------------------------------
    {
      return ctx->get_task();
    }

    //--------------------------------------------------------------------------
    void* Runtime::get_local_task_variable_untyped(Context ctx,
                                                   LocalVariableID id)
    //--------------------------------------------------------------------------
    {
      return runtime->get_local_task_variable(ctx, id);
    }

    //--------------------------------------------------------------------------
    void Runtime::set_local_task_variable_untyped(Context ctx,
               LocalVariableID id, const void* value, void (*destructor)(void*))
    //--------------------------------------------------------------------------
    {
      runtime->set_local_task_variable(ctx, id, value, destructor);
    }

    //--------------------------------------------------------------------------
    Future Runtime::get_current_time(Context ctx, Future precondition)
    //--------------------------------------------------------------------------
    {
      TimingLauncher launcher(LEGION_MEASURE_SECONDS);
      launcher.add_precondition(precondition);
      return runtime->issue_timing_measurement(ctx, launcher);
    }

    //--------------------------------------------------------------------------
    Future Runtime::get_current_time_in_microseconds(Context ctx, Future pre)
    //--------------------------------------------------------------------------
    {
      TimingLauncher launcher(LEGION_MEASURE_MICRO_SECONDS);
      launcher.add_precondition(pre);
      return runtime->issue_timing_measurement(ctx, launcher);
    }

    //--------------------------------------------------------------------------
    Future Runtime::get_current_time_in_nanoseconds(Context ctx, Future pre)
    //--------------------------------------------------------------------------
    {
      TimingLauncher launcher(LEGION_MEASURE_NANO_SECONDS);
      launcher.add_precondition(pre);
      return runtime->issue_timing_measurement(ctx, launcher);
    }

    //--------------------------------------------------------------------------
    Future Runtime::issue_timing_measurement(Context ctx, 
                                             const TimingLauncher &launcher)
    //--------------------------------------------------------------------------
    {
      return runtime->issue_timing_measurement(ctx, launcher);
    }

    //--------------------------------------------------------------------------
    /*static*/ long long Runtime::get_zero_time(void)
    //--------------------------------------------------------------------------
    {
      return Realm::Clock::get_zero_time();
    }

    //--------------------------------------------------------------------------
    Mapping::Mapper* Runtime::get_mapper(Context ctx, MapperID id,
                                         Processor target)
    //--------------------------------------------------------------------------
    {
      return runtime->get_mapper(ctx, id, target);
    }

    //--------------------------------------------------------------------------
    Mapping::MapperContext Runtime::begin_mapper_call(Context ctx, MapperID id,
                                                      Processor target)
    //--------------------------------------------------------------------------
    {
      return runtime->begin_mapper_call(ctx, id, target);
    }

    //--------------------------------------------------------------------------
    void Runtime::end_mapper_call(Mapping::MapperContext ctx)
    //--------------------------------------------------------------------------
    {
      runtime->end_mapper_call(ctx);
    }

    //--------------------------------------------------------------------------
    Processor Runtime::get_executing_processor(Context ctx)
    //--------------------------------------------------------------------------
    {
      return ctx->get_executing_processor();
    }

    //--------------------------------------------------------------------------
    const Task* Runtime::get_current_task(Context ctx)
    //--------------------------------------------------------------------------
    {
      if (ctx == DUMMY_CONTEXT)
        return NULL;
      return ctx->get_task();
    }

    //--------------------------------------------------------------------------
    size_t Runtime::query_available_memory(Context ctx, Memory target)
    //--------------------------------------------------------------------------
    {
      if (ctx == DUMMY_CONTEXT)
        return 0;
      return ctx->query_available_memory(target);
    }

    //--------------------------------------------------------------------------
    void Runtime::raise_region_exception(Context ctx, 
                                         PhysicalRegion region, bool nuclear)
    //--------------------------------------------------------------------------
    {
      ctx->raise_region_exception(region, nuclear);
    }

    //--------------------------------------------------------------------------
    void Runtime::yield(Context ctx)
    //--------------------------------------------------------------------------
    {
      ctx->yield();
    }

    //--------------------------------------------------------------------------
    bool Runtime::is_MPI_interop_configured(void)
    //--------------------------------------------------------------------------
    {
      return runtime->is_MPI_interop_configured();
    }

    //--------------------------------------------------------------------------
    const std::map<int,AddressSpace>& 
                                Runtime::find_forward_MPI_mapping(void)
    //--------------------------------------------------------------------------
    {
      return runtime->find_forward_MPI_mapping();
    }

    //--------------------------------------------------------------------------
    const std::map<AddressSpace,int>&
                                Runtime::find_reverse_MPI_mapping(void)
    //--------------------------------------------------------------------------
    {
      return runtime->find_reverse_MPI_mapping();
    }

    //--------------------------------------------------------------------------
    int Runtime::find_local_MPI_rank(void)
    //--------------------------------------------------------------------------
    {
      return runtime->find_local_MPI_rank();
    }

    //--------------------------------------------------------------------------
    Mapping::MapperRuntime* Runtime::get_mapper_runtime(void)
    //--------------------------------------------------------------------------
    {
      return runtime->get_mapper_runtime();
    }

    //--------------------------------------------------------------------------
    MapperID Runtime::generate_dynamic_mapper_id(void)
    //--------------------------------------------------------------------------
    {
      return runtime->generate_dynamic_mapper_id();
    }

    //--------------------------------------------------------------------------
    MapperID Runtime::generate_library_mapper_ids(const char *name, size_t cnt)
    //--------------------------------------------------------------------------
    {
      return runtime->generate_library_mapper_ids(name, cnt);
    }

    //--------------------------------------------------------------------------
    /*static*/ MapperID Runtime::generate_static_mapper_id(void)
    //--------------------------------------------------------------------------
    {
      return Internal::Runtime::generate_static_mapper_id();
    }

    //--------------------------------------------------------------------------
    void Runtime::add_mapper(MapperID map_id, Mapping::Mapper *mapper, 
                             Processor proc)
    //--------------------------------------------------------------------------
    {
      runtime->add_mapper(map_id, mapper, proc);
    }

    //--------------------------------------------------------------------------
    void Runtime::replace_default_mapper(Mapping::Mapper *mapper,Processor proc)
    //--------------------------------------------------------------------------
    {
      runtime->replace_default_mapper(mapper, proc);
    }

    //--------------------------------------------------------------------------
    ProjectionID Runtime::generate_dynamic_projection_id(void)
    //--------------------------------------------------------------------------
    {
      return runtime->generate_dynamic_projection_id();
    }

    //--------------------------------------------------------------------------
    ProjectionID Runtime::generate_library_projection_ids(const char *name,
                                                          size_t count)
    //--------------------------------------------------------------------------
    {
      return runtime->generate_library_projection_ids(name, count);
    }

    //--------------------------------------------------------------------------
    /*static*/ ProjectionID Runtime::generate_static_projection_id(void)
    //--------------------------------------------------------------------------
    {
      return Internal::Runtime::generate_static_projection_id();
    }

    //--------------------------------------------------------------------------
    void Runtime::register_projection_functor(ProjectionID pid,
                                              ProjectionFunctor *func,
                                              bool silence_warnings,
                                              const char *warning_string)
    //--------------------------------------------------------------------------
    {
      runtime->register_projection_functor(pid, func, true/*need zero check*/,
                                           silence_warnings, warning_string);
    }

    //--------------------------------------------------------------------------
    /*static*/ void Runtime::preregister_projection_functor(ProjectionID pid,
                                                        ProjectionFunctor *func)
    //--------------------------------------------------------------------------
    {
      Internal::Runtime::preregister_projection_functor(pid, func);
    }

    //--------------------------------------------------------------------------
    /*static*/ ProjectionFunctor* Runtime::get_projection_functor(
                                                               ProjectionID pid)
    //--------------------------------------------------------------------------
    {
      return Internal::Runtime::get_projection_functor(pid);
    }

    //--------------------------------------------------------------------------
    ShardingID Runtime::generate_dynamic_sharding_id(void)
    //--------------------------------------------------------------------------
    {
      return runtime->generate_dynamic_sharding_id();
    }

    //--------------------------------------------------------------------------
    ShardingID Runtime::generate_library_sharding_ids(const char *name,
                                                      size_t count)
    //--------------------------------------------------------------------------
    {
      return runtime->generate_library_sharding_ids(name, count);
    }

    //--------------------------------------------------------------------------
    /*static*/ ShardingID Runtime::generate_static_sharding_id(void)
    //--------------------------------------------------------------------------
    {
      return Internal::Runtime::generate_static_sharding_id();
    }

    //--------------------------------------------------------------------------
    void Runtime::register_sharding_functor(ShardingID sid,
                                            ShardingFunctor *functor,
                                            bool silence_warnings,
                                            const char *warning_string)
    //--------------------------------------------------------------------------
    {
      runtime->register_sharding_functor(sid, functor, true/*need zero check*/,
                                         silence_warnings, warning_string);
    }

    //--------------------------------------------------------------------------
    /*static*/ void Runtime::preregister_sharding_functor(ShardingID sid,
                                                          ShardingFunctor *func)
    //--------------------------------------------------------------------------
    {
      Internal::Runtime::preregister_sharding_functor(sid, func);
    }

    //--------------------------------------------------------------------------
    /*static*/ ShardingFunctor* Runtime::get_sharding_functor(ShardingID sid)
    //--------------------------------------------------------------------------
    {
      return Internal::Runtime::get_sharding_functor(sid);
    }

    //--------------------------------------------------------------------------
    void Runtime::attach_semantic_information(TaskID task_id, SemanticTag tag,
                       const void *buffer, size_t size, bool is_mut, bool local)
    //--------------------------------------------------------------------------
    {
      runtime->attach_semantic_information(task_id, tag, buffer, size, 
                                           is_mut, !local);
    }

    //--------------------------------------------------------------------------
    void Runtime::attach_semantic_information(IndexSpace handle,
                                                       SemanticTag tag,
                                                       const void *buffer,
                                                       size_t size, bool is_mut)
    //--------------------------------------------------------------------------
    {
      runtime->attach_semantic_information(handle, tag, buffer, size, is_mut);
    }

    //--------------------------------------------------------------------------
    void Runtime::attach_semantic_information(IndexPartition handle,
                                                       SemanticTag tag,
                                                       const void *buffer,
                                                       size_t size, bool is_mut)
    //--------------------------------------------------------------------------
    {
      runtime->attach_semantic_information(handle, tag, buffer, size, is_mut);
    }

    //--------------------------------------------------------------------------
    void Runtime::attach_semantic_information(FieldSpace handle,
                                                       SemanticTag tag,
                                                       const void *buffer,
                                                       size_t size, bool is_mut)
    //--------------------------------------------------------------------------
    {
      runtime->attach_semantic_information(handle, tag, buffer, size, is_mut);
    }

    //--------------------------------------------------------------------------
    void Runtime::attach_semantic_information(FieldSpace handle,
                                                       FieldID fid,
                                                       SemanticTag tag,
                                                       const void *buffer,
                                                       size_t size, bool is_mut)
    //--------------------------------------------------------------------------
    {
      runtime->attach_semantic_information(handle, fid, tag, buffer, 
                                           size, is_mut);
    }

    //--------------------------------------------------------------------------
    void Runtime::attach_semantic_information(LogicalRegion handle,
                                                       SemanticTag tag,
                                                       const void *buffer,
                                                       size_t size, bool is_mut)
    //--------------------------------------------------------------------------
    {
      runtime->attach_semantic_information(handle, tag, buffer, size, is_mut);
    }

    //--------------------------------------------------------------------------
    void Runtime::attach_semantic_information(LogicalPartition handle,
                                                       SemanticTag tag,
                                                       const void *buffer,
                                                       size_t size, bool is_mut)
    //--------------------------------------------------------------------------
    {
      runtime->attach_semantic_information(handle, tag, buffer, size, is_mut);
    }

    //--------------------------------------------------------------------------
    void Runtime::attach_name(TaskID task_id, const char *name, 
                              bool is_mutable, bool local_only)
    //--------------------------------------------------------------------------
    {
      Runtime::attach_semantic_information(task_id, LEGION_NAME_SEMANTIC_TAG, 
                              name, strlen(name) + 1, is_mutable, local_only);
    }

    //--------------------------------------------------------------------------
    void Runtime::attach_name(IndexSpace handle, const char *name, bool is_mut)
    //--------------------------------------------------------------------------
    {
      Runtime::attach_semantic_information(handle, LEGION_NAME_SEMANTIC_TAG, 
                                           name, strlen(name) + 1, is_mut);
    }

    //--------------------------------------------------------------------------
    void Runtime::attach_name(IndexPartition handle, const char *name, bool ism)
    //--------------------------------------------------------------------------
    {
      Runtime::attach_semantic_information(handle, LEGION_NAME_SEMANTIC_TAG, 
                                           name, strlen(name) + 1, ism);
    }

    //--------------------------------------------------------------------------
    void Runtime::attach_name(FieldSpace handle, const char *name, bool is_mut)
    //--------------------------------------------------------------------------
    {
      Runtime::attach_semantic_information(handle, LEGION_NAME_SEMANTIC_TAG, 
                                           name, strlen(name) + 1, is_mut);
    }

    //--------------------------------------------------------------------------
    void Runtime::attach_name(FieldSpace handle,
                                       FieldID fid,
                                       const char *name, bool is_mutable)
    //--------------------------------------------------------------------------
    {
      Runtime::attach_semantic_information(handle, fid, 
          LEGION_NAME_SEMANTIC_TAG, name, strlen(name) + 1, is_mutable);
    }

    //--------------------------------------------------------------------------
    void Runtime::attach_name(LogicalRegion handle, const char *name, bool ism)
    //--------------------------------------------------------------------------
    {
      Runtime::attach_semantic_information(handle,
          LEGION_NAME_SEMANTIC_TAG, name, strlen(name) + 1, ism);
    }

    //--------------------------------------------------------------------------
    void Runtime::attach_name(LogicalPartition handle, const char *name, bool m)
    //--------------------------------------------------------------------------
    {
      Runtime::attach_semantic_information(handle,
          LEGION_NAME_SEMANTIC_TAG, name, strlen(name) + 1, m);
    }

    //--------------------------------------------------------------------------
    bool Runtime::retrieve_semantic_information(TaskID task_id, SemanticTag tag,
                                              const void *&result, size_t &size,
                                                bool can_fail, bool wait_until)
    //--------------------------------------------------------------------------
    {
      return runtime->retrieve_semantic_information(task_id, tag, result, size,
                                                    can_fail, wait_until);
    }

    //--------------------------------------------------------------------------
    bool Runtime::retrieve_semantic_information(IndexSpace handle,
                                                         SemanticTag tag,
                                                         const void *&result,
                                                         size_t &size,
                                                         bool can_fail,
                                                         bool wait_until)
    //--------------------------------------------------------------------------
    {
      return runtime->retrieve_semantic_information(handle, tag, result, size,
                                                    can_fail, wait_until);
    }

    //--------------------------------------------------------------------------
    bool Runtime::retrieve_semantic_information(IndexPartition handle,
                                                         SemanticTag tag,
                                                         const void *&result,
                                                         size_t &size,
                                                         bool can_fail,
                                                         bool wait_until)
    //--------------------------------------------------------------------------
    {
      return runtime->retrieve_semantic_information(handle, tag, result, size,
                                                    can_fail, wait_until);
    }

    //--------------------------------------------------------------------------
    bool Runtime::retrieve_semantic_information(FieldSpace handle,
                                                         SemanticTag tag,
                                                         const void *&result,
                                                         size_t &size,
                                                         bool can_fail,
                                                         bool wait_until)
    //--------------------------------------------------------------------------
    {
      return runtime->retrieve_semantic_information(handle, tag, result, size,
                                                    can_fail, wait_until);
    }

    //--------------------------------------------------------------------------
    bool Runtime::retrieve_semantic_information(FieldSpace handle,
                                                         FieldID fid,
                                                         SemanticTag tag,
                                                         const void *&result,
                                                         size_t &size,
                                                         bool can_fail,
                                                         bool wait_until)
    //--------------------------------------------------------------------------
    {
      return runtime->retrieve_semantic_information(handle, fid, tag, result, 
                                                    size, can_fail, wait_until);
    }

    //--------------------------------------------------------------------------
    bool Runtime::retrieve_semantic_information(LogicalRegion handle,
                                                         SemanticTag tag,
                                                         const void *&result,
                                                         size_t &size,
                                                         bool can_fail,
                                                         bool wait_until)
    //--------------------------------------------------------------------------
    {
      return runtime->retrieve_semantic_information(handle, tag, result, size,
                                                    can_fail, wait_until);
    }

    //--------------------------------------------------------------------------
    bool Runtime::retrieve_semantic_information(LogicalPartition part,
                                                         SemanticTag tag,
                                                         const void *&result,
                                                         size_t &size,
                                                         bool can_fail,
                                                         bool wait_until)
    //--------------------------------------------------------------------------
    {
      return runtime->retrieve_semantic_information(part, tag, result, size,
                                                    can_fail, wait_until);
    }

    //--------------------------------------------------------------------------
    void Runtime::retrieve_name(TaskID task_id, const char *&result)
    //--------------------------------------------------------------------------
    {
      const void* dummy_ptr; size_t dummy_size;
      Runtime::retrieve_semantic_information(task_id, LEGION_NAME_SEMANTIC_TAG,
                                         dummy_ptr, dummy_size, false, false);
      static_assert(sizeof(dummy_ptr) == sizeof(result), "Fuck c++");
      memcpy(&result, &dummy_ptr, sizeof(result));
    }

    //--------------------------------------------------------------------------
    void Runtime::retrieve_name(IndexSpace handle, const char *&result)
    //--------------------------------------------------------------------------
    {
      const void* dummy_ptr; size_t dummy_size;
      Runtime::retrieve_semantic_information(handle,
          LEGION_NAME_SEMANTIC_TAG, dummy_ptr, dummy_size, false, false);
      static_assert(sizeof(dummy_ptr) == sizeof(result), "Fuck c++");
      memcpy(&result, &dummy_ptr, sizeof(result));
    }

    //--------------------------------------------------------------------------
    void Runtime::retrieve_name(IndexPartition handle, const char *&result)
    //--------------------------------------------------------------------------
    {
      const void* dummy_ptr; size_t dummy_size;
      Runtime::retrieve_semantic_information(handle,
          LEGION_NAME_SEMANTIC_TAG, dummy_ptr, dummy_size, false, false);
      static_assert(sizeof(dummy_ptr) == sizeof(result), "Fuck c++");
      memcpy(&result, &dummy_ptr, sizeof(result));
    }

    //--------------------------------------------------------------------------
    void Runtime::retrieve_name(FieldSpace handle, const char *&result)
    //--------------------------------------------------------------------------
    {
      const void* dummy_ptr; size_t dummy_size;
      Runtime::retrieve_semantic_information(handle,
          LEGION_NAME_SEMANTIC_TAG, dummy_ptr, dummy_size, false, false);
      static_assert(sizeof(dummy_ptr) == sizeof(result), "Fuck c++");
      memcpy(&result, &dummy_ptr, sizeof(result));
    }

    //--------------------------------------------------------------------------
    void Runtime::retrieve_name(FieldSpace handle,
                                         FieldID fid,
                                         const char *&result)
    //--------------------------------------------------------------------------
    {
      const void* dummy_ptr; size_t dummy_size;
      Runtime::retrieve_semantic_information(handle, fid,
          LEGION_NAME_SEMANTIC_TAG, dummy_ptr, dummy_size, false, false);
      static_assert(sizeof(dummy_ptr) == sizeof(result), "Fuck c++");
      memcpy(&result, &dummy_ptr, sizeof(result));
    }

    //--------------------------------------------------------------------------
    void Runtime::retrieve_name(LogicalRegion handle,
                                         const char *&result)
    //--------------------------------------------------------------------------
    {
      const void* dummy_ptr; size_t dummy_size;
      Runtime::retrieve_semantic_information(handle,
          LEGION_NAME_SEMANTIC_TAG, dummy_ptr, dummy_size, false, false);
      static_assert(sizeof(dummy_ptr) == sizeof(result), "Fuck c++");
      memcpy(&result, &dummy_ptr, sizeof(result));
    }

    //--------------------------------------------------------------------------
    void Runtime::retrieve_name(LogicalPartition part,
                                         const char *&result)
    //--------------------------------------------------------------------------
    {
      const void* dummy_ptr; size_t dummy_size;
      Runtime::retrieve_semantic_information(part,
          LEGION_NAME_SEMANTIC_TAG, dummy_ptr, dummy_size, false, false);
      static_assert(sizeof(dummy_ptr) == sizeof(result), "Fuck c++");
      memcpy(&result, &dummy_ptr, sizeof(result));
    }

    //--------------------------------------------------------------------------
    void Runtime::print_once(Context ctx, FILE *f, const char *message)
    //--------------------------------------------------------------------------
    {
      runtime->print_once(ctx, f, message);
    }

    //--------------------------------------------------------------------------
    void Runtime::log_once(Context ctx, Realm::LoggerMessage &message)
    //--------------------------------------------------------------------------
    {
      runtime->log_once(ctx, message);
    }

    //--------------------------------------------------------------------------
    Realm::RegionInstance Runtime::create_task_local_instance(Memory memory,
                                           Realm::InstanceLayoutGeneric *layout)
    //--------------------------------------------------------------------------
    {
      if (Internal::implicit_context == NULL)
        REPORT_LEGION_ERROR(ERROR_DEFERRED_ALLOCATION_FAILURE,
            "It is illegal to request the creation of DeferredBuffer, Deferred"
            "Value, or DeferredReduction objects outside of Legion tasks.")
      return 
         Internal::implicit_context->create_task_local_instance(memory, layout);
    }

    //--------------------------------------------------------------------------
    void Runtime::destroy_task_local_instance(Realm::RegionInstance instance)
    //--------------------------------------------------------------------------
    {
      if (Internal::implicit_context == NULL)
        REPORT_LEGION_ERROR(ERROR_DEFERRED_ALLOCATION_FAILURE,
            "It is illegal to request the destruction of DeferredBuffer, "
            "Deferred Value, or DeferredReduction objects outside of "
            "Legion tasks.")
      return
         Internal::implicit_context->destroy_task_local_instance(instance);
    }

    //--------------------------------------------------------------------------
    /*static*/ const char* Runtime::get_legion_version(void)
    //--------------------------------------------------------------------------
    {
      static const char *legion_runtime_version = LEGION_VERSION;
      return legion_runtime_version;
    }

    //--------------------------------------------------------------------------
    /*static*/ int Runtime::start(int argc, char **argv, bool background,
                                  bool default_mapper, bool filter)
    //--------------------------------------------------------------------------
    {
      return Internal::Runtime::start(argc, argv, background, 
                                      default_mapper, filter);
    }

    //--------------------------------------------------------------------------
    /*static*/ void Runtime::initialize(int *argc, char ***argv, 
                                        bool filter, bool parse)
    //--------------------------------------------------------------------------
    {
      Internal::Runtime::initialize(argc, argv, parse, filter);
    }

    //--------------------------------------------------------------------------
    /*static*/ int Runtime::wait_for_shutdown(void)
    //--------------------------------------------------------------------------
    {
      return Internal::Runtime::wait_for_shutdown();
    } 

    //--------------------------------------------------------------------------
    /*static*/ void Runtime::set_return_code(int return_code)
    //--------------------------------------------------------------------------
    {
      Internal::Runtime::set_return_code(return_code);
    }

    //--------------------------------------------------------------------------
    Future Runtime::launch_top_level_task(const TaskLauncher &launcher)
    //--------------------------------------------------------------------------
    {
      return runtime->launch_top_level_task(launcher);
    }

    //--------------------------------------------------------------------------
    Context Runtime::begin_implicit_task(TaskID top_task_id,
                                         MapperID top_mapper_id,
                                         Processor::Kind proc_kind,
                                         const char *task_name,
                                         bool control_replicable,
                                         unsigned shard_per_address_space,
                                         int shard_id, DomainPoint point)
    //--------------------------------------------------------------------------
    {
      return runtime->begin_implicit_task(top_task_id, top_mapper_id, proc_kind,
       task_name, control_replicable, shard_per_address_space, shard_id, point);
    }

    //--------------------------------------------------------------------------
    void Runtime::unbind_implicit_task_from_external_thread(Context ctx)
    //--------------------------------------------------------------------------
    {
      runtime->unbind_implicit_task_from_external_thread(ctx);
    }

    //--------------------------------------------------------------------------
    void Runtime::bind_implicit_task_to_external_thread(Context ctx)
    //--------------------------------------------------------------------------
    {
      runtime->bind_implicit_task_to_external_thread(ctx);
    }

    //--------------------------------------------------------------------------
    void Runtime::finish_implicit_task(Context ctx, Realm::Event effects)
    //--------------------------------------------------------------------------
    {
      runtime->finish_implicit_task(ctx, Internal::ApEvent(effects));
    } 

    //--------------------------------------------------------------------------
    /*static*/ void Runtime::set_top_level_task_id(TaskID top_id)
    //--------------------------------------------------------------------------
    {
      Internal::Runtime::set_top_level_task_id(top_id);
    }

    //--------------------------------------------------------------------------
    /*static*/ void Runtime::set_top_level_task_mapper_id(MapperID mapper_id)
    //--------------------------------------------------------------------------
    {
      Internal::Runtime::set_top_level_task_mapper_id(mapper_id);
    }

    //--------------------------------------------------------------------------
    /*static*/ size_t Runtime::get_maximum_dimension(void)
    //--------------------------------------------------------------------------
    {
      return LEGION_MAX_DIM;
    }

    //--------------------------------------------------------------------------
    /*static*/ void Runtime::configure_MPI_interoperability(int rank)
    //--------------------------------------------------------------------------
    {
      Internal::Runtime::configure_MPI_interoperability(rank);
    }

    //--------------------------------------------------------------------------
    /*static*/ LegionHandshake Runtime::create_external_handshake(
                bool init_in_ext, int ext_participants, int legion_participants)
    //--------------------------------------------------------------------------
    {
#ifdef DEBUG_LEGION
      assert(ext_participants > 0);
      assert(legion_participants > 0);
#endif
      LegionHandshake result(
          new Internal::LegionHandshakeImpl(init_in_ext,
                                       ext_participants, legion_participants));
      Internal::Runtime::register_handshake(result);
      return result;
    }

    //--------------------------------------------------------------------------
    /*static*/ MPILegionHandshake Runtime::create_handshake(bool init_in_MPI,
                                                        int mpi_participants,
                                                        int legion_participants)
    //--------------------------------------------------------------------------
    {
#ifdef DEBUG_LEGION
      assert(mpi_participants > 0);
      assert(legion_participants > 0);
#endif
      MPILegionHandshake result(
          new Internal::LegionHandshakeImpl(init_in_MPI,
                                       mpi_participants, legion_participants));
      Internal::Runtime::register_handshake(result);
      return result;
    }

    //--------------------------------------------------------------------------
    /*static*/ void Runtime::register_reduction_op(ReductionOpID redop_id,
                                                   ReductionOp *redop,
                                                   SerdezInitFnptr init_fnptr,
                                                   SerdezFoldFnptr fold_fnptr,
                                                   bool permit_duplicates)
    //--------------------------------------------------------------------------
    {
      Internal::Runtime::register_reduction_op(redop_id, redop, init_fnptr, 
                                               fold_fnptr, permit_duplicates);
    }

    //--------------------------------------------------------------------------
    /*static*/ const ReductionOp* Runtime::get_reduction_op(
                                                        ReductionOpID redop_id)
    //--------------------------------------------------------------------------
    {
      return Internal::Runtime::get_reduction_op(redop_id);
    }

    //--------------------------------------------------------------------------
    /*static*/ void Runtime::register_custom_serdez_op(CustomSerdezID serdez_id,
                                                       SerdezOp *serdez_op,
                                                       bool permit_duplicates)
    //--------------------------------------------------------------------------
    {
      Internal::Runtime::register_serdez_op(serdez_id, serdez_op,
                                            permit_duplicates);
    }

    //--------------------------------------------------------------------------
    /*static*/ const SerdezOp* Runtime::get_serdez_op(CustomSerdezID serdez_id)
    //--------------------------------------------------------------------------
    {
      return Internal::Runtime::get_serdez_op(serdez_id);
    } 

    //--------------------------------------------------------------------------
    /*static*/ void Runtime::add_registration_callback(
                                             RegistrationCallbackFnptr callback,
                                             bool dedup, size_t dedup_tag)
    //--------------------------------------------------------------------------
    {
      Internal::Runtime::add_registration_callback(callback, dedup, dedup_tag);
    }

    //--------------------------------------------------------------------------
    /*static*/ void Runtime::add_registration_callback(
                       RegistrationWithArgsCallbackFnptr callback, 
                       const UntypedBuffer &buffer, bool dedup, size_t dedup_tag)
    //--------------------------------------------------------------------------
    {
      Internal::Runtime::add_registration_callback(callback, buffer, 
                                                   dedup, dedup_tag);
    }

    //--------------------------------------------------------------------------
    void Runtime::perform_registration_callback(
                                RegistrationCallbackFnptr callback, bool global,
                                bool deduplicate, size_t dedup_tag)
    //--------------------------------------------------------------------------
    {
      Internal::Runtime::perform_dynamic_registration_callback(callback,
                                        global, deduplicate, dedup_tag);
    }

    //--------------------------------------------------------------------------
    void Runtime::perform_registration_callback(
                                     RegistrationWithArgsCallbackFnptr callback,
                                     const UntypedBuffer &buffer, bool global,
                                     bool deduplicate, size_t dedup_tag)
    //--------------------------------------------------------------------------
    {
      Internal::Runtime::perform_dynamic_registration_callback(callback, buffer,
                                                global, deduplicate, dedup_tag);
    }

    //--------------------------------------------------------------------------
    /*static*/ void Runtime::set_registration_callback(
                                            RegistrationCallbackFnptr callback)
    //--------------------------------------------------------------------------
    {
      Internal::Runtime::add_registration_callback(callback, true/*dedup*/, 0);
    }

    //--------------------------------------------------------------------------
    /*static*/ const InputArgs& Runtime::get_input_args(void)
    //--------------------------------------------------------------------------
    {
      if (!Internal::Runtime::runtime_started)
        REPORT_LEGION_ERROR(ERROR_DYNAMIC_CALL_PRE_RUNTIME_START,
            "Illegal call to 'get_input_args' before the runtime is started")
      if (Internal::implicit_runtime != NULL)
        return Internal::implicit_runtime->input_args;
      // Otherwise this is not from a Legion task, so fallback to the_runtime
      return Internal::Runtime::the_runtime->input_args;
    }

    //--------------------------------------------------------------------------
    /*static*/ bool Runtime::has_runtime(void)
    //--------------------------------------------------------------------------
    {
      return Internal::Runtime::runtime_started;
    }

    //--------------------------------------------------------------------------
    /*static*/ Runtime* Runtime::get_runtime(Processor p)
    //--------------------------------------------------------------------------
    {
      if (!Internal::Runtime::runtime_started)
        REPORT_LEGION_ERROR(ERROR_DYNAMIC_CALL_PRE_RUNTIME_START,
            "Illegal call to 'get_runtime' before the runtime is started")
      // If we have an implicit runtime we use that
      if (Internal::implicit_runtime != NULL)
        return Internal::implicit_runtime->external;
      // Otherwise this is not from a Legion task, so fallback to the_runtime
      return Internal::Runtime::the_runtime->external;
    }

    //--------------------------------------------------------------------------
    /*static*/ bool Runtime::has_context(void)
    //--------------------------------------------------------------------------
    {
      return (Internal::implicit_context != NULL);
    }

    //--------------------------------------------------------------------------
    /*static*/ Context Runtime::get_context(void)
    //--------------------------------------------------------------------------
    {
      return Internal::implicit_context;
    } 

    //--------------------------------------------------------------------------
    /*static*/ const Task* Runtime::get_context_task(Context ctx)
    //--------------------------------------------------------------------------
    {
      return ctx->get_owner_task();
    }

    //--------------------------------------------------------------------------
    TaskID Runtime::generate_dynamic_task_id(void)
    //--------------------------------------------------------------------------
    {
      return runtime->generate_dynamic_task_id();
    }

    //--------------------------------------------------------------------------
    TaskID Runtime::generate_library_task_ids(const char *name, size_t count)
    //--------------------------------------------------------------------------
    {
      return runtime->generate_library_task_ids(name, count);
    }

    //--------------------------------------------------------------------------
    /*static*/ TaskID Runtime::generate_static_task_id(void)
    //--------------------------------------------------------------------------
    {
      return Internal::Runtime::generate_static_task_id();
    }

    //--------------------------------------------------------------------------
    ReductionOpID Runtime::generate_dynamic_reduction_id(void)
    //--------------------------------------------------------------------------
    {
      return runtime->generate_dynamic_reduction_id();
    }

    //--------------------------------------------------------------------------
    ReductionOpID Runtime::generate_library_reduction_ids(const char *name,
                                                          size_t count)
    //--------------------------------------------------------------------------
    {
      return runtime->generate_library_reduction_ids(name, count);
    }

    //--------------------------------------------------------------------------
    /*static*/ ReductionOpID Runtime::generate_static_reduction_id(void)
    //--------------------------------------------------------------------------
    {
      return Internal::Runtime::generate_static_reduction_id();
    }

    //--------------------------------------------------------------------------
    CustomSerdezID Runtime::generate_dynamic_serdez_id(void)
    //--------------------------------------------------------------------------
    {
      return runtime->generate_dynamic_serdez_id();
    }

    //--------------------------------------------------------------------------
    CustomSerdezID Runtime::generate_library_serdez_ids(const char *name,
                                                        size_t count)
    //--------------------------------------------------------------------------
    {
      return runtime->generate_library_serdez_ids(name, count);
    }

    //--------------------------------------------------------------------------
    /*static*/ CustomSerdezID Runtime::generate_static_serdez_id(void)
    //--------------------------------------------------------------------------
    {
      return Internal::Runtime::generate_static_serdez_id();
    }

    //--------------------------------------------------------------------------
    VariantID Runtime::register_task_variant(
                                    const TaskVariantRegistrar &registrar,
                                    const CodeDescriptor &realm_desc,
                                    const void *user_data /*= NULL*/,
                                    size_t user_len /*= 0*/,
                                    size_t return_type_size/*=MAX_RETURN_SIZE*/,
                                    VariantID vid /*= AUTO_GENERATE_ID*/,
                                    bool has_return_type_size /*= true*/)
    //--------------------------------------------------------------------------
    {
      return runtime->register_variant(registrar, user_data, user_len, 
             realm_desc, return_type_size, has_return_type_size, vid);
    }

    //--------------------------------------------------------------------------
    /*static*/ VariantID Runtime::preregister_task_variant(
              const TaskVariantRegistrar &registrar,
	      const CodeDescriptor &realm_desc,
	      const void *user_data /*= NULL*/,
	      size_t user_len /*= 0*/,
	      const char *task_name /*= NULL*/,
              VariantID vid /*=AUTO_GENERATE_ID*/,
              size_t return_type_size /*=MAX_RETURN_SIZE*/,
              bool has_return_type_size /*=true*/,
              bool check_task_id/*=true*/)
    //--------------------------------------------------------------------------
    {
      // Make a copy of the descriptor here
      return Internal::Runtime::preregister_variant(registrar, user_data, 
             user_len, realm_desc, return_type_size, has_return_type_size,
             task_name, vid, check_task_id);
    }

    //--------------------------------------------------------------------------
    /*static*/ void Runtime::legion_task_preamble(
                                       const void *data, size_t datalen,
                                       Processor p, const Task *& task,
                                       const std::vector<PhysicalRegion> *& reg,
                                       Context& ctx, Runtime *& runtime)
    //--------------------------------------------------------------------------
    {
      // Read the context out of the buffer
#ifdef DEBUG_LEGION
      assert(datalen == sizeof(Context));
#endif
      ctx = *((const Context*)data);
      task = ctx->get_task();
      const Processor exec_proc = Processor::get_executing_processor();
#ifdef DEBUG_LEGION
      assert(exec_proc.exists());
#endif
      reg = &ctx->begin_task(exec_proc);
      runtime = Internal::implicit_runtime->external;
    }

    //--------------------------------------------------------------------------
    /*static*/ void Runtime::legion_task_postamble(Context ctx,
                                                 const void *retvalptr,
                                                 size_t retvalsize, bool owned,
                                                 Realm::RegionInstance inst,
                                                 const void *metadataptr,
                                                 size_t metadatasize)
    //--------------------------------------------------------------------------
    {
      ctx->end_task(retvalptr, retvalsize, owned, inst, NULL/*functor*/,
          NULL/*resource*/, NULL/*freefunc*/, metadataptr, metadatasize,
          Internal::ApEvent::NO_AP_EVENT);
    }

    //--------------------------------------------------------------------------
    /*static*/ void Runtime::legion_task_postamble(Context ctx,
                                    FutureFunctor *callback_functor, bool owned)
    //--------------------------------------------------------------------------
    {
      ctx->end_task(NULL, 0, owned, Realm::RegionInstance::NO_INST,
          callback_functor, NULL/*resource*/, NULL/*freefunc*/, NULL, 0,
          Internal::ApEvent::NO_AP_EVENT);
    }

    //--------------------------------------------------------------------------
    /*static*/ void Runtime::legion_task_postamble(Context ctx,
        const void *ptr, size_t size, bool owned,
        const Realm::ExternalInstanceResource &resource,
        void (*freefunc)(const Realm::ExternalInstanceResource&),
        const void *metadataptr, size_t metadatasize)
    //--------------------------------------------------------------------------
    {
      ctx->end_task(ptr, size, owned, Realm::RegionInstance::NO_INST,
          NULL/*functor*/, &resource, freefunc, metadataptr, metadatasize,
          Internal::ApEvent::NO_AP_EVENT);
    }

    //--------------------------------------------------------------------------
    ShardID Runtime::get_shard_id(Context ctx, bool I_know_what_I_am_doing)
    //--------------------------------------------------------------------------
    {
      if (!I_know_what_I_am_doing)
        REPORT_LEGION_ERROR(ERROR_CONFUSED_USER, "User does not know what "
            "they are doing asking for the shard ID in task %s (UID %lld)",
            ctx->get_task_name(), ctx->get_unique_id())
      const Task *task = get_local_task(ctx);
      return task->get_shard_id();
    }

    //--------------------------------------------------------------------------
    size_t Runtime::get_num_shards(Context ctx, bool I_know_what_I_am_doing)
    //--------------------------------------------------------------------------
    {
      if (!I_know_what_I_am_doing)
        REPORT_LEGION_ERROR(ERROR_CONFUSED_USER, "User does not know what they"
            " are doing asking for the number of shards in task %s (UID %lld)",
            ctx->get_task_name(), ctx->get_unique_id())
      const Task *task = get_local_task(ctx);
      return task->get_total_shards();
    }

    //--------------------------------------------------------------------------
    Future Runtime::consensus_match(Context ctx, const void *input,void *output,
                     size_t num_elements, size_t element_size, const char *prov)
    //--------------------------------------------------------------------------
    {
      Internal::AutoProvenance provenance(prov);
      return ctx->consensus_match(input, output, num_elements, 
                                  element_size, provenance);
    }

    //--------------------------------------------------------------------------
    /*static*/ void Runtime::enable_profiling(void)
    //--------------------------------------------------------------------------
    {
    }

    //--------------------------------------------------------------------------
    /*static*/ void Runtime::disable_profiling(void)
    //--------------------------------------------------------------------------
    {
    }

    //--------------------------------------------------------------------------
    /*static*/ void Runtime::dump_profiling(void)
    //--------------------------------------------------------------------------
    {
    }

    //--------------------------------------------------------------------------
    LayoutConstraintID Runtime::register_layout(
                                     const LayoutConstraintRegistrar &registrar)
    //--------------------------------------------------------------------------
    {
      return runtime->register_layout(registrar, LEGION_AUTO_GENERATE_ID);
    }

    //--------------------------------------------------------------------------
    void Runtime::release_layout(LayoutConstraintID layout_id)
    //--------------------------------------------------------------------------
    {
      runtime->release_layout(layout_id);
    }

    //--------------------------------------------------------------------------
    /*static*/ LayoutConstraintID Runtime::preregister_layout(
                                     const LayoutConstraintRegistrar &registrar,
                                     LayoutConstraintID layout_id)
    //--------------------------------------------------------------------------
    {
      return Internal::Runtime::preregister_layout(registrar, layout_id);
    }

    //--------------------------------------------------------------------------
    FieldSpace Runtime::get_layout_constraint_field_space(
                                                   LayoutConstraintID layout_id)
    //--------------------------------------------------------------------------
    {
      return runtime->get_layout_constraint_field_space(layout_id);
    }

    //--------------------------------------------------------------------------
    void Runtime::get_layout_constraints(LayoutConstraintID layout_id,
                                        LayoutConstraintSet &layout_constraints)
    //--------------------------------------------------------------------------
    {
      runtime->get_layout_constraints(layout_id, layout_constraints);
    }

    //--------------------------------------------------------------------------
    const char* Runtime::get_layout_constraints_name(LayoutConstraintID id)
    //--------------------------------------------------------------------------
    {
      return runtime->get_layout_constraints_name(id);
    }

}; // namespace Legion

// EOF
