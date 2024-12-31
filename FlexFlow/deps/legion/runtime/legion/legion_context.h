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

#ifndef __LEGION_CONTEXT_H__
#define __LEGION_CONTEXT_H__

#include "legion.h"
#include "legion/legion_tasks.h"
#include "legion/legion_mapping.h"
#include "legion/legion_instances.h"
#include "legion/legion_allocation.h"

namespace Legion {
  namespace Internal {
   
    /**
     * \class TaskContext
     * The base class for all task contexts which 
     * provide all the methods for handling the 
     * execution of a task at runtime.
     */
    class TaskContext : public DistributedCollectable {
    public:
      class AutoRuntimeCall {
      public:
        AutoRuntimeCall(TaskContext *c) : ctx(c) { ctx->begin_runtime_call(); }
        ~AutoRuntimeCall(void) { ctx->end_runtime_call(); }
      public:
        TaskContext *const ctx;
      };
      // This is a no-op task for yield operations
      struct YieldArgs : public LgTaskArgs<YieldArgs> {
      public:
        static const LgTaskID TASK_ID = LG_YIELD_TASK_ID;
      public:
        YieldArgs(UniqueID uid) : LgTaskArgs<YieldArgs>(uid) { }
      };
    public:
      TaskContext(Runtime *runtime, SingleTask *owner, int depth,
                  const std::vector<RegionRequirement> &reqs,
                  const std::vector<OutputRequirement> &output_reqs,
                  DistributedID did, bool perform_registration,
                  bool inline_task, bool implicit_ctx = false,
                  CollectiveMapping *mapping = NULL);
      virtual ~TaskContext(void);
    public:
      // This is used enough that we want it inlined
      inline Processor get_executing_processor(void) const
        { return executing_processor; }
      inline void set_executing_processor(Processor p)
        { executing_processor = p; }
      inline const char* get_task_name(void)
        { return get_task()->get_task_name(); }
      inline const std::vector<PhysicalRegion>& get_physical_regions(void) const
        { return physical_regions; }
      inline SingleTask* get_owner_task(void) const { return owner_task; }
      inline bool is_priority_mutable(void) const { return mutable_priority; }
      inline int get_depth(void) const { return depth; }
    public:
      virtual ShardID get_shard_id(void) const { return 0; }
      virtual DistributedID get_replication_id(void) const { return 0; }
      virtual size_t get_total_shards(void) const { return 1; }
    public:
      // Interface for task contexts
      virtual ContextID get_logical_tree_context(void) const = 0;
      virtual ContextID get_physical_tree_context(void) const = 0;
      virtual Task* get_task(void); 
      virtual UniqueID get_unique_id(void) const;
      virtual InnerContext* find_parent_context(void);
      virtual void compute_task_tree_coordinates(
                TaskTreeCoordinates &coords) const = 0;
      virtual bool attempt_children_complete(void) = 0;
      virtual bool attempt_children_commit(void) = 0;
      virtual VariantImpl* select_inline_variant(TaskOp *child,
                const std::vector<PhysicalRegion> &parent_regions,
                std::deque<InstanceSet> &physical_instances);
      virtual bool is_leaf_context(void) const;
      virtual bool is_inner_context(void) const;
#ifdef LEGION_USE_LIBDL
      virtual void perform_global_registration_callbacks(
                     Realm::DSOReferenceImplementation *dso, const void *buffer,
                     size_t buffer_size, bool withargs, size_t dedup_tag,
                     RtEvent local_done, RtEvent global_done, 
                     std::set<RtEvent> &preconditions);
#endif
      virtual void print_once(FILE *f, const char *message) const;
      virtual void log_once(Realm::LoggerMessage &message) const;
      virtual Future from_value(const void *value, size_t value_size,
          bool owned, Provenance *provenance, bool shard_local);
      virtual Future from_value(const void *value, size_t size, bool owned,
          const Realm::ExternalInstanceResource &resource,
          void (*freefunc)(const Realm::ExternalInstanceResource&),
          Provenance *provenance, bool shard_local);
      virtual Future consensus_match(const void *input, void *output,
          size_t num_elements, size_t element_size, Provenance *provenance);
    public:
      virtual VariantID register_variant(const TaskVariantRegistrar &registrar,
                          const void *user_data, size_t user_data_size,
                          const CodeDescriptor &desc, size_t ret_size,
                          bool has_ret_size, VariantID vid, bool check_task_id);
      virtual TraceID generate_dynamic_trace_id(void);
      virtual MapperID generate_dynamic_mapper_id(void);
      virtual ProjectionID generate_dynamic_projection_id(void);
      virtual ShardingID generate_dynamic_sharding_id(void);
      virtual TaskID generate_dynamic_task_id(void);
      virtual ReductionOpID generate_dynamic_reduction_id(void);
      virtual CustomSerdezID generate_dynamic_serdez_id(void);
      virtual bool perform_semantic_attach(const char *func, unsigned kind,
          const void *arg, size_t arglen, SemanticTag tag, const void *buffer,
          size_t size, bool is_mutable, bool &global, 
          const void *arg2 = NULL, size_t arg2len = 0);
      virtual void post_semantic_attach(void);
    public:
      virtual void return_resources(ResourceTracker *target,
                                    size_t return_index,
                                    std::set<RtEvent> &preconditions) = 0;
      virtual void pack_return_resources(Serializer &rez,
                                         size_t return_index) = 0;
      virtual void log_created_requirements(void) = 0;
      virtual void report_leaks_and_duplicates(
          std::set<RtEvent> &preconditions) = 0;
    public:
      // Interface to operations performed by a context 
      virtual IndexSpace create_index_space(const Domain &bounds,
                                            TypeTag type_tag,
                                            Provenance *provenance) = 0;
      virtual IndexSpace create_index_space(const Future &future,
                                            TypeTag type_tag,
                                            Provenance *provenance) = 0;
      virtual IndexSpace create_index_space(
                           const std::vector<DomainPoint> &points,
                           Provenance *provenance) = 0;
      virtual IndexSpace create_index_space(
                           const std::vector<Domain> &rects,
                           Provenance *provenance) = 0;
      // This variant creates an uninitialized index space
      // that later is set by a task
      virtual IndexSpace create_unbound_index_space(TypeTag type_tag,
                                              Provenance *provenance) = 0;
    public:
      virtual IndexSpace union_index_spaces(
                           const std::vector<IndexSpace> &spaces,
                           Provenance *provenance) = 0;
      virtual IndexSpace intersect_index_spaces(
                           const std::vector<IndexSpace> &spaces,
                           Provenance *provenance) = 0;
      virtual IndexSpace subtract_index_spaces(
                           IndexSpace left, IndexSpace right,
                           Provenance *provenance) = 0;
      virtual void create_shared_ownership(IndexSpace handle) = 0;
      virtual void destroy_index_space(IndexSpace handle,
                                       const bool unordered,
                                       const bool recurse,
                                       Provenance *provenance) = 0;
      virtual void create_shared_ownership(IndexPartition handle) = 0;
      virtual void destroy_index_partition(IndexPartition handle,
                                           const bool unordered,
                                           const bool recurse,
                                           Provenance *provenance) = 0;
      virtual IndexPartition create_equal_partition(
                                            IndexSpace parent,
                                            IndexSpace color_space,
                                            size_t granularity,
                                            Color color,
                                            Provenance *provenance) = 0;
      virtual IndexPartition create_partition_by_weights(IndexSpace parent,
                                            const FutureMap &weights,
                                            IndexSpace color_space,
                                            size_t granularity, 
                                            Color color,
                                            Provenance *provenance) = 0;
      virtual IndexPartition create_partition_by_union(
                                            IndexSpace parent,
                                            IndexPartition handle1,
                                            IndexPartition handle2,
                                            IndexSpace color_space,
                                            PartitionKind kind,
                                            Color color,
                                            Provenance *provenance) = 0;
      virtual IndexPartition create_partition_by_intersection(
                                            IndexSpace parent,
                                            IndexPartition handle1,
                                            IndexPartition handle2,
                                            IndexSpace color_space,
                                            PartitionKind kind,
                                            Color color,
                                            Provenance *provenance) = 0;
      virtual IndexPartition create_partition_by_intersection(
                                            IndexSpace parent,
                                            IndexPartition partition,
                                            PartitionKind kind,
                                            Color color, 
                                            bool dominates,
                                            Provenance *provenance) = 0;
      virtual IndexPartition create_partition_by_difference(
                                            IndexSpace parent,
                                            IndexPartition handle1,
                                            IndexPartition handle2,
                                            IndexSpace color_space,
                                            PartitionKind kind,
                                            Color color,
                                            Provenance *provenance) = 0;
      virtual Color create_cross_product_partitions(
                                            IndexPartition handle1,
                                            IndexPartition handle2,
                              std::map<IndexSpace,IndexPartition> &handles,
                                            PartitionKind kind,
                                            Color color,
                                            Provenance *provenance) = 0;
      virtual void create_association(      LogicalRegion domain,
                                            LogicalRegion domain_parent,
                                            FieldID domain_fid,
                                            IndexSpace range,
                                            MapperID id, MappingTagID tag,
                                            const UntypedBuffer &marg,
                                            Provenance *prov) = 0;
      virtual IndexPartition create_restricted_partition(
                                            IndexSpace parent,
                                            IndexSpace color_space,
                                            const void *transform,
                                            size_t transform_size,
                                            const void *extent,
                                            size_t extent_size,
                                            PartitionKind part_kind,
                                            Color color,
                                            Provenance *provenance) = 0;
      virtual IndexPartition create_partition_by_domain(
                                            IndexSpace parent,
                                  const std::map<DomainPoint,Domain> &domains,
                                            IndexSpace color_space,
                                            bool perform_intersections,
                                            PartitionKind part_kind,
                                            Color color,
                                            Provenance *provenance) = 0;
      virtual IndexPartition create_partition_by_domain(
                                            IndexSpace parent,
                                            const FutureMap &domains,
                                            IndexSpace color_space,
                                            bool perform_intersections,
                                            PartitionKind part_kind,
                                            Color color, 
                                            Provenance *provenance,
                                            bool skip_check = false) = 0;
      virtual IndexPartition create_partition_by_field(
                                            LogicalRegion handle,
                                            LogicalRegion parent_priv,
                                            FieldID fid,
                                            IndexSpace color_space,
                                            Color color,
                                            MapperID id, MappingTagID tag,
                                            PartitionKind part_kind,
                                            const UntypedBuffer &marg,
                                            Provenance *prov) = 0;
      virtual IndexPartition create_partition_by_image(
                                            IndexSpace handle,
                                            LogicalPartition projection,
                                            LogicalRegion parent,
                                            FieldID fid,
                                            IndexSpace color_space,
                                            PartitionKind part_kind,
                                            Color color,
                                            MapperID id, MappingTagID tag,
                                            const UntypedBuffer &marg,
                                            Provenance *prov) = 0;
      virtual IndexPartition create_partition_by_image_range(
                                            IndexSpace handle,
                                            LogicalPartition projection,
                                            LogicalRegion parent,
                                            FieldID fid,
                                            IndexSpace color_space,
                                            PartitionKind part_kind,
                                            Color color,
                                            MapperID id, MappingTagID tag,
                                            const UntypedBuffer &marg,
                                            Provenance *prov) = 0;
      virtual IndexPartition create_partition_by_preimage(
                                            IndexPartition projection,
                                            LogicalRegion handle,
                                            LogicalRegion parent,
                                            FieldID fid,
                                            IndexSpace color_space,
                                            PartitionKind part_kind,
                                            Color color,
                                            MapperID id, MappingTagID tag,
                                            const UntypedBuffer &marg,
                                            Provenance *prov) = 0;
      virtual IndexPartition create_partition_by_preimage_range(
                                            IndexPartition projection,
                                            LogicalRegion handle,
                                            LogicalRegion parent,
                                            FieldID fid,
                                            IndexSpace color_space,
                                            PartitionKind part_kind,
                                            Color color,
                                            MapperID id, MappingTagID tag,
                                            const UntypedBuffer &marg,
                                            Provenance *prov) = 0;
      virtual IndexPartition create_pending_partition(
                                            IndexSpace parent,
                                            IndexSpace color_space,
                                            PartitionKind part_kind,
                                            Color color,
                                            Provenance *prov,
                                            bool trust = false) = 0;
      virtual IndexSpace create_index_space_union(
                                            IndexPartition parent,
                                            const void *realm_color,
                                            size_t color_size,
                                            TypeTag type_tag,
                                const std::vector<IndexSpace> &handles,
                                            Provenance *provenance) = 0;
      virtual IndexSpace create_index_space_union(
                                            IndexPartition parent,
                                            const void *realm_color,
                                            size_t color_size,
                                            TypeTag type_tag,
                                            IndexPartition handle,
                                            Provenance *provenance) = 0;
      virtual IndexSpace create_index_space_intersection(
                                            IndexPartition parent,
                                            const void *realm_color,
                                            size_t color_size,
                                            TypeTag type_tag,
                                const std::vector<IndexSpace> &handles,
                                            Provenance *provenance) = 0;
      virtual IndexSpace create_index_space_intersection(
                                            IndexPartition parent,
                                            const void *realm_color,
                                            size_t color_size,
                                            TypeTag type_tag,
                                            IndexPartition handle,
                                            Provenance *provenance) = 0;
      virtual IndexSpace create_index_space_difference(
                                            IndexPartition parent,
                                            const void *realm_color,
                                            size_t color_size,
                                            TypeTag type_tag,
                                            IndexSpace initial,
                                const std::vector<IndexSpace> &handles,
                                            Provenance *provenance) = 0;
      virtual FieldSpace create_field_space(Provenance *provenance) = 0;
      virtual FieldSpace create_field_space(const std::vector<size_t> &sizes,
                                        std::vector<FieldID> &resulting_fields,
                                        CustomSerdezID serdez_id,
                                        Provenance *provenance) = 0;
      virtual FieldSpace create_field_space(const std::vector<Future> &sizes,
                                        std::vector<FieldID> &resulting_fields,
                                        CustomSerdezID serdez_id,
                                        Provenance *provenance) = 0;
      virtual void create_shared_ownership(FieldSpace handle) = 0;
      virtual void destroy_field_space(FieldSpace handle,
                                       const bool unordered,
                                       Provenance *provenance) = 0;
      virtual FieldID allocate_field(FieldSpace space, size_t field_size,
                                     FieldID fid, bool local,
                                     CustomSerdezID serdez_id,
                                     Provenance *provenance) = 0;
      virtual FieldID allocate_field(FieldSpace space, const Future &field_size,
                                     FieldID fid, bool local,
                                     CustomSerdezID serdez_id,
                                     Provenance *provenance) = 0;
      virtual void allocate_local_field(
                                     FieldSpace space, size_t field_size,
                                     FieldID fid, CustomSerdezID serdez_id,
                                     std::set<RtEvent> &done_events,
                                     Provenance *provenance) = 0;
      virtual void free_field(FieldAllocatorImpl *allocator, FieldSpace space, 
                              FieldID fid, const bool unordered,
                              Provenance *provenance) = 0;
      virtual void allocate_fields(FieldSpace space,
                                   const std::vector<size_t> &sizes,
                                   std::vector<FieldID> &resuling_fields,
                                   bool local, CustomSerdezID serdez_id,
                                   Provenance *provenance) = 0;
      virtual void allocate_fields(FieldSpace space,
                                   const std::vector<Future> &sizes,
                                   std::vector<FieldID> &resuling_fields,
                                   bool local, CustomSerdezID serdez_id,
                                   Provenance *provenance) = 0;
      virtual void allocate_local_fields(FieldSpace space,
                                   const std::vector<size_t> &sizes,
                                   const std::vector<FieldID> &resuling_fields,
                                   CustomSerdezID serdez_id,
                                   std::set<RtEvent> &done_events,
                                   Provenance *provenance) = 0;
      virtual void free_fields(FieldAllocatorImpl *allocator, FieldSpace space, 
                               const std::set<FieldID> &to_free,
                               const bool unordered,
                               Provenance *provenance) = 0; 
      virtual LogicalRegion create_logical_region(
                                          IndexSpace index_space,
                                          FieldSpace field_space,
                                          const bool task_local,
                                          Provenance *provenance,
                                          const bool output_region = false) = 0;
      virtual void create_shared_ownership(LogicalRegion handle) = 0;
      virtual void destroy_logical_region(LogicalRegion handle,
                                          const bool unordered,
                                          Provenance *provenance) = 0;
      virtual void reset_equivalence_sets(LogicalRegion parent, 
                                          LogicalRegion region,
                                          const std::set<FieldID> &fields) = 0;
      virtual FieldAllocatorImpl* create_field_allocator(FieldSpace handle,
                                                         bool unordered) = 0;
      virtual void destroy_field_allocator(FieldSpaceNode *node, 
                                           bool from_application = true) = 0;
      virtual void get_local_field_set(const FieldSpace handle,
                                       const std::set<unsigned> &indexes,
                                       std::set<FieldID> &to_set) const = 0;
      virtual void get_local_field_set(const FieldSpace handle,
                                       const std::set<unsigned> &indexes,
                                       std::vector<FieldID> &to_set) const = 0;
    public:
      virtual void add_physical_region(const RegionRequirement &req, 
          bool mapped, MapperID mid, MappingTagID tag, ApUserEvent &unmap_event,
          bool virtual_mapped, const InstanceSet &physical_instances) = 0;
      virtual Future execute_task(const TaskLauncher &launcher,
                                  std::vector<OutputRequirement> *outputs) = 0;
      virtual FutureMap execute_index_space(const IndexTaskLauncher &launcher,
                                   std::vector<OutputRequirement> *outputs) = 0;
      virtual Future execute_index_space(const IndexTaskLauncher &launcher,
                                   ReductionOpID redop, bool deterministic,
                                   std::vector<OutputRequirement> *outputs) = 0;
      virtual Future reduce_future_map(const FutureMap &future_map,
                                   ReductionOpID redop, bool deterministic,
                                   MapperID map_id, MappingTagID tag,
                                   Provenance *provenance,
                                   Future initial_value) = 0;
      virtual FutureMap construct_future_map(IndexSpace domain,
                               const std::map<DomainPoint,UntypedBuffer> &data,
                                             Provenance *provenance,
                                             bool collective = false,
                                             ShardingID sid = 0,
                                             bool implicit = false,
                                             bool internal = false,
                                             bool check_space = true) = 0;
      virtual FutureMap construct_future_map(const Domain &domain,
                                const std::map<DomainPoint,UntypedBuffer> &data,
                                             bool collective = false,
                                             ShardingID sid = 0,
                                             bool implicit = false) = 0;
      virtual FutureMap construct_future_map(IndexSpace domain,
                               const std::map<DomainPoint,Future> &futures,
                                             Provenance *provenance,
                                             bool internal = false,
                                             bool collective = false,
                                             ShardingID sid = 0,
                                             bool implicit = false,
                                             bool check_space = true) = 0;
      virtual FutureMap construct_future_map(const Domain &domain,
                    const std::map<DomainPoint,Future> &futures,
                                             bool internal = false,
                                             bool collective = false,
                                             ShardingID sid = 0,
                                             bool implicit = false) = 0;
      virtual FutureMap transform_future_map(const FutureMap &fm,
                                             IndexSpace new_domain, 
                      TransformFutureMapImpl::PointTransformFnptr fnptr,
                                             Provenance *provenance) = 0;
      virtual FutureMap transform_future_map(const FutureMap &fm,
                                             IndexSpace new_domain,
                                             PointTransformFunctor *functor,
                                             bool own_functor,
                                             Provenance *provenance) = 0;
      virtual PhysicalRegion map_region(const InlineLauncher &launcher) = 0;
      virtual ApEvent remap_region(const PhysicalRegion &region,
                                   Provenance *provenance,
                                   bool internal = false) = 0;
      virtual void unmap_region(PhysicalRegion region) = 0;
      virtual void unmap_all_regions(bool external) = 0;
      virtual void fill_fields(const FillLauncher &launcher) = 0;
      virtual void fill_fields(const IndexFillLauncher &launcher) = 0;
      virtual void discard_fields(const DiscardLauncher &launcher) = 0;
      virtual void issue_copy(const CopyLauncher &launcher) = 0;
      virtual void issue_copy(const IndexCopyLauncher &launcher) = 0;
      virtual void issue_acquire(const AcquireLauncher &launcher) = 0;
      virtual void issue_release(const ReleaseLauncher &launcher) = 0;
      virtual PhysicalRegion attach_resource(
                                  const AttachLauncher &launcher) = 0;
      virtual ExternalResources attach_resources(
                                  const IndexAttachLauncher &launcher) = 0;
      virtual Future detach_resource(PhysicalRegion region, 
                                     const bool flush,const bool unordered,
                                     Provenance *provenance = NULL) = 0;
      virtual Future detach_resources(ExternalResources resources,
                                    const bool flush, const bool unordered,
                                    Provenance *provenance) = 0;
      virtual void progress_unordered_operations(bool end_task = false) = 0;
      virtual FutureMap execute_must_epoch(
                                 const MustEpochLauncher &launcher) = 0;
      virtual Future issue_timing_measurement(
                                    const TimingLauncher &launcher) = 0;
      virtual Future select_tunable_value(const TunableLauncher &launcher) = 0;
      virtual Future issue_mapping_fence(Provenance *provenance) = 0;
      virtual Future issue_execution_fence(Provenance *provenance) = 0;
      virtual void complete_frame(Provenance *provenance) = 0;
      virtual Predicate create_predicate(const Future &f,
                                         Provenance *provenance) = 0;
      virtual Predicate predicate_not(const Predicate &p,
                                      Provenance *provenance) = 0;
      virtual Predicate create_predicate(const PredicateLauncher &launcher) = 0;
      virtual Future get_predicate_future(const Predicate &p,
                                          Provenance *provenance) = 0;
    public:
      virtual void begin_trace(TraceID tid, bool logical_only,
        bool static_trace, const std::set<RegionTreeID> *managed, bool dep,
        Provenance *provenance) = 0;
      virtual void end_trace(TraceID tid, bool deprecated,
                             Provenance *provenance) = 0;
      virtual void record_previous_trace(LogicalTrace *trace) = 0;
      virtual void invalidate_trace_cache(LogicalTrace *trace,
                                          Operation *invalidator) = 0;
      virtual void record_blocking_call(void) = 0;
    public:
      virtual void issue_frame(FrameOp *frame, ApEvent frame_termination) = 0;
      virtual void perform_frame_issue(FrameOp *frame, 
                                       ApEvent frame_termination) = 0;
      virtual void finish_frame(ApEvent frame_termination) = 0;
    public:
      virtual void increment_outstanding(void) = 0;
      virtual void decrement_outstanding(void) = 0;
      virtual void increment_pending(void) = 0;
      virtual void decrement_pending(TaskOp *child) = 0;
      virtual void decrement_pending(bool need_deferral) = 0;
      virtual void increment_frame(void) = 0;
      virtual void decrement_frame(void) = 0;
    public:
      // Override by RemoteTask and TopLevelTask
      virtual InnerContext* find_top_context(InnerContext *previous = NULL) = 0;
    public:
      virtual void initialize_region_tree_contexts(
          const std::vector<RegionRequirement> &clone_requirements,
          const LegionVector<VersionInfo> &version_infos,
          const std::vector<ApUserEvent> &unmap_events) = 0; 
      virtual void invalidate_region_tree_contexts(const bool is_top_level_task,
                                      std::set<RtEvent> &applied,
                                      const ShardMapping *mapping = NULL,
                                      ShardID source_shard = 0) = 0;
    public:
      virtual const std::vector<PhysicalRegion>& begin_task(Processor proc);
      virtual PhysicalInstance create_task_local_instance(Memory memory,
                                        Realm::InstanceLayoutGeneric *layout);
      virtual void destroy_task_local_instance(PhysicalInstance instance);
      virtual void end_task(const void *res, size_t res_size, bool owned,
                      PhysicalInstance inst, FutureFunctor *callback_functor,
                      const Realm::ExternalInstanceResource *resource,
                      void (*freefunc)(const Realm::ExternalInstanceResource&),
                      const void *metadataptr, size_t metadatasize, 
                      ApEvent effects);
      virtual void post_end_task(FutureInstance *instance,
                                 void *metadata, size_t metasize,
                                 FutureFunctor *callback_functor,
                                 bool own_callback_functor) = 0;
      bool is_task_local_instance(PhysicalInstance instance);
      LgEvent escape_task_local_instance(PhysicalInstance instance);
      FutureInstance* copy_to_future_inst(const void *value, size_t size);
      virtual void handle_mispredication(void);
    public:
      virtual Lock create_lock(void);
      virtual void destroy_lock(Lock l) = 0;
      virtual Grant acquire_grant(const std::vector<LockRequest> &requests) = 0;
      virtual void release_grant(Grant grant) = 0;
    public:
      virtual PhaseBarrier create_phase_barrier(unsigned arrivals);
      virtual void destroy_phase_barrier(PhaseBarrier pb) = 0;
      virtual PhaseBarrier advance_phase_barrier(PhaseBarrier pb);
    public:
      virtual DynamicCollective create_dynamic_collective(
                                                  unsigned arrivals,
                                                  ReductionOpID redop,
                                                  const void *init_value,
                                                  size_t init_size) = 0;
      virtual void destroy_dynamic_collective(DynamicCollective dc) = 0;
      virtual void arrive_dynamic_collective(DynamicCollective dc,
                        const void *buffer, size_t size, unsigned count) = 0;
      virtual void defer_dynamic_collective_arrival(DynamicCollective dc,
                                                    const Future &future,
                                                    unsigned count) = 0;
      virtual Future get_dynamic_collective_result(DynamicCollective dc,
                                                   Provenance *provenance) = 0;
      virtual DynamicCollective advance_dynamic_collective(
                                                   DynamicCollective dc) = 0;
    public:
      virtual TaskPriority get_current_priority(void) const = 0;
      virtual void set_current_priority(TaskPriority priority) = 0;
    public:
      PhysicalRegion get_physical_region(unsigned idx);
      void get_physical_references(unsigned idx, InstanceSet &refs); 
    public:
      OutputRegion get_output_region(unsigned idx) const;
      const std::vector<OutputRegion> get_output_regions(void) const
        { return output_regions; }
    public:
      virtual void raise_poison_exception(void);
      virtual void raise_region_exception(PhysicalRegion region, bool nuclear);
    public:
      bool safe_cast(RegionTreeForest *forest, IndexSpace handle, 
                     const void *realm_point, TypeTag type_tag);
      bool is_region_mapped(unsigned idx);
      void record_padded_fields(VariantImpl *variant);
    protected:
      LegionErrorType check_privilege_internal(const RegionRequirement &req,
                                      const RegionRequirement &parent_req,
                                      std::set<FieldID>& privilege_fields,
                                      FieldID &bad_field, int local, int &bad,
                                      bool skip_privileges) const;
      bool check_region_dependence(RegionTreeID tid, IndexSpace space,
                                  const RegionRequirement &our_req,
                                  const RegionUsage &our_usage,
                                  const RegionRequirement &req,
                                  bool check_privileges = true) const;
    public:
      void add_output_region(const OutputRequirement &req,
                             const InstanceSet &instances,
                             bool global_indexing, bool valid);
      void finalize_output_regions(void);
      void initialize_overhead_profiler(void);
      inline void begin_runtime_call(void);
      inline void end_runtime_call(void);
      inline void begin_wait(bool from_application);
      inline void end_wait(bool from_application);
      void remap_unmapped_regions(LogicalTrace *current_trace,
                           const std::vector<PhysicalRegion> &unmapped_regions,
                           Provenance *provenance);
    public:
      void* get_local_task_variable(LocalVariableID id);
      void set_local_task_variable(LocalVariableID id, const void *value,
                                   void (*destructor)(void*));
    public:
      void yield(void);
      size_t query_available_memory(Memory target);
    public:
      void increment_inlined(void);
      void decrement_inlined(void);
      void wait_for_inlined(void);
    protected:
      Future predicate_task_false(const TaskLauncher &launcher,
                                  Provenance *provenance);
      FutureMap predicate_index_task_false(size_t context_index,
                                           IndexSpace launch_space,
                                           const IndexTaskLauncher &launcher,
                                           Provenance *provenance);
      Future predicate_index_task_reduce_false(const IndexTaskLauncher &launch,
                                               IndexSpace launch_space,
                                               ReductionOpID redop,
                                               Provenance *provenance);
    public:
      // Find an index space name for a concrete launch domain
      IndexSpace find_index_launch_space(const Domain &domain,
                                         Provenance *provenance);
    public:
      // A little help for ConsensusMatchExchange since it is templated
      static void help_complete_future(Future &f, const void *ptr,
                                       size_t size, bool own);
    public:
      SingleTask *const owner_task;
      const std::vector<RegionRequirement> &regions;
      const std::vector<OutputRequirement> &output_reqs;
    protected:
      // For profiling information
      friend class SingleTask;
    protected:
      int                                       depth;
      // This data structure doesn't need a lock becaue
      // it is only mutated by the application task 
      std::vector<PhysicalRegion>               physical_regions;
    protected:
      std::vector<OutputRegion>                 output_regions;
    protected:
      Processor                             executing_processor;
    public:
      // Support for inlining
      mutable LocalLock                           inline_lock;
      unsigned                                    inlined_tasks;
      RtUserEvent                                 inlining_done;
    protected:
      class OverheadProfiler : 
        public Mapping::ProfilingMeasurements::RuntimeOverhead {
      public:
        OverheadProfiler(void) : inside_runtime_call(false) { }
      public:
        long long previous_profiling_time;
        bool inside_runtime_call;
      };
      OverheadProfiler *overhead_profiler; 
    protected:
      class ImplicitProfiler {
      public:
        std::vector<std::pair<long long,long long> > waits; 
        long long start_time;
      };
      ImplicitProfiler *implicit_profiler;
    protected:
      std::map<LocalVariableID,
               std::pair<void*,void (*)(void*)> > task_local_variables;
    protected:
      // Cache for accelerating safe casts
      std::map<IndexSpace,IndexSpaceNode*> safe_cast_spaces; 
    protected:
      // Map of task local instances including their unique events
      // from the profilters perspective
      std::map<PhysicalInstance,LgEvent> task_local_instances;
    protected:
      bool task_executed;
      bool mutable_priority;
    protected: 
      bool children_complete_invoked;
      bool children_commit_invoked;
    public:
      const bool inline_task;
      const bool implicit_task; 
    };

    class InnerContext : public TaskContext,
                         public ResourceTracker, 
                         public InstanceDeletionSubscriber,
                         public Murmur3Hasher::HashVerifier,
                         public LegionHeapify<InnerContext> {
    public:
      enum PipelineStage {
        EXECUTING_STAGE,
        EXECUTED_STAGE,
        COMPLETED_STAGE,
        COMMITTED_STAGE,
      };
      struct ReorderBufferEntry {
      public:
        inline ReorderBufferEntry(size_t index)
          : operation(NULL), operation_index(index), stage(COMMITTED_STAGE) { }
        inline ReorderBufferEntry(Operation *op)
          : operation(op), operation_index(op->get_ctx_index()),
            stage(EXECUTING_STAGE) { }
      public:
        Operation *operation;
        size_t operation_index;
        PipelineStage stage;
      };
    public:
      // Prepipeline stages need to hold a reference since the
      // logical analysis could clean the context up before it runs
      struct PrepipelineArgs : public LgTaskArgs<PrepipelineArgs> {
      public:
        static const LgTaskID TASK_ID = LG_PRE_PIPELINE_ID;
      public:
        PrepipelineArgs(Operation *op, InnerContext *ctx)
          : LgTaskArgs<PrepipelineArgs>(op->get_unique_op_id()),
            context(ctx) { }
      public:
        InnerContext *const context;
      };
      struct DependenceArgs : public LgTaskArgs<DependenceArgs> {
      public:
        static const LgTaskID TASK_ID = LG_TRIGGER_DEPENDENCE_ID;
      public:
        DependenceArgs(Operation *op, InnerContext *ctx)
          : LgTaskArgs<DependenceArgs>(op->get_unique_op_id()), 
            context(ctx) { }
      public:
        InnerContext *const context;
      }; 
      struct TriggerReadyArgs : public LgTaskArgs<TriggerReadyArgs> {
      public:
        static const LgTaskID TASK_ID = LG_TRIGGER_READY_ID;
      public:
        TriggerReadyArgs(Operation *op, InnerContext *ctx)
          : LgTaskArgs<TriggerReadyArgs>(op->get_unique_op_id()),
            context(ctx) { }
      public:
        InnerContext *const context;
      };
      struct DeferredEnqueueTaskArgs : 
        public LgTaskArgs<DeferredEnqueueTaskArgs> {
      public:
        static const LgTaskID TASK_ID = LG_DEFERRED_ENQUEUE_TASK_ID;
      public:
        DeferredEnqueueTaskArgs(TaskOp *t, InnerContext *ctx)
          : LgTaskArgs<DeferredEnqueueTaskArgs>(t->get_unique_op_id()),
            context(ctx) { }
      public:
        InnerContext *const context;
      };
      struct DeferredDistributeTaskArgs : 
        public LgTaskArgs<DeferredDistributeTaskArgs> {
      public:
        static const LgTaskID TASK_ID = LG_DEFERRED_DISTRIBUTE_TASK_ID;
      public:
        DeferredDistributeTaskArgs(TaskOp *op, InnerContext *ctx)
          : LgTaskArgs<DeferredDistributeTaskArgs>(op->get_unique_op_id()),
            context(ctx) { }
      public:
        InnerContext *const context;
      };
      struct DeferredLaunchTaskArgs :
        public LgTaskArgs<DeferredLaunchTaskArgs> {
      public:
        static const LgTaskID TASK_ID = LG_DEFERRED_LAUNCH_TASK_ID;
      public:
        DeferredLaunchTaskArgs(TaskOp *op, InnerContext *ctx)
          : LgTaskArgs<DeferredLaunchTaskArgs>(op->get_unique_op_id()),
            context(ctx) { }
      public:
        InnerContext *const context;
      };
      struct TriggerResolutionArgs : public LgTaskArgs<TriggerResolutionArgs> {
      public:
        static const LgTaskID TASK_ID = LG_TRIGGER_RESOLUTION_ID;
      public:
        TriggerResolutionArgs(Operation *op, InnerContext *ctx)
          : LgTaskArgs<TriggerResolutionArgs>(op->get_unique_op_id()),
            context(ctx) { }
      public:
        InnerContext *const context;
      };
      struct TriggerExecutionArgs : public LgTaskArgs<TriggerExecutionArgs> {
      public:
        static const LgTaskID TASK_ID = LG_TRIGGER_EXECUTION_ID;
      public:
        TriggerExecutionArgs(Operation *op, InnerContext *ctx)
          : LgTaskArgs<TriggerExecutionArgs>(op->get_unique_op_id()),
            context(ctx) { }
      public:
        InnerContext *const context;
      };
      struct DeferredExecutionArgs : public LgTaskArgs<DeferredExecutionArgs> {
      public:
        static const LgTaskID TASK_ID = LG_DEFERRED_EXECUTION_ID;
      public:
        DeferredExecutionArgs(Operation *op, InnerContext *ctx)
          : LgTaskArgs<DeferredExecutionArgs>(op->get_unique_op_id()),
            context(ctx) { }
      public:
        InnerContext *const context;
      };
      struct TriggerCompletionArgs : public LgTaskArgs<TriggerCompletionArgs> {
      public:
        static const LgTaskID TASK_ID = LG_TRIGGER_COMPLETION_ID;
      public:
        TriggerCompletionArgs(Operation *op, InnerContext *ctx)
          : LgTaskArgs<TriggerCompletionArgs>(op->get_unique_op_id()),
            context(ctx) { }
      public:
        InnerContext *const context;
      };
      struct DeferredCompletionArgs : 
        public LgTaskArgs<DeferredCompletionArgs> {
      public:
        static const LgTaskID TASK_ID = LG_DEFERRED_COMPLETION_ID;
      public:
        DeferredCompletionArgs(Operation *op, InnerContext *ctx)
          : LgTaskArgs<DeferredCompletionArgs>(op->get_unique_op_id()),
            context(ctx) { }
      public:
        InnerContext *const context;
      };
      struct TriggerCommitArgs : public LgTaskArgs<TriggerCommitArgs> {
      public:
        static const LgTaskID TASK_ID = LG_TRIGGER_COMMIT_ID; 
      public:
        TriggerCommitArgs(Operation *op, InnerContext *ctx)
          : LgTaskArgs<TriggerCommitArgs>(op->get_unique_op_id()),
            context(ctx) { }
      public:
        InnerContext *const context;
      };
      struct DeferredCommitArgs : public LgTaskArgs<DeferredCommitArgs> {
      public:
        static const LgTaskID TASK_ID = LG_DEFERRED_COMMIT_ID;
      public:
        DeferredCommitArgs(const std::pair<Operation*,bool> &op,
                           InnerContext *ctx)
          : LgTaskArgs<DeferredCommitArgs>(op.first->get_unique_op_id()),
            context(ctx) { }
      public:
        InnerContext *const context;
      };
      struct PostEndArgs : public LgTaskArgs<PostEndArgs> {
      public:
        static const LgTaskID TASK_ID = LG_POST_END_ID;
      public:
        PostEndArgs(TaskOp *owner, InnerContext *ctx)
          : LgTaskArgs<PostEndArgs>(owner->get_unique_op_id()),
            proxy_this(ctx) { }
      public:
        InnerContext *const proxy_this;
      };
      struct PostTaskArgs {
      public:
        PostTaskArgs(TaskContext *ctx, size_t x, RtEvent w,
            FutureInstance *i, void *m, size_t s, FutureFunctor *f, bool o)
          : context(ctx), index(x), wait_on(w), instance(i), 
            metadata(m), metasize(s), functor(f), own_functor(o) { }
      public:
        inline bool operator<(const PostTaskArgs &rhs) const
          { return index < rhs.index; }
      public:
        TaskContext *context;
        size_t index;
        RtEvent wait_on;
        FutureInstance *instance;
        void *metadata;
        size_t metasize;
        FutureFunctor *functor;
        bool own_functor;
      };
      struct IssueFrameArgs : public LgTaskArgs<IssueFrameArgs> {
      public:
        static const LgTaskID TASK_ID = LG_ISSUE_FRAME_TASK_ID;
      public:
        IssueFrameArgs(TaskOp *owner, InnerContext *ctx,
                       FrameOp *f, ApEvent term)
          : LgTaskArgs<IssueFrameArgs>(owner->get_unique_op_id()),
            parent_ctx(ctx), frame(f), frame_termination(term) { }
      public:
        InnerContext *const parent_ctx;
        FrameOp *const frame;
        const ApEvent frame_termination;
      };
      struct VerifyPartitionArgs : public LgTaskArgs<VerifyPartitionArgs> {
      public:
        static const LgTaskID TASK_ID = LG_DEFER_VERIFY_PARTITION_TASK_ID;
      public:
        VerifyPartitionArgs(InnerContext *proxy, IndexPartition p, 
                            PartitionKind k, const char *f)
          : LgTaskArgs<VerifyPartitionArgs>(proxy->get_unique_id()), 
            proxy_this(proxy), pid(p), kind(k), func(f) { }
      public:
        InnerContext *const proxy_this;
        const IndexPartition pid;
        const PartitionKind kind;
        const char *const func;
      };
      template<typename T>
      struct QueueEntry {
      public:
        QueueEntry(void) { op = {}; }
        QueueEntry(T o, RtEvent r) : op(o), ready(r) { }
      public:
        T op;
        RtEvent ready;
      };
      struct LocalFieldInfo {
      public:
        LocalFieldInfo(void)
          : fid(0), size(0), serdez(0), index(0), ancestor(false) { }
        LocalFieldInfo(FieldID f, size_t s, CustomSerdezID z, 
                       unsigned idx, bool a)
          : fid(f), size(s), serdez(z), index(idx), ancestor(a) { }
      public:
        FieldID fid;
        size_t size;
        CustomSerdezID serdez;
        unsigned index;
        bool ancestor;
      };
      class AttachProjectionFunctor : public ProjectionFunctor {
      public:
        AttachProjectionFunctor(Runtime *rt, ProjectionID pid,
                                std::vector<IndexSpace> &&spaces);
        virtual ~AttachProjectionFunctor(void) { }
      public:
        using ProjectionFunctor::project;
        virtual LogicalRegion project(LogicalRegion upper_bound,
                                      const DomainPoint &point,
                                      const Domain &launch_domain);
        virtual LogicalRegion project(LogicalPartition upper_bound,
                                      const DomainPoint &point,
                                      const Domain &launch_domain);
      public:
        virtual bool is_functional(void) const { return true; }
        // Some depth >0 means the runtime can't analyze it
        virtual unsigned get_depth(void) const { return UINT_MAX; }
      public:
        static unsigned compute_offset(const DomainPoint &point,
                                       const Domain &launch);
      public:
        const std::vector<IndexSpace> handles;
        const ProjectionID pid;
      };
      typedef CollectiveViewCreatorBase::CollectiveResult CollectiveResult;
    public:
      InnerContext(Runtime *runtime, SingleTask *owner, int depth, 
                   bool full_inner, const std::vector<RegionRequirement> &reqs,
                   const std::vector<OutputRequirement> &output_reqs,
                   const std::vector<unsigned> &parent_indexes,
                   const std::vector<bool> &virt_mapped,
                   ApEvent execution_fence, DistributedID did = 0,
                   bool inline_task = false, bool implicit_task = false,
                   bool concurrent_task = false,
                   CollectiveMapping *mapping = NULL);
      InnerContext(const InnerContext &rhs) = delete;
      virtual ~InnerContext(void);
    public:
      InnerContext& operator=(const InnerContext &rhs) = delete;
    public:
      inline size_t get_tunable_index(void)
        { return total_tunable_count++; }
      inline unsigned get_max_trace_templates(void) const
        { return context_configuration.max_templates_per_trace; }
      void record_physical_trace_replay(RtEvent ready, bool replay);
      bool is_replaying_physical_trace(void); 
      inline bool is_concurrent_context(void) const
        { return concurrent_context; }
    public: // Garbage collection methods
      virtual void notify_local(void);
    public: // Privilege tracker methods
      virtual void receive_resources(size_t return_index,
              std::map<LogicalRegion,unsigned> &created_regions,
              std::vector<DeletedRegion> &deleted_regions,
              std::set<std::pair<FieldSpace,FieldID> > &created_fields,
              std::vector<DeletedField> &deleted_fields,
              std::map<FieldSpace,unsigned> &created_field_spaces,
              std::map<FieldSpace,std::set<LogicalRegion> > &latent_spaces,
              std::vector<DeletedFieldSpace> &deleted_field_spaces,
              std::map<IndexSpace,unsigned> &created_index_spaces,
              std::vector<DeletedIndexSpace> &deleted_index_spaces,
              std::map<IndexPartition,unsigned> &created_partitions,
              std::vector<DeletedPartition> &deleted_partitions,
              std::set<RtEvent> &preconditions);
    public: // Murmur3Hasher::HashVerifier method
      virtual bool verify_hash(const uint64_t hash[2],
          const char *description, Provenance *provenance, bool every);
    public:
      LogicalRegion find_logical_region(unsigned index);
      int find_parent_region_req(const RegionRequirement &req, 
                                 bool check_privilege = true);
      LegionErrorType check_privilege(const IndexSpaceRequirement &req) const;
      LegionErrorType check_privilege(const RegionRequirement &req, 
                                      FieldID &bad_field, int &bad_index, 
                                      bool skip_privileges = false) const; 
    public:
      unsigned add_created_region(LogicalRegion handle, bool task_local,
                                  bool output_region = false);
      // for logging created region requirements
      virtual void log_created_requirements(void); 
      virtual void report_leaks_and_duplicates(
          std::set<RtEvent> &preconditions);
    public:
      unsigned register_region_creation(LogicalRegion handle, bool task_local,
                                        bool output_region);
    public:
      void register_field_creation(FieldSpace space, FieldID fid, bool local);
      void register_all_field_creations(FieldSpace space, bool local,
                                        const std::vector<FieldID> &fields);
    public:
      void register_field_space_creation(FieldSpace space);
    public:
      bool has_created_index_space(IndexSpace space) const;
      void register_index_space_creation(IndexSpace space);
    public:
      void register_index_partition_creation(IndexPartition handle);
    public:
      void analyze_destroy_fields(FieldSpace handle,
                                  const std::set<FieldID> &to_delete,
                                  std::vector<RegionRequirement> &delete_reqs,
                                  std::vector<unsigned> &parent_req_indexes,
                                  std::vector<FieldID> &global_to_free,
                                  std::vector<FieldID> &local_to_free,
                                  std::vector<FieldID> &local_field_indexes,
                                  std::vector<unsigned> &deletion_req_indexes);
      void analyze_destroy_logical_region(LogicalRegion handle,
                                  std::vector<RegionRequirement> &delete_reqs,
                                  std::vector<unsigned> &parent_req_indexes,
                                  std::vector<bool> &returnable_privileges);
      void analyze_free_local_fields(FieldSpace handle,
                                  const std::vector<FieldID> &local_to_free,
                                  std::vector<unsigned> &local_field_indexes);
      void remove_deleted_local_fields(FieldSpace space,
                                 const std::vector<FieldID> &to_remove);
    protected:
      void register_region_creations(
                     std::map<LogicalRegion,unsigned> &regions);
      void register_region_deletions(ApEvent precondition,
                     const std::map<Operation*,GenerationID> &dependences,
                     std::vector<DeletedRegion> &regions,
                     std::set<RtEvent> &preconditions);
      void register_field_creations(
            std::set<std::pair<FieldSpace,FieldID> > &fields);
      void register_field_deletions(ApEvent precondition,
            const std::map<Operation*,GenerationID> &dependences,
            std::vector<DeletedField> &fields,
            std::set<RtEvent> &preconditions);
      void register_field_space_creations(
                          std::map<FieldSpace,unsigned> &spaces);
      void register_latent_field_spaces(
            std::map<FieldSpace,std::set<LogicalRegion> > &spaces);
      void register_field_space_deletions(ApEvent precondition,
                          const std::map<Operation*,GenerationID> &dependences,
                          std::vector<DeletedFieldSpace> &spaces,
                          std::set<RtEvent> &preconditions);
      void register_index_space_creations(
                          std::map<IndexSpace,unsigned> &spaces);
      void register_index_space_deletions(ApEvent precondition,
                          const std::map<Operation*,GenerationID> &dependences,
                          std::vector<DeletedIndexSpace> &spaces,
                          std::set<RtEvent> &preconditions);
      void register_index_partition_creations(
                          std::map<IndexPartition,unsigned> &parts);
      void register_index_partition_deletions(ApEvent precondition,
                          const std::map<Operation*,GenerationID> &dependences,
                          std::vector<DeletedPartition> &parts,
                          std::set<RtEvent> &preconditions);
      ApEvent compute_return_deletion_dependences(size_t return_index,
                          std::map<Operation*,GenerationID> &dependences);
    public:
      int has_conflicting_regions(MapOp *map, bool &parent_conflict,
                                  bool &inline_conflict);
      int has_conflicting_regions(AttachOp *attach, bool &parent_conflict,
                                  bool &inline_conflict);
      int has_conflicting_internal(const RegionRequirement &req, 
                                   bool &parent_conflict,
                                   bool &inline_conflict);
      void find_conflicting_regions(TaskOp *task,
                                    std::vector<PhysicalRegion> &conflicting);
      void find_conflicting_regions(CopyOp *copy,
                                    std::vector<PhysicalRegion> &conflicting);
      void find_conflicting_regions(AcquireOp *acquire,
                                    std::vector<PhysicalRegion> &conflicting);
      void find_conflicting_regions(ReleaseOp *release,
                                    std::vector<PhysicalRegion> &conflicting);
      void find_conflicting_regions(DependentPartitionOp *partition,
                                    std::vector<PhysicalRegion> &conflicting);
      void find_conflicting_internal(const RegionRequirement &req,
                                    std::vector<PhysicalRegion> &conflicting);
      void find_conflicting_regions(FillOp *fill,
                                    std::vector<PhysicalRegion> &conflicting); 
      void find_conflicting_regions(DiscardOp *fill,
                                    std::vector<PhysicalRegion> &conflicting);
      void register_inline_mapped_region(const PhysicalRegion &region);
      void unregister_inline_mapped_region(const PhysicalRegion &region);
    public:
      void print_children(void);
      void perform_window_wait(void);
    public:
      // Interface for task contexts
      virtual ContextID get_logical_tree_context(void) const;
      virtual ContextID get_physical_tree_context(void) const;
      virtual bool is_inner_context(void) const;
      virtual void pack_remote_context(Serializer &rez, 
          AddressSpaceID target, bool replicate = false);
      virtual void compute_task_tree_coordinates(
                            TaskTreeCoordinates &coordinates) const;
      virtual RtEvent compute_equivalence_sets(unsigned req_index,
                      const std::vector<EqSetTracker*> &targets,
                      const std::vector<AddressSpaceID> &target_spaces,
                      AddressSpaceID creation_target_space,
                      IndexSpaceExpression *expr, const FieldMask &mask);
      virtual RtEvent record_output_equivalence_set(EqSetTracker *source,
                      AddressSpaceID source_space, unsigned req_index,
                      EquivalenceSet *set, const FieldMask &mask);
      EqKDTree* find_equivalence_set_kd_tree(unsigned req_index,
          LocalLock *&tree_lock, bool return_null_if_doesnt_exist = false);
      EqKDTree* find_or_create_output_set_kd_tree(unsigned req_index,
                                                  LocalLock *&tree_lock);
      void finalize_output_eqkd_tree(unsigned req_index);
      // This method must be called while holding the privilege lock
      IndexSpace find_root_index_space(unsigned req_index);
      RtEvent report_equivalence_sets(const CollectiveMapping &target_mapping,
          const std::vector<EqSetTracker*> &targets,
          const AddressSpaceID creation_target_space, const FieldMask &mask,
          std::vector<unsigned> &new_target_references,
          FieldMaskSet<EquivalenceSet> &eq_sets,
          FieldMaskSet<EqKDTree> &new_subscriptions,
          FieldMaskSet<EqKDTree> &to_create,
          std::map<EqKDTree*,Domain> &creation_rects,
          std::map<EquivalenceSet*,LegionMap<Domain,FieldMask> > &creation_srcs,
          size_t expected_responses, std::vector<RtEvent> &ready_events);
      RtEvent report_output_registrations(EqSetTracker *target,
          AddressSpaceID target_space, unsigned references,
          FieldMaskSet<EqKDTree> &new_subscriptions);
      virtual EqKDTree* create_equivalence_set_kd_tree(IndexSpaceNode *node);
    public:
      virtual bool attempt_children_complete(void);
      virtual bool attempt_children_commit(void);
      bool inline_child_task(TaskOp *child);
      virtual void return_resources(ResourceTracker *target, 
                                    size_t return_index,
                                    std::set<RtEvent> &preconditions);
      virtual void pack_return_resources(Serializer &rez, size_t return_index);
    protected:
      IndexSpace create_index_space_internal(const Domain *bounds,
                                             TypeTag type_tag,
                                             Provenance *provenance);
    public:
      // Find an index space name for a concrete launch domain
      IndexSpace find_index_launch_space(const Domain &domain,
                                         Provenance *provenance);
    public:
      // Interface to operations performed by a context
      virtual IndexSpace create_index_space(const Domain &bounds,
                                            TypeTag type_tag,
                                            Provenance *provenance);
      virtual IndexSpace create_index_space(const Future &future,
                                            TypeTag type_tag,
                                            Provenance *provenance);
      virtual IndexSpace create_index_space(
                           const std::vector<DomainPoint> &points,
                           Provenance *provenance);
      virtual IndexSpace create_index_space(
                           const std::vector<Domain> &rects,
                           Provenance *provenance);
      virtual IndexSpace create_unbound_index_space(TypeTag type_tag,
                                                    Provenance *provenance);
      virtual IndexSpace union_index_spaces(
                           const std::vector<IndexSpace> &spaces,
                           Provenance *provenance);
      virtual IndexSpace intersect_index_spaces(
                           const std::vector<IndexSpace> &spaces,
                           Provenance *provenance);
      virtual IndexSpace subtract_index_spaces(
                           IndexSpace left, IndexSpace right,
                           Provenance *provenance);
      virtual void create_shared_ownership(IndexSpace handle);
      virtual void destroy_index_space(IndexSpace handle, const bool unordered,
                                       const bool recurse,
                                       Provenance *provenance);
      virtual void create_shared_ownership(IndexPartition handle);
      virtual void destroy_index_partition(IndexPartition handle,
                                           const bool unordered,
                                           const bool recurse,
                                           Provenance *provenance);
      virtual IndexPartition create_equal_partition(
                                            IndexSpace parent,
                                            IndexSpace color_space,
                                            size_t granularity,
                                            Color color,
                                            Provenance *provenance);
      virtual IndexPartition create_partition_by_weights(IndexSpace parent,
                                            const FutureMap &weights,
                                            IndexSpace color_space,
                                            size_t granularity, 
                                            Color color,
                                            Provenance *provenance);
      virtual IndexPartition create_partition_by_union(
                                            IndexSpace parent,
                                            IndexPartition handle1,
                                            IndexPartition handle2,
                                            IndexSpace color_space,
                                            PartitionKind kind,
                                            Color color,
                                            Provenance *provenance);
      virtual IndexPartition create_partition_by_intersection(
                                            IndexSpace parent,
                                            IndexPartition handle1,
                                            IndexPartition handle2,
                                            IndexSpace color_space,
                                            PartitionKind kind,
                                            Color color,
                                            Provenance *provenance);
      virtual IndexPartition create_partition_by_intersection(
                                            IndexSpace parent,
                                            IndexPartition partition,
                                            PartitionKind kind,
                                            Color color,
                                            bool dominates,
                                            Provenance *provenance);
      virtual IndexPartition create_partition_by_difference(
                                            IndexSpace parent,
                                            IndexPartition handle1,
                                            IndexPartition handle2,
                                            IndexSpace color_space,
                                            PartitionKind kind,
                                            Color color,
                                            Provenance *provenance);
      virtual Color create_cross_product_partitions(
                                            IndexPartition handle1,
                                            IndexPartition handle2,
                              std::map<IndexSpace,IndexPartition> &handles,
                                            PartitionKind kind,
                                            Color color,
                                            Provenance *provenance);
      virtual void create_association(      LogicalRegion domain,
                                            LogicalRegion domain_parent,
                                            FieldID domain_fid,
                                            IndexSpace range,
                                            MapperID id, MappingTagID tag,
                                            const UntypedBuffer &marg,
                                            Provenance *prov);
      virtual IndexPartition create_restricted_partition(
                                            IndexSpace parent,
                                            IndexSpace color_space,
                                            const void *transform,
                                            size_t transform_size,
                                            const void *extent,
                                            size_t extent_size,
                                            PartitionKind part_kind,
                                            Color color,
                                            Provenance *provenance);
      virtual IndexPartition create_partition_by_domain(
                                            IndexSpace parent,
                                  const std::map<DomainPoint,Domain> &domains,
                                            IndexSpace color_space,
                                            bool perform_intersections,
                                            PartitionKind part_kind,
                                            Color color,
                                            Provenance *provenance);
      virtual IndexPartition create_partition_by_domain(
                                            IndexSpace parent,
                                            const FutureMap &domains,
                                            IndexSpace color_space,
                                            bool perform_intersections,
                                            PartitionKind part_kind,
                                            Color color,
                                            Provenance *provenance,
                                            bool skip_check = false);
      virtual IndexPartition create_partition_by_field(
                                            LogicalRegion handle,
                                            LogicalRegion parent_priv,
                                            FieldID fid,
                                            IndexSpace color_space,
                                            Color color,
                                            MapperID id, MappingTagID tag,
                                            PartitionKind part_kind,
                                            const UntypedBuffer &marg,
                                            Provenance *prov);
      virtual IndexPartition create_partition_by_image(
                                            IndexSpace handle,
                                            LogicalPartition projection,
                                            LogicalRegion parent,
                                            FieldID fid,
                                            IndexSpace color_space,
                                            PartitionKind part_kind,
                                            Color color,
                                            MapperID id, MappingTagID tag,
                                            const UntypedBuffer &marg,
                                            Provenance *prov);
      virtual IndexPartition create_partition_by_image_range(
                                            IndexSpace handle,
                                            LogicalPartition projection,
                                            LogicalRegion parent,
                                            FieldID fid,
                                            IndexSpace color_space,
                                            PartitionKind part_kind,
                                            Color color,
                                            MapperID id, MappingTagID tag,
                                            const UntypedBuffer &marg,
                                            Provenance *prov);
      virtual IndexPartition create_partition_by_preimage(
                                            IndexPartition projection,
                                            LogicalRegion handle,
                                            LogicalRegion parent,
                                            FieldID fid,
                                            IndexSpace color_space,
                                            PartitionKind part_kind,
                                            Color color,
                                            MapperID id, MappingTagID tag,
                                            const UntypedBuffer &marg,
                                            Provenance *prov);
      virtual IndexPartition create_partition_by_preimage_range(
                                            IndexPartition projection,
                                            LogicalRegion handle,
                                            LogicalRegion parent,
                                            FieldID fid,
                                            IndexSpace color_space,
                                            PartitionKind part_kind,
                                            Color color,
                                            MapperID id, MappingTagID tag,
                                            const UntypedBuffer &marg,
                                            Provenance *prov);
      virtual IndexPartition create_pending_partition(
                                            IndexSpace parent,
                                            IndexSpace color_space,
                                            PartitionKind part_kind,
                                            Color color,
                                            Provenance *provenance,
                                            bool trust = false);
      virtual IndexSpace create_index_space_union(
                                            IndexPartition parent,
                                            const void *realm_color,
                                            size_t color_size,
                                            TypeTag type_tag,
                                const std::vector<IndexSpace> &handles,
                                            Provenance *provenance);
      virtual IndexSpace create_index_space_union(
                                            IndexPartition parent,
                                            const void *realm_color,
                                            size_t color_size,
                                            TypeTag type_tag,
                                            IndexPartition handle,
                                            Provenance *provenance);
      virtual IndexSpace create_index_space_intersection(
                                            IndexPartition parent,
                                            const void *realm_color,
                                            size_t color_size,
                                            TypeTag type_tag,
                                const std::vector<IndexSpace> &handles,
                                            Provenance *provenance);
      virtual IndexSpace create_index_space_intersection(
                                            IndexPartition parent,
                                            const void *realm_color,
                                            size_t color_size,
                                            TypeTag type_tag,
                                            IndexPartition handle,
                                            Provenance *provenance);
      virtual IndexSpace create_index_space_difference(
                                            IndexPartition parent,
                                            const void *realm_color,
                                            size_t color_size,
                                            TypeTag type_tag,
                                            IndexSpace initial,
                                const std::vector<IndexSpace> &handles,
                                            Provenance *provenance);
      virtual void verify_partition(IndexPartition pid, PartitionKind kind,
                                    const char *function_name);
      static void handle_partition_verification(const void *args);
      virtual FieldSpace create_field_space(Provenance *provenance);
      virtual FieldSpace create_field_space(const std::vector<size_t> &sizes,
                                        std::vector<FieldID> &resulting_fields,
                                        CustomSerdezID serdez_id,
                                        Provenance *provenance);
      virtual FieldSpace create_field_space(const std::vector<Future> &sizes,
                                        std::vector<FieldID> &resulting_fields,
                                        CustomSerdezID serdez_id,
                                        Provenance *provenance);
      virtual void create_shared_ownership(FieldSpace handle);
      virtual void destroy_field_space(FieldSpace handle,
                                       const bool unordered,
                                       Provenance *provenance);
      virtual FieldID allocate_field(FieldSpace space, size_t field_size,
                                     FieldID fid, bool local,
                                     CustomSerdezID serdez_id,
                                     Provenance *provenance);
      virtual FieldID allocate_field(FieldSpace space, const Future &field_size,
                                     FieldID fid, bool local,
                                     CustomSerdezID serdez_id,
                                     Provenance *provenance);
      virtual void allocate_local_field(
                                     FieldSpace space, size_t field_size,
                                     FieldID fid, CustomSerdezID serdez_id,
                                     std::set<RtEvent> &done_events,
                                     Provenance *provenance);
      virtual void free_field(FieldAllocatorImpl *allocator, FieldSpace space, 
                              FieldID fid, const bool unordered,
                              Provenance *provenance);
      virtual void allocate_fields(FieldSpace space,
                                   const std::vector<size_t> &sizes,
                                   std::vector<FieldID> &resuling_fields,
                                   bool local, CustomSerdezID serdez_id,
                                   Provenance *provenance);
      virtual void allocate_fields(FieldSpace space,
                                   const std::vector<Future> &sizes,
                                   std::vector<FieldID> &resuling_fields,
                                   bool local, CustomSerdezID serdez_id,
                                   Provenance *provenance);
      virtual void allocate_local_fields(FieldSpace space,
                                   const std::vector<size_t> &sizes,
                                   const std::vector<FieldID> &resuling_fields,
                                   CustomSerdezID serdez_id,
                                   std::set<RtEvent> &done_events,
                                   Provenance *provenance);
      virtual void free_fields(FieldAllocatorImpl *allocator, FieldSpace space, 
                               const std::set<FieldID> &to_free,
                               const bool unordered,
                               Provenance *provenance);
      virtual LogicalRegion create_logical_region(
                                            IndexSpace index_space,
                                            FieldSpace field_space,
                                            bool task_local,
                                            Provenance *provenance,
                                            const bool output_region = false);
      virtual void create_shared_ownership(LogicalRegion handle);
      virtual void destroy_logical_region(LogicalRegion handle,
                                          const bool unordered,
                                          Provenance *provenance);
      virtual void reset_equivalence_sets(LogicalRegion parent, 
                                          LogicalRegion region,
                                          const std::set<FieldID> &fields);
      virtual FieldAllocatorImpl* create_field_allocator(FieldSpace handle,
                                                         bool unordered);
      virtual void destroy_field_allocator(FieldSpaceNode *node,
                                           bool from_application = true);
      virtual void get_local_field_set(const FieldSpace handle,
                                       const std::set<unsigned> &indexes,
                                       std::set<FieldID> &to_set) const;
      virtual void get_local_field_set(const FieldSpace handle,
                                       const std::set<unsigned> &indexes,
                                       std::vector<FieldID> &to_set) const;
    public:
      virtual void add_physical_region(const RegionRequirement &req, 
          bool mapped, MapperID mid, MappingTagID tag, ApUserEvent &unmap_event,
          bool virtual_mapped, const InstanceSet &physical_instances);
      virtual Future execute_task(const TaskLauncher &launcher,
                                  std::vector<OutputRequirement> *outputs);
      virtual FutureMap execute_index_space(const IndexTaskLauncher &launcher,
                                       std::vector<OutputRequirement> *outputs);
      virtual Future execute_index_space(const IndexTaskLauncher &launcher,
                                       ReductionOpID redop, bool deterministic,
                                       std::vector<OutputRequirement> *outputs);
      virtual Future reduce_future_map(const FutureMap &future_map,
                                       ReductionOpID redop, bool deterministic,
                                       MapperID map_id, MappingTagID tag,
                                       Provenance *provenance,
                                       Future initial_value);
      virtual FutureMap construct_future_map(IndexSpace domain,
                               const std::map<DomainPoint,UntypedBuffer> &data,
                                             Provenance *provenance,
                                             bool collective = false,
                                             ShardingID sid = 0,
                                             bool implicit = false,
                                             bool internal = false,
                                             bool check_space = true);
      virtual FutureMap construct_future_map(const Domain &domain,
                                const std::map<DomainPoint,UntypedBuffer> &data,
                                             bool collective = false,
                                             ShardingID sid = 0,
                                             bool implicit = false);
      virtual FutureMap construct_future_map(IndexSpace domain,
                                   const std::map<DomainPoint,Future> &futures,
                                             Provenance *provenance,
                                             bool internal = false,
                                             bool collective = false,
                                             ShardingID sid = 0,
                                             bool implicit = false,
                                             bool check_space = true);
      virtual FutureMap construct_future_map(const Domain &domain,
                    const std::map<DomainPoint,Future> &futures,
                                             bool internal = false,
                                             bool collective = false,
                                             ShardingID sid = 0,
                                             bool implicit = false);
      virtual FutureMap transform_future_map(const FutureMap &fm,
                                             IndexSpace new_domain, 
                      TransformFutureMapImpl::PointTransformFnptr fnptr,
                                             Provenance *provenance);
      virtual FutureMap transform_future_map(const FutureMap &fm,
                                             IndexSpace new_domain,
                                             PointTransformFunctor *functor,
                                             bool own_functor,
                                             Provenance *provenance);
      virtual PhysicalRegion map_region(const InlineLauncher &launcher);
      virtual ApEvent remap_region(const PhysicalRegion &region,
                                   Provenance *provenance,
                                   bool internal = false);
      virtual void unmap_region(PhysicalRegion region);
      virtual void unmap_all_regions(bool external);
      virtual void fill_fields(const FillLauncher &launcher);
      virtual void fill_fields(const IndexFillLauncher &launcher);
      virtual void discard_fields(const DiscardLauncher &launcher);
      virtual void issue_copy(const CopyLauncher &launcher);
      virtual void issue_copy(const IndexCopyLauncher &launcher);
      virtual void issue_acquire(const AcquireLauncher &launcher);
      virtual void issue_release(const ReleaseLauncher &launcher);
      virtual PhysicalRegion attach_resource(const AttachLauncher &launcher);
      virtual ExternalResources attach_resources(
                                        const IndexAttachLauncher &launcher);
      virtual RegionTreeNode* compute_index_attach_upper_bound(
                                        const IndexAttachLauncher &launcher,
                                        const std::vector<unsigned> &indexes);
      ProjectionID compute_index_attach_projection(
                                        IndexTreeNode *node, IndexAttachOp *op,
                                        unsigned local_start, size_t local_size,
                                        std::vector<IndexSpace> &spaces,
                                        const bool can_use_identity = false);
      virtual Future detach_resource(PhysicalRegion region, const bool flush,
                                     const bool unordered,
                                     Provenance *provenance = NULL);
      virtual Future detach_resources(ExternalResources resources,
                                      const bool flush, const bool unordered,
                                      Provenance *provenance);
      virtual void progress_unordered_operations(bool end_task = false);
      virtual FutureMap execute_must_epoch(const MustEpochLauncher &launcher);
      virtual Future issue_timing_measurement(const TimingLauncher &launcher);
      virtual Future select_tunable_value(const TunableLauncher &launcher);
      virtual Future issue_mapping_fence(Provenance *provenance);
      virtual Future issue_execution_fence(Provenance *provenance);
      virtual void complete_frame(Provenance *provenance);
      virtual Predicate create_predicate(const Future &f,
                                         Provenance *provenance);
      virtual Predicate predicate_not(const Predicate &p,
                                      Provenance *provenance);
      virtual Predicate create_predicate(const PredicateLauncher &launcher);
      virtual Future get_predicate_future(const Predicate &p,
                                          Provenance *provenance);
      virtual PredicateImpl* create_predicate_impl(Operation *op);
    public:
      // The following set of operations correspond directly
      // to the complete_mapping, complete_operation, and
      // commit_operations performed by an operation.  Every
      // one of those calls invokes the corresponding one of
      // these calls to notify the parent context.
      size_t register_new_child_operation(Operation *op,
                                                  RtUserEvent &resolved,
                const std::vector<StaticDependence> *dependences);
      // Must be called while holding the dependence lock
      virtual void insert_unordered_ops(AutoLock &d_lock);
      void issue_unordered_operations(AutoLock &d_lock, 
                std::vector<Operation*> &ready_operations);
      size_t register_new_summary_operation(TraceSummaryOp *op);
    public:
      void add_to_prepipeline_queue(Operation *op);
      bool process_prepipeline_stage(void);
    public:
      virtual bool add_to_dependence_queue(Operation *op, 
                                           bool unordered = false,
                                           bool outermost = true);
      void process_dependence_stage(void);
      void add_to_post_task_queue(TaskContext *ctx, RtEvent wait_on,
                                  FutureInstance *instance,
                                  FutureFunctor *callback_functor,
                                  bool own_callback_functor,
                                  const void *metadataptr,
                                  size_t metadatasize);
    public:
      template<typename T, typename ARGS, bool HAS_BOUNDS>
      void add_to_queue(QueueEntry<T> entry, LocalLock &lock,
                        std::list<QueueEntry<T> > &queue,
                        CompletionQueue &comp_queue);
      template<typename T>
      T process_queue(LocalLock &lock, RtEvent &next_ready,
                      std::list<QueueEntry<T> > &queue,
                      CompletionQueue &comp_queue,
                      std::vector<T> &to_perform) const;
    public:
      void add_to_ready_queue(Operation *op, RtEvent ready);
      bool process_ready_queue(void);
    public:
      void add_to_task_queue(TaskOp *op, RtEvent ready);
      bool process_enqueue_task_queue(void);
    public:
      void add_to_distribute_task_queue(TaskOp *op, RtEvent ready);
      bool process_distribute_task_queue(void);
    public:
      void add_to_launch_task_queue(TaskOp *op, RtEvent ready);
      bool process_launch_task_queue(void);
    public:
      void add_to_resolution_queue(Operation *op, RtEvent ready);
      bool process_resolution_queue(void);
    public:
      void add_to_trigger_execution_queue(Operation *op, RtEvent ready);
      bool process_trigger_execution_queue(void);
    public:
      void add_to_deferred_execution_queue(Operation *op, RtEvent ready);
      bool process_deferred_execution_queue(void);
    public:
      void add_to_trigger_completion_queue(Operation *op, RtEvent ready);
      bool process_trigger_completion_queue(void);
    public:
      void add_to_deferred_completion_queue(Operation *op, RtEvent ready);
      bool process_deferred_completion_queue(void);
    public:
      void add_to_trigger_commit_queue(Operation *op, RtEvent ready); 
      bool process_trigger_commit_queue(void);
    public:
      void add_to_deferred_commit_queue(Operation *op, RtEvent ready,
                                        bool deactivate);
      bool process_deferred_commit_queue(void);
      bool process_post_end_tasks(void);
    public:
      void register_executing_child(Operation *op);
      void register_child_executed(Operation *op);
      void register_child_complete(Operation *op);
      void register_child_commit(Operation *op); 
      ReorderBufferEntry& find_rob_entry(Operation *op);
      ApEvent register_implicit_dependences(Operation *op, 
                              RtEvent &mappin_fence_event);
    public:
      RtEvent get_current_mapping_fence_event(void);
      ApEvent get_current_execution_fence_event(void);
      // Break this into two pieces since we know that there are some
      // kinds of operations (like deletions) that want to act like 
      // one-sided fences (e.g. waiting on everything before) but not
      // preventing re-ordering for things afterwards
      void perform_fence_analysis(Operation *op, 
          std::set<ApEvent> &preconditions, bool mapping, bool execution);
      void update_current_fence(FenceOp *op,
                                        bool mapping, bool execution);
      void update_current_implicit_creation(Operation *op);
    public:
      virtual void begin_trace(TraceID tid, bool logical_only,
          bool static_trace, const std::set<RegionTreeID> *managed, bool dep,
          Provenance *provenance);
      virtual void end_trace(TraceID tid, bool deprecated,
                             Provenance *provenance);
      virtual void record_previous_trace(LogicalTrace *trace);
      virtual void invalidate_trace_cache(LogicalTrace *trace,
                                          Operation *invalidator);
      virtual void record_blocking_call(void);
    public:
      virtual void issue_frame(FrameOp *frame, ApEvent frame_termination);
      virtual void perform_frame_issue(FrameOp *frame, 
                                       ApEvent frame_termination);
      virtual void finish_frame(ApEvent frame_termination);
    public:
      virtual void increment_outstanding(void);
      virtual void decrement_outstanding(void);
      virtual void increment_pending(void);
      virtual void decrement_pending(TaskOp *child);
      virtual void decrement_pending(bool need_deferral);
      virtual void increment_frame(void);
      virtual void decrement_frame(void);
    public:
#ifdef DEBUG_LEGION_COLLECTIVES
      virtual MergeCloseOp* get_merge_close_op(Operation *op,
                                               RegionTreeNode *node);
      virtual RefinementOp* get_refinement_op(Operation *op,
                                               RegionTreeNode *node);
#else
      virtual MergeCloseOp* get_merge_close_op(void);
      virtual RefinementOp* get_refinement_op(void);
#endif
      virtual VirtualCloseOp* get_virtual_close_op(void);
    public:
      virtual void pack_inner_context(Serializer &rez) const;
      static InnerContext* unpack_inner_context(Deserializer &derez,
                                                Runtime *runtime);
    public:
      bool nonexclusive_virtual_mapping(unsigned index);
      virtual InnerContext* find_parent_physical_context(unsigned index);
    public:
      // Override by RemoteTask and TopLevelTask
      virtual InnerContext* find_top_context(InnerContext *previous = NULL);
    public:
      void configure_context(MapperManager *mapper, TaskPriority priority);
      virtual void initialize_region_tree_contexts(
          const std::vector<RegionRequirement> &clone_requirements,
          const LegionVector<VersionInfo> &version_infos,
          const std::vector<ApUserEvent> &unmap_events);
      virtual EquivalenceSet* create_initial_equivalence_set(unsigned idx1,
                                                  const RegionRequirement &req);
      virtual void refine_equivalence_sets(unsigned req_index, 
                                           IndexSpaceNode *node,
                                           const FieldMask &refinement_mask,
                                           std::vector<RtEvent> &applied_events,
                                           bool sharded = false);
      virtual void invalidate_region_tree_contexts(const bool is_top_level_task,
                            std::set<RtEvent> &applied,
                            const ShardMapping *mapping = NULL,
                            ShardID source_shard = 0);
      void invalidate_created_requirement_contexts(const bool is_top_level_task,
                            std::set<RtEvent> &applied,
                            const ShardMapping *mapping, ShardID source_shard);
      virtual void receive_created_region_contexts(
                          const std::vector<RegionNode*> &created_regions,
                          const std::vector<EqKDTree*> &created_trees,
                          std::set<RtEvent> &applied_events,
                          const ShardMapping *mapping, ShardID source_shard);
      void invalidate_region_tree_context(const RegionRequirement &req,
          unsigned req_index, std::set<RtEvent> &applied_events, 
          bool filter_specific_fields);
    public:
      virtual ProjectionSummary* construct_projection_summary(
          Operation *op, unsigned index, const RegionRequirement &req,
          LogicalState *owner, const ProjectionInfo &proj_info);
      virtual bool has_interfering_shards(ProjectionSummary *one,
                                          ProjectionSummary *two);
      virtual bool match_timeouts(std::vector<LogicalUser*> &timeouts,
                                  std::vector<LogicalUser*> &to_delete,
                                  TimeoutMatchExchange *&exchange);
    public:
      void record_fill_view_creation(FillView *view);
      void record_fill_view_creation(DistributedID future_did, FillView *view);
      FillView* find_or_create_fill_view(FillOp *op, 
                             const void *value, size_t value_size);
      FillView* find_or_create_fill_view(FillOp *op,
                             const Future &future, bool &set_value);
      FillView* find_fill_view(const void *value, size_t value_size);
      FillView* find_fill_view(const Future &future);
    public:
      virtual void notify_instance_deletion(PhysicalManager *deleted); 
      virtual void add_subscriber_reference(PhysicalManager *manager) 
        { add_nested_resource_ref(manager->did); }
      virtual bool remove_subscriber_reference(PhysicalManager *manager)
        { return remove_nested_resource_ref(manager->did); }
    public:
      virtual const std::vector<PhysicalRegion>& begin_task(Processor proc);
      virtual void end_task(const void *res, size_t res_size, bool owned,
                      PhysicalInstance inst, FutureFunctor *callback_functor,
                      const Realm::ExternalInstanceResource *resource,
                      void (*freefunc)(const Realm::ExternalInstanceResource&),
                      const void *metadataptr, size_t metadatasize,
                      ApEvent effects);
      virtual void post_end_task(FutureInstance *instance,
                                 void *metadata, size_t metasize,
                                 FutureFunctor *callback_functor,
                                 bool own_callback_functor);
      virtual void handle_mispredication(void);
    public:
      virtual void destroy_lock(Lock l);
      virtual Grant acquire_grant(const std::vector<LockRequest> &requests);
      virtual void release_grant(Grant grant);
    public:
      virtual void destroy_phase_barrier(PhaseBarrier pb);
    public:
      void perform_barrier_dependence_analysis(Operation *op,
            const std::vector<PhaseBarrier> &wait_barriers,
            const std::vector<PhaseBarrier> &arrive_barriers,
            MustEpochOp *must_epoch = NULL);
    protected:
      void analyze_barrier_dependences(Operation *op,
            const std::vector<PhaseBarrier> &barriers,
            MustEpochOp *must_epoch, bool previous_gen);
    public:
      virtual DynamicCollective create_dynamic_collective(
                                                  unsigned arrivals,
                                                  ReductionOpID redop,
                                                  const void *init_value,
                                                  size_t init_size);
      virtual void destroy_dynamic_collective(DynamicCollective dc);
      virtual void arrive_dynamic_collective(DynamicCollective dc,
                        const void *buffer, size_t size, unsigned count);
      virtual void defer_dynamic_collective_arrival(DynamicCollective dc,
                                                    const Future &future,
                                                    unsigned count);
      virtual Future get_dynamic_collective_result(DynamicCollective dc,
                                                   Provenance *provenance);
      virtual DynamicCollective advance_dynamic_collective(
                                                   DynamicCollective dc);
    public:
      virtual TaskPriority get_current_priority(void) const;
      virtual void set_current_priority(TaskPriority priority); 
    public:
      static void handle_compute_equivalence_sets_request(Deserializer &derez,
                                     Runtime *runtime, AddressSpaceID source);
      static void handle_compute_equivalence_sets_response(Deserializer &derez,
                                                           Runtime *runtime);
      static void handle_output_equivalence_set_request(Deserializer &derez,
                                     Runtime *runtime);
      static void handle_output_equivalence_set_response(Deserializer &derez,
                                     Runtime *runtime, AddressSpaceID source);
    public:
      static void handle_prepipeline_stage(const void *args);
      static void handle_dependence_stage(const void *args);
      static void handle_ready_queue(const void *args);
      static void handle_enqueue_task_queue(const void *args);
      static void handle_distribute_task_queue(const void *args);
      static void handle_launch_task_queue(const void *args);
      static void handle_resolution_queue(const void *args);
      static void handle_trigger_execution_queue(const void *args);
      static void handle_deferred_execution_queue(const void *args);
      static void handle_trigger_completion_queue(const void *args);
      static void handle_deferred_completion_queue(const void *args);
      static void handle_trigger_commit_queue(const void *args);
      static void handle_deferred_commit_queue(const void *args);
      static void handle_post_end_task(const void *args);
    public:
      void send_context(AddressSpaceID source);
    public:
      // These three methods guard all access to the creation of views onto
      // physical instances within a parent task context. This is important
      // because we need to guarantee the invariant that for every given 
      // physical instance in a context it has at most one logical view
      // that represents its state in the physical analysis.
      // Be careful here! These methods should be called on a context
      // that is the result of find_parent_physical_context to account
      // for virtual mappings
      void convert_individual_views(const std::vector<PhysicalManager*> &srcs,
                                    std::vector<IndividualView*> &views,
                                    CollectiveMapping *mapping = NULL);
      void convert_individual_views(const InstanceSet &sources,
                                    std::vector<IndividualView*> &views,
                                    CollectiveMapping *mapping = NULL);
      void convert_analysis_views(const InstanceSet &targets,
                       LegionVector<FieldMaskSet<InstanceView> > &target_views);
      IndividualView* create_instance_top_view(PhysicalManager *manager,
                                AddressSpaceID source,
                                CollectiveMapping *mapping = NULL);
      virtual CollectiveResult* find_or_create_collective_view(RegionTreeID tid,
          const std::vector<DistributedID> &instances, RtEvent &ready);
      void notify_collective_deletion(RegionTreeID tid, DistributedID did);
    protected:
      RtEvent dispatch_collective_invalidation(
          const CollectiveResult *collective, const FieldMask &invalid_mask,
          const FieldMaskSet<CollectiveResult> &replacements);
      CollectiveResult* find_or_create_collective_view(RegionTreeID tid,
          const std::vector<DistributedID> &instances);
      RtEvent create_collective_view(DistributedID creator_did,
          DistributedID collective_did, CollectiveMapping *mapping,
          const std::vector<DistributedID> &individual_dids);
      static void release_collective_view(Runtime *runtime, 
          DistributedID context_did, DistributedID collective_did);
    public:
      static void handle_create_collective_view(Deserializer &derez,
                                                Runtime *runtime);
      static void handle_delete_collective_view(Deserializer &derez,
                                                Runtime *runtime);
      static void handle_release_collective_view(Deserializer &derez,
                                                 Runtime *runtime);
    protected:
      void execute_task_launch(TaskOp *task, bool index, 
                               LogicalTrace *current_trace, 
                               Provenance *provenance, 
                               bool silence_warnings, bool inlining_enabled);
    public:
      void clone_local_fields(
          std::map<FieldSpace,std::vector<LocalFieldInfo> > &child_local) const;
#ifdef DEBUG_LEGION
      // This is a helpful debug method that can be useful when called from
      // a debugger to find the earliest operation that hasn't mapped yet
      // which is especially useful when debugging scheduler hangs
      Operation* get_earliest(void) const;
#endif
#ifdef LEGION_SPY
      void register_implicit_replay_dependence(Operation *op);
#endif
    public:
      const ContextID tree_context;
      const bool full_inner_context;
    protected:
      // This is immutable except for remote contexts which unpack it 
      // after the object has already been created
      bool concurrent_context;
      bool finished_execution;
      bool has_inline_accessor; 
    protected:
      mutable LocalLock                         privilege_lock;
      unsigned                                  next_created_index;
      // Application tasks can manipulate these next two data
      // structure by creating regions and fields, make sure you are
      // holding the operation lock when you are accessing them
      // We use a region requirement with an empty privilege_fields
      // set to indicate regions on which we have privileges for 
      // all fields because this is a created region instead of
      // a created field.
      std::map<unsigned,RegionRequirement>      created_requirements;
      std::map<unsigned,bool>                   returnable_privileges;
      // Number of outstanding deletions using this created requirement
      // The last one to send the count to zero actually gets to delete
      // the requirement and the logical region
      std::map<unsigned,unsigned>               deletion_counts; 
    protected:
      // Equivalence set trees are used for finding the equivalence sets
      // for a given parent region requirement. Note that each of these
      // trees comes with an associated tree lock that guarantees that 
      // invalidation are exclusive with respect to all other kinds of
      // operations that traverse the equivalence set trees
      class EqKDRoot {
      public:
        EqKDRoot(void);
        EqKDRoot(EqKDTree *tree);
        EqKDRoot(const EqKDRoot &rhs) = delete;
        EqKDRoot(EqKDRoot &&rhs);
        ~EqKDRoot(void);
      public:
        EqKDRoot& operator=(const EqKDRoot &rhs) = delete;
        EqKDRoot& operator=(EqKDRoot &&rhs);
      public:
        EqKDTree *tree;
        LocalLock *lock;
      };
      std::map<unsigned,EqKDRoot>                       equivalence_set_trees;
      // Pending computations for equivalence set trees
      std::map<unsigned,RtUserEvent>            pending_equivalence_set_trees;
    protected:
      Mapper::ContextConfigOutput           context_configuration;
      TaskTreeCoordinates                   context_coordinates;
    protected:
      const std::vector<unsigned>           &parent_req_indexes;
      const std::vector<bool>               &virtual_mapped;
      // Keep track of inline mapping regions for this task
      // so we can see when there are conflicts, note that accessing
      // this data structure requires the inline lock because
      // unordered detach operations can touch it without synchronizing
      // with the executing task
      LegionList<PhysicalRegion,TASK_INLINE_REGION_ALLOC> inline_regions;
    protected:
      mutable LocalLock                     child_op_lock;
      // Track whether this task has finished executing
      size_t total_children_count; // total number of sub-operations
      size_t executing_children_count;
      size_t executed_children_count;
      size_t total_summary_count;
      size_t total_tunable_count;
      std::atomic<size_t> outstanding_children_count; 
      std::deque<ReorderBufferEntry> reorder_buffer;
      // For tracking any operations that come from outside the
      // task like a garbage collector that need to be inserted
      // into the stream of operations from the task
      std::vector<Operation*> unordered_ops;
#ifdef LEGION_SPY
      // Some help for Legion Spy for validating fences
      std::deque<UniqueID> ops_since_last_fence;
      std::set<ApEvent> previous_completion_events;
      // And for verifying the cummulativity property of task
      // (e.g. that they are not complete until all their children are)
      std::vector<ApEvent> cummulative_child_completion_events;
#endif
    protected: // Queues for fusing together small meta-tasks
      mutable LocalLock                               prepipeline_lock;
      std::deque<std::pair<Operation*,GenerationID> > prepipeline_queue;
    protected:
      mutable LocalLock                               dependence_lock;
      std::deque<Operation*>                          dependence_queue;
      RtEvent                                         dependence_precondition;
    protected: 
      mutable LocalLock                               ready_lock;
      std::list<QueueEntry<Operation*> >              ready_queue;
      CompletionQueue                                 ready_comp_queue;
    protected:
      mutable LocalLock                               enqueue_task_lock;
      std::list<QueueEntry<TaskOp*> >                 enqueue_task_queue;
      CompletionQueue                                 enqueue_task_comp_queue;
    protected:
      mutable LocalLock                               distribute_task_lock;
      std::list<QueueEntry<TaskOp*> >                 distribute_task_queue;
      CompletionQueue                                distribute_task_comp_queue;
    protected:
      mutable LocalLock                               launch_task_lock;
      std::list<QueueEntry<TaskOp*> >                 launch_task_queue;
      CompletionQueue                                 launch_task_comp_queue;
    protected:
      mutable LocalLock                               resolution_lock;
      std::list<QueueEntry<Operation*> >              resolution_queue;
      CompletionQueue                                 resolution_comp_queue;
    protected:
      mutable LocalLock                               trigger_execution_lock;
      std::list<QueueEntry<Operation*> >              trigger_execution_queue;
      CompletionQueue                             trigger_execution_comp_queue;
    protected:
      mutable LocalLock                               deferred_execution_lock;
      std::list<QueueEntry<Operation*> >              deferred_execution_queue;
      CompletionQueue                             deferred_execution_comp_queue;
    protected:
      mutable LocalLock                               trigger_completion_lock;
      std::list<QueueEntry<Operation*> >              trigger_completion_queue;
      CompletionQueue                             trigger_completion_comp_queue;
    protected:
      mutable LocalLock                               deferred_completion_lock;
      std::list<QueueEntry<Operation*> >              deferred_completion_queue;
      CompletionQueue                            deferred_completion_comp_queue;
    protected:
      mutable LocalLock                               trigger_commit_lock;
      std::list<QueueEntry<Operation*> >              trigger_commit_queue;
      CompletionQueue                                 trigger_commit_comp_queue;
    protected:
      mutable LocalLock                               deferred_commit_lock;
      std::list<QueueEntry<std::pair<Operation*,bool> > > deferred_commit_queue;
      CompletionQueue                                deferred_commit_comp_queue;
    protected:
      mutable LocalLock                               post_task_lock;
      std::list<PostTaskArgs>                         post_task_queue;
      CompletionQueue                                 post_task_comp_queue;
    protected:
      // Traces for this task's execution
      LegionMap<TraceID,LogicalTrace*,TASK_TRACES_ALLOC> traces;
      LogicalTrace *current_trace;
      LogicalTrace *previous_trace;
      // ID is either 0 for not replaying, 1 for replaying, or
      // the event id for signaling that the status isn't ready 
      std::atomic<realm_id_t> physical_trace_replay_status;
      bool valid_wait_event;
      RtUserEvent window_wait;
      std::deque<ApEvent> frame_events;
    protected:
      // Number of sub-tasks ready to map
      unsigned outstanding_subtasks;
      // Number of mapped sub-tasks that are yet to run
      unsigned pending_subtasks;
      // Number of pending_frames
      unsigned pending_frames;
      // Track whether this context is current active for scheduling
      // indicating that it is no longer far enough ahead
      bool currently_active_context;
    protected:
#ifdef LEGION_SPY
      UniqueID current_fence_uid;
      GenerationID current_mapping_fence_gen;
#endif
      unsigned current_mapping_fence_index;
      RtEvent current_mapping_fence_event;
      ApEvent current_execution_fence_event;
      unsigned current_execution_fence_index;
      // We currently do not track dependences for dependent partitioning
      // operations on index partitions and their subspaces directly, so 
      // we instead use this to ensure mapping dependence ordering with 
      // any operations which might need downstream information about 
      // partitions or subspaces. Note that this means that all dependent
      // partitioning operations are guaranteed to map in order currently
      // We've now extended this to include creation operations and pending
      // partition operations as well for similar reasons, so now this 
      // is a general operation class
      Operation *last_implicit_creation;
      GenerationID last_implicit_creation_gen;
    protected:
      // For managing changing task priorities
      ApEvent realm_done_event;
      TaskPriority current_priority;
    protected: // Instance top view data structures
      mutable LocalLock                          instance_view_lock;
      std::map<PhysicalManager*,IndividualView*> instance_top_views;
      std::map<PhysicalManager*,RtUserEvent>     pending_top_views;
    protected:
      // Field allocation data
      std::map<FieldSpace,FieldAllocatorImpl*> field_allocators;
    protected:
      // Our cached set of index spaces for immediate domains
      std::map<Domain,IndexSpace> index_launch_spaces;
    protected:
      // Dependence tracking information for phase barriers
      mutable LocalLock                                   phase_barrier_lock;
      struct BarrierContribution {
      public:
        BarrierContribution(void) : op(NULL), gen(0), uid(0), muid(0) { }
        BarrierContribution(Operation *o, GenerationID g, 
                            UniqueID u, UniqueID m, size_t bg)
          : op(o), gen(g), uid(u), muid(m), bargen(bg) { }
      public:
        Operation *op;
        GenerationID gen;
        UniqueID uid;
        UniqueID muid; // must epoch uid
        size_t bargen; // the barrier generation
      };
      std::map<size_t,std::list<BarrierContribution> > barrier_contributions;
    protected:
      // Track information for locally allocated fields
      mutable LocalLock                                 local_field_lock;
      std::map<FieldSpace,std::vector<LocalFieldInfo> > local_field_infos;
    protected:
      // Cache for fill views
      mutable LocalLock     fill_view_lock;            
      std::list<FillView*>  value_fill_view_cache;
      std::list<std::pair<FillView*,DistributedID> > future_fill_view_cache;
      static const size_t MAX_FILL_VIEW_CACHE_SIZE = 64;
    protected:
      // This data structure should only be accessed during the logical
      // analysis stage of the pipeline and therefore no lock is needed
      std::map<IndexTreeNode*,
        std::vector<AttachProjectionFunctor*> > attach_functions;
    protected:
      // Resources that can build up over a task's lifetime
      LegionDeque<Reservation,TASK_RESERVATION_ALLOC> context_locks;
      LegionDeque<ApBarrier,TASK_BARRIER_ALLOC> context_barriers;
    protected:
      // Collective instance rendezvous data structures
      mutable LocalLock                                 collective_lock;
      // Only valid on the onwer context node
      std::map<RegionTreeID,
               std::vector<CollectiveResult*> >         collective_results;
    public:
      // TODO: delete this once we properly replay mapping dependences
      RtEvent inorder_concurrent_replay_analysis;
      RtEvent total_hack_function_for_inorder_concurrent_replay_analysis(
                                                            RtEvent mapped);
    };

    /**
     * \class TopLevelContext
     * This is the top-level task context that
     * exists at the root of a task tree. In
     * general there will only be one of these
     * per application unless mappers decide to
     * create their own tasks for performing
     * computation.
     */
    class TopLevelContext : public InnerContext {
    public:
      TopLevelContext(Runtime *runtime, Processor executing,
          DistributedID id = 0, CollectiveMapping *mapping = NULL);
      TopLevelContext(const TopLevelContext &rhs) = delete;
      virtual ~TopLevelContext(void);
    public:
      TopLevelContext& operator=(const TopLevelContext &rhs) = delete;
    public:
      virtual void pack_remote_context(Serializer &rez, 
          AddressSpaceID target, bool replicate = false);
      virtual InnerContext* find_parent_context(void);
      virtual UniqueID get_unique_id(void) const { return root_uid; }
    public:
      virtual InnerContext* find_outermost_local_context(
                          InnerContext *previous = NULL);
      virtual InnerContext* find_top_context(InnerContext *previous = NULL);
    public:
      virtual void receive_created_region_contexts(
                          const std::vector<RegionNode*> &created_regions,
                          const std::vector<EqKDTree*> &created_trees,
                          std::set<RtEvent> &applied_events,
                          const ShardMapping *mapping, ShardID source_shard);
      virtual RtEvent compute_equivalence_sets(unsigned req_index,
                      const std::vector<EqSetTracker*> &targets,
                      const std::vector<AddressSpaceID> &target_spaces,
                      AddressSpaceID creation_target_space,
                      IndexSpaceExpression *expr, const FieldMask &mask);
      virtual RtEvent record_output_equivalence_set(EqSetTracker *source,
                      AddressSpaceID source_space, unsigned req_index,
                      EquivalenceSet *set, const FieldMask &mask);
    public:
      const UniqueID root_uid;
    protected:
      std::vector<RegionRequirement>       dummy_requirements;
      std::vector<OutputRequirement>       dummy_output_requirements;
      std::vector<unsigned>                dummy_indexes;
      std::vector<bool>                    dummy_mapped;
    };

    /**
     * \class ReplicateContext
     * A replicate context is a special kind of inner context for
     * executing control-replicated tasks.
     */
    class ReplicateContext : public InnerContext {
    public: 
      struct ISBroadcast {
      public:
        ISBroadcast(void) : expr_id(0), did(0), double_buffer(false) { }
        ISBroadcast(IndexSpaceID i, IndexTreeID t, IndexSpaceExprID e, 
                    DistributedID d, bool db)
          : space_id(i), tid(t), expr_id(e), did(d), double_buffer(db) { }
      public:
        IndexSpaceID space_id;
        IndexTreeID tid;
        IndexSpaceExprID expr_id;
        DistributedID did;
        bool double_buffer;
      };
      struct IPBroadcast {
      public:
        IPBroadcast(void) : did(0), double_buffer(false) { }
        IPBroadcast(IndexPartitionID p, DistributedID d, bool db) 
          : pid(p), did(d), double_buffer(db) { }
      public:
        IndexPartitionID pid;
        DistributedID did;
        bool double_buffer;
      };
      struct FSBroadcast { 
      public:
        FSBroadcast(void) : did(0), double_buffer(false) { }
        FSBroadcast(FieldSpaceID i, DistributedID d, bool db) 
          : space_id(i), did(d), double_buffer(db) { }
      public:
        FieldSpaceID space_id;
        DistributedID did;
        bool double_buffer;
      };
      struct FIDBroadcast {
      public:
        FIDBroadcast(void) : field_id(0), double_buffer(false) { }
        FIDBroadcast(FieldID fid, bool db)
          : field_id(fid), double_buffer(db) { }
      public:
        FieldID field_id;
        bool double_buffer;
      };
      struct LRBroadcast {
      public:
        LRBroadcast(void) : tid(0), double_buffer(false) { }
        LRBroadcast(RegionTreeID t, DistributedID d, bool db) :
          tid(t), did(d), double_buffer(db) { }
      public:
        RegionTreeID tid;
        DistributedID did;
        bool double_buffer;
      };
      struct DIDBroadcast {
        DIDBroadcast(void) : did(0), double_buffer(false) { }
        DIDBroadcast(DistributedID d, bool db) : did(d), double_buffer(db) { }
      public:
        DistributedID did;
        bool double_buffer;
      };
      struct IntraSpaceDeps {
      public:
        std::map<ShardID,RtEvent> ready_deps;
        std::map<ShardID,RtUserEvent> pending_deps;
      };
    public:
      template<typename T, bool LOGICAL, bool SINGLE=false>
      class ReplBarrier {
      public:
        ReplBarrier(void) : owner(false) { }
        ReplBarrier(const ReplBarrier &rhs) = delete;
        ReplBarrier(ReplBarrier &&rhs)
          : barrier(rhs.barrier), owner(rhs.owner) { rhs.owner = false; }
        ~ReplBarrier(void) 
          { if (owner && barrier.exists()) barrier.destroy_barrier(); }
      public:
        ReplBarrier& operator=(const ReplBarrier &rhs) = delete;
        inline ReplBarrier& operator=(ReplBarrier &&rhs)
          {
            if (owner && barrier.exists()) barrier.destroy_barrier();
            barrier = rhs.barrier;
            owner = rhs.owner;
            rhs.owner = false;
            return *this;
          }
      public:
#ifdef DEBUG_LEGION_COLLECTIVES
        inline T next(ReplicateContext *ctx, ReductionOpID redop = 0,
            const void *init_value = NULL, size_t init_size = 0)
#else
        inline T next(ReplicateContext *ctx)
#endif
        {
          if (!barrier.exists())
          {
            if (LOGICAL)
              owner = ctx->create_new_logical_barrier(barrier,
#ifdef DEBUG_LEGION_COLLECTIVES
                  redop, init_value, init_size,
#endif
                  SINGLE ? 1 : ctx->total_shards);
            else
              owner = ctx->create_new_replicate_barrier(barrier,
#ifdef DEBUG_LEGION_COLLECTIVES
                  redop, init_value, init_size,
#endif
                  SINGLE ? 1 : ctx->total_shards);
          }
          const T result = barrier;
          Runtime::advance_barrier(barrier);
          return result;
        }
      private:
        T barrier;
        bool owner;
      };
    public:
      enum ReplicateAPICall {
        REPLICATE_PERFORM_REGISTRATION_CALLBACK,
        REPLICATE_CONSENSUS_MATCH,
        REPLICATE_REGISTER_TASK_VARIANT,
        REPLICATE_GENERATE_DYNAMIC_TRACE_ID,
        REPLICATE_GENERATE_DYNAMIC_MAPPER_ID,
        REPLICATE_GENERATE_DYNAMIC_PROJECTION_ID,
        REPLICATE_GENERATE_DYNAMIC_SHARDING_ID,
        REPLICATE_GENERATE_DYNAMIC_TASK_ID,
        REPLICATE_GENERATE_DYNAMIC_REDUCTION_ID,
        REPLICATE_GENERATE_DYNAMIC_SERDEZ_ID,
        REPLICATE_CREATE_INDEX_SPACE,
        REPLICATE_CREATE_UNBOUND_INDEX_SPACE,
        REPLICATE_UNION_INDEX_SPACES,
        REPLICATE_INTERSECT_INDEX_SPACES,
        REPLICATE_SUBTRACT_INDEX_SPACES,
        REPLICATE_CREATE_SHARED_OWNERSHIP,
        REPLICATE_DESTROY_INDEX_SPACE,
        REPLICATE_DESTROY_INDEX_PARTITION,
        REPLICATE_CREATE_EQUAL_PARTITION,
        REPLICATE_CREATE_PARTITION_BY_WEIGHTS,
        REPLICATE_CREATE_PARTITION_BY_UNION,
        REPLICATE_CREATE_PARTITION_BY_INTERSECTION,
        REPLICATE_CREATE_PARTITION_BY_DIFFERENCE,
        REPLICATE_CREATE_CROSS_PRODUCT_PARTITIONS,
        REPLICATE_CREATE_ASSOCIATION,
        REPLICATE_CREATE_RESTRICTED_PARTITION,
        REPLICATE_CREATE_PARTITION_BY_DOMAIN,
        REPLICATE_CREATE_PARTITION_BY_FIELD,
        REPLICATE_CREATE_PARTITION_BY_IMAGE,
        REPLICATE_CREATE_PARTITION_BY_IMAGE_RANGE,
        REPLICATE_CREATE_PARTITION_BY_PREIMAGE,
        REPLICATE_CREATE_PARTITION_BY_PREIMAGE_RANGE,
        REPLICATE_CREATE_PENDING_PARTITION,
        REPLICATE_CREATE_INDEX_SPACE_UNION,
        REPLICATE_CREATE_INDEX_SPACE_INTERSECTION,
        REPLICATE_CREATE_INDEX_SPACE_DIFFERENCE,
        REPLICATE_CREATE_FIELD_SPACE,
        REPLICATE_DESTROY_FIELD_SPACE,
        REPLICATE_ALLOCATE_FIELD,
        REPLICATE_FREE_FIELD,
        REPLICATE_ALLOCATE_FIELDS,
        REPLICATE_FREE_FIELDS,
        REPLICATE_CREATE_LOGICAL_REGION,
        REPLICATE_DESTROY_LOGICAL_REGION,
        REPLICATE_RESET_EQUIVALENCE_SETS,
        REPLICATE_CREATE_FIELD_ALLOCATOR,
        REPLICATE_EXECUTE_TASK,
        REPLICATE_EXECUTE_INDEX_SPACE,
        REPLICATE_REDUCE_FUTURE_MAP,
        REPLICATE_CONSTRUCT_FUTURE_MAP,
        REPLICATE_FUTURE_MAP_GET_ALL_FUTURES,
        REPLICATE_FUTURE_MAP_WAIT_ALL_FUTURES,
        REPLICATE_MAP_REGION,
        REPLICATE_REMAP_REGION,
        REPLICATE_FILL_FIELDS,
        REPLICATE_DISCARD_FIELDS,
        REPLICATE_ISSUE_COPY,
        REPLICATE_ATTACH_RESOURCE,
        REPLICATE_DETACH_RESOURCE,
        REPLICATE_INDEX_ATTACH_RESOURCE,
        REPLICATE_INDEX_DETACH_RESOURCE,
        REPLICATE_ACQUIRE,
        REPLICATE_RELEASE,
        REPLICATE_MUST_EPOCH,
        REPLICATE_TIMING_MEASUREMENT,
        REPLICATE_TUNABLE_SELECTION,
        REPLICATE_MAPPING_FENCE,
        REPLICATE_EXECUTION_FENCE,
        REPLICATE_BEGIN_TRACE,
        REPLICATE_END_TRACE,
        REPLICATE_CREATE_PHASE_BARRIER,
        REPLICATE_DESTROY_PHASE_BARRIER,
        REPLICATE_ADVANCE_PHASE_BARRIER,
        REPLICATE_ADVANCE_DYNAMIC_COLLECTIVE,
        REPLICATE_END_TASK,
        REPLICATE_FUTURE_FROM_VALUE,
        REPLICATE_ATTACH_TASK_INFO,
        REPLICATE_ATTACH_INDEX_SPACE_INFO,
        REPLICATE_ATTACH_INDEX_PARTITION_INFO,
        REPLICATE_ATTACH_FIELD_SPACE_INFO,
        REPLICATE_ATTACH_FIELD_INFO,
        REPLICATE_ATTACH_LOGICAL_REGION_INFO,
        REPLICATE_ATTACH_LOGICAL_PARTITION_INFO,
      };
    public:
      class AttachDetachShardingFunctor : public ShardingFunctor {
      public:
        AttachDetachShardingFunctor(void) { }
        virtual ~AttachDetachShardingFunctor(void) { }
      public:
        virtual ShardID shard(const DomainPoint &point,
                              const Domain &full_space,
                              const size_t total_shards);
      };
      /**
       * \class UniversalShardingFunctor
       * This is a special sharding functor only used during the logical 
       * analysis and has no bearing on the actual computed sharding. For
       * some operations we need to have a way to say that an individual
       * operation will be analyzed collectively on all the shards. This
       * sharding function accomplishes this by mapping all the points to
       * the non-shard UINT_MAX which will be non-interfering with 
       * This maps all the points to the non-shard UINT_MAX which means that
       * it will interfere with any normally mapped projections but not with
       * any other projections which will be analyzed on all the nodes.
       */
      class UniversalShardingFunctor : public ShardingFunctor {
      public:
        UniversalShardingFunctor(void) { }
        virtual ~UniversalShardingFunctor(void) { }
      public:
        virtual ShardID shard(const DomainPoint &point,
                              const Domain &full_space,
                              const size_t total_shards) { return UINT_MAX; }
      };
    public:
      ReplicateContext(Runtime *runtime, ShardTask *owner,int d,bool full_inner,
                       const std::vector<RegionRequirement> &reqs,
                       const std::vector<OutputRequirement> &output_reqs,
                       const std::vector<unsigned> &parent_indexes,
                       const std::vector<bool> &virt_mapped,
                       ApEvent execution_fence_event,
                       ShardManager *manager, bool inline_task, 
                       bool implicit_task = false, bool concurrent = false);
      ReplicateContext(const ReplicateContext &rhs) = delete;
      virtual ~ReplicateContext(void);
    public:
      ReplicateContext& operator=(const ReplicateContext &rhs) = delete;
    public:
      inline int get_shard_collective_radix(void) const
        { return shard_collective_radix; }
      inline int get_shard_collective_log_radix(void) const
        { return shard_collective_log_radix; }
      inline int get_shard_collective_stages(void) const
        { return shard_collective_stages; }
      inline int get_shard_collective_participating_shards(void) const
        { return shard_collective_participating_shards; }
      inline int get_shard_collective_last_radix(void) const
        { return shard_collective_last_radix; } 
      virtual ShardID get_shard_id(void) const { return owner_shard->shard_id; }
      virtual DistributedID get_replication_id(void) const;
      virtual size_t get_total_shards(void) const { return total_shards; }
      virtual ContextID get_physical_tree_context(void) const;
    public: // Privilege tracker methods
      virtual void receive_resources(size_t return_index,
              std::map<LogicalRegion,unsigned> &created_regions,
              std::vector<DeletedRegion> &deleted_regions,
              std::set<std::pair<FieldSpace,FieldID> > &created_fields,
              std::vector<DeletedField> &deleted_fields,
              std::map<FieldSpace,unsigned> &created_field_spaces,
              std::map<FieldSpace,std::set<LogicalRegion> > &latent_spaces,
              std::vector<DeletedFieldSpace> &deleted_field_spaces,
              std::map<IndexSpace,unsigned> &created_index_spaces,
              std::vector<DeletedIndexSpace> &deleted_index_spaces,
              std::map<IndexPartition,unsigned> &created_partitions,
              std::vector<DeletedPartition> &deleted_partitions,
              std::set<RtEvent> &preconditions);
    public: // Murmur3Hasher::HashVerifier method
      virtual bool verify_hash(const uint64_t hash[2],
          const char *description, Provenance *provenance, bool every);
    protected:
      void receive_replicate_resources(size_t return_index,
              std::map<LogicalRegion,unsigned> &created_regions,
              std::vector<DeletedRegion> &deleted_regions,
              std::set<std::pair<FieldSpace,FieldID> > &created_fields,
              std::vector<DeletedField> &deleted_fields,
              std::map<FieldSpace,unsigned> &created_field_spaces,
              std::map<FieldSpace,std::set<LogicalRegion> > &latent_spaces,
              std::vector<DeletedFieldSpace> &deleted_field_spaces,
              std::map<IndexSpace,unsigned> &created_index_spaces,
              std::vector<DeletedIndexSpace> &deleted_index_spaces,
              std::map<IndexPartition,unsigned> &created_partitions,
              std::vector<DeletedPartition> &deleted_partitions,
              std::set<RtEvent> &preconditions, RtBarrier &ready_barrier, 
              RtBarrier &mapped_barrier, RtBarrier &execution_barrier);
      void register_region_deletions(ApEvent precondition,
                     const std::map<Operation*,GenerationID> &dependences,
                     std::vector<DeletedRegion> &regions,
                     std::set<RtEvent> &preconditions, RtBarrier &ready_barrier,
                     RtBarrier &mapped_barrier, RtBarrier &execution_barrier);
      void register_field_deletions(ApEvent precondition,
            const std::map<Operation*,GenerationID> &dependences,
            std::vector<DeletedField> &fields,
            std::set<RtEvent> &preconditions, RtBarrier &ready_barrier,
            RtBarrier &mapped_barrier, RtBarrier &execution_barrier);
      void register_field_space_deletions(ApEvent precondition,
                    const std::map<Operation*,GenerationID> &dependences,
                    std::vector<DeletedFieldSpace> &spaces,
                    std::set<RtEvent> &preconditions, RtBarrier &ready_barrier,
                    RtBarrier &mapped_barrier, RtBarrier &execution_barrier);
      void register_index_space_deletions(ApEvent precondition,
                    const std::map<Operation*,GenerationID> &dependences,
                    std::vector<DeletedIndexSpace> &spaces,
                    std::set<RtEvent> &preconditions, RtBarrier &ready_barrier,
                    RtBarrier &mapped_barrier, RtBarrier &execution_barrier);
      void register_index_partition_deletions(ApEvent precondition,
                    const std::map<Operation*,GenerationID> &dependences,
                    std::vector<DeletedPartition> &parts,
                    std::set<RtEvent> &preconditions, RtBarrier &ready_barrier,
                    RtBarrier &mapped_barrier, RtBarrier &execution_barrier);
    public:
      void perform_replicated_region_deletions(
                     std::vector<LogicalRegion> &regions,
                     std::set<RtEvent> &preconditions);
      void perform_replicated_field_deletions(
            std::vector<std::pair<FieldSpace,FieldID> > &fields,
            std::set<RtEvent> &preconditions);
      void perform_replicated_field_space_deletions(
                          std::vector<FieldSpace> &spaces,
                          std::set<RtEvent> &preconditions);
      void perform_replicated_index_space_deletions(
                          std::vector<IndexSpace> &spaces,
                          std::set<RtEvent> &preconditions);
      void perform_replicated_index_partition_deletions(
                          std::vector<IndexPartition> &parts,
                          std::set<RtEvent> &preconditions);
    public:
#ifdef LEGION_USE_LIBDL
      virtual void perform_global_registration_callbacks(
                     Realm::DSOReferenceImplementation *dso, const void *buffer,
                     size_t buffer_size, bool withargs, size_t dedup_tag,
                     RtEvent local_done, RtEvent global_done, 
                     std::set<RtEvent> &preconditions);
#endif
      virtual void print_once(FILE *f, const char *message) const;
      virtual void log_once(Realm::LoggerMessage &message) const;
      virtual Future from_value(const void *value, size_t value_size,
          bool owned, Provenance *provenance, bool shard_local);
      virtual Future from_value(const void *buffer, size_t size, bool owned,
          const Realm::ExternalInstanceResource &resource,
          void (*freefunc)(const Realm::ExternalInstanceResource&),
          Provenance *provenance, bool shard_local);
      virtual Future consensus_match(const void *input, void *output,
          size_t num_elements, size_t element_size, Provenance *provenance); 
    public:
      virtual VariantID register_variant(const TaskVariantRegistrar &registrar,
                          const void *user_data, size_t user_data_size,
                          const CodeDescriptor &desc, size_t ret_size,
                          bool has_ret_size, VariantID vid, bool check_task_id);
      virtual VariantImpl* select_inline_variant(TaskOp *child,
                const std::vector<PhysicalRegion> &parent_regions,
                std::deque<InstanceSet> &physical_instances);
      virtual TraceID generate_dynamic_trace_id(void);
      virtual MapperID generate_dynamic_mapper_id(void);
      virtual ProjectionID generate_dynamic_projection_id(void);
      virtual ShardingID generate_dynamic_sharding_id(void);
      virtual TaskID generate_dynamic_task_id(void);
      virtual ReductionOpID generate_dynamic_reduction_id(void);
      virtual CustomSerdezID generate_dynamic_serdez_id(void);
      virtual bool perform_semantic_attach(const char *func, unsigned kind,
          const void *arg, size_t arglen, SemanticTag tag, const void *buffer,
          size_t size, bool is_mutable, bool &global, 
          const void *arg2 = NULL, size_t arg2len = 0);
      virtual void post_semantic_attach(void);
    public:
      virtual EquivalenceSet* create_initial_equivalence_set(unsigned idx1,
                                                  const RegionRequirement &req);
      virtual void refine_equivalence_sets(unsigned req_index,
                                           IndexSpaceNode *node,
                                           const FieldMask &refinement_mask,
                                           std::vector<RtEvent> &applied_events,
                                           bool sharded = false);
      virtual void receive_created_region_contexts(
                          const std::vector<RegionNode*> &created_regions,
                          const std::vector<EqKDTree*> &created_trees,
                          std::set<RtEvent> &applied_events,
                          const ShardMapping *mapping, ShardID source_shard);
      bool compute_shard_to_shard_mapping(const ShardMapping &src_mapping,
                std::multimap<ShardID,ShardID> &src_to_dst_mapping) const;
      void handle_created_region_contexts(Deserializer &derez,
                                          std::set<RtEvent> &applied_events);
    public: 
      // Interface to operations performed by a context
      virtual IndexSpace create_index_space(const Domain &domain, 
                                            TypeTag type_tag,
                                            Provenance *provenance);
      virtual IndexSpace create_index_space(const Future &future, 
                                            TypeTag type_tag,
                                            Provenance *provenance);
      virtual IndexSpace create_index_space(
                           const std::vector<DomainPoint> &points,
                           Provenance *provenance);
      virtual IndexSpace create_index_space(
                           const std::vector<Domain> &rects,
                           Provenance *provenance);
      virtual IndexSpace create_unbound_index_space(TypeTag type_tag,
                                                    Provenance *provenance);
    protected:
      IndexSpace create_index_space_replicated(const Domain *bounds,
                                               TypeTag type_tag,
                                               Provenance *provenance);
    public:
      virtual IndexSpace union_index_spaces(
                           const std::vector<IndexSpace> &spaces,
                           Provenance *provenance);
      virtual IndexSpace intersect_index_spaces(
                           const std::vector<IndexSpace> &spaces,
                           Provenance *provenance);
      virtual IndexSpace subtract_index_spaces(
                           IndexSpace left, IndexSpace right,
                           Provenance *provenance);
      virtual void create_shared_ownership(IndexSpace handle);
      virtual void destroy_index_space(IndexSpace handle, 
                                       const bool unordered,
                                       const bool recurse,
                                       Provenance *provenance);
      virtual void create_shared_ownership(IndexPartition handle);
      virtual void destroy_index_partition(IndexPartition handle, 
                                           const bool unordered,
                                           const bool recurse,
                                           Provenance *provenance);
      virtual IndexPartition create_equal_partition(
                                            IndexSpace parent,
                                            IndexSpace color_space,
                                            size_t granularity,
                                            Color color,
                                            Provenance *provenance);
      virtual IndexPartition create_partition_by_weights(IndexSpace parent,
                                            const FutureMap &weights,
                                            IndexSpace color_space,
                                            size_t granularity, 
                                            Color color,
                                            Provenance *provenance);
      virtual IndexPartition create_partition_by_union(
                                            IndexSpace parent,
                                            IndexPartition handle1,
                                            IndexPartition handle2,
                                            IndexSpace color_space,
                                            PartitionKind kind,
                                            Color color,
                                            Provenance *provenance);
      virtual IndexPartition create_partition_by_intersection(
                                            IndexSpace parent,
                                            IndexPartition handle1,
                                            IndexPartition handle2,
                                            IndexSpace color_space,
                                            PartitionKind kind,
                                            Color color,
                                            Provenance *provenance);
      virtual IndexPartition create_partition_by_intersection(
                                            IndexSpace parent,
                                            IndexPartition partition,
                                            PartitionKind kind,
                                            Color color,
                                            bool dominates,
                                            Provenance *provenance);
      virtual IndexPartition create_partition_by_difference(
                                            IndexSpace parent,
                                            IndexPartition handle1,
                                            IndexPartition handle2,
                                            IndexSpace color_space,
                                            PartitionKind kind,
                                            Color color,
                                            Provenance *provenance);
      virtual Color create_cross_product_partitions(
                                            IndexPartition handle1,
                                            IndexPartition handle2,
                              std::map<IndexSpace,IndexPartition> &handles,
                                            PartitionKind kind,
                                            Color color,
                                            Provenance *provenance);
      virtual void create_association(      LogicalRegion domain,
                                            LogicalRegion domain_parent,
                                            FieldID domain_fid,
                                            IndexSpace range,
                                            MapperID id, MappingTagID tag,
                                            const UntypedBuffer &marg,
                                            Provenance *provenance);
      virtual IndexPartition create_restricted_partition(
                                            IndexSpace parent,
                                            IndexSpace color_space,
                                            const void *transform,
                                            size_t transform_size,
                                            const void *extent,
                                            size_t extent_size,
                                            PartitionKind part_kind,
                                            Color color,
                                            Provenance *provenance);
      virtual IndexPartition create_partition_by_domain(
                                            IndexSpace parent,
                                  const std::map<DomainPoint,Domain> &domains,
                                            IndexSpace color_space,
                                            bool perform_intersections,
                                            PartitionKind part_kind,
                                            Color color,
                                            Provenance *provenance);
      virtual IndexPartition create_partition_by_domain(
                                            IndexSpace parent,
                                            const FutureMap &domains,
                                            IndexSpace color_space,
                                            bool perform_intersections,
                                            PartitionKind part_kind,
                                            Color color,
                                            Provenance *provenance,
                                            bool skip_check = false);
      virtual IndexPartition create_partition_by_field(
                                            LogicalRegion handle,
                                            LogicalRegion parent_priv,
                                            FieldID fid,
                                            IndexSpace color_space,
                                            Color color,
                                            MapperID id, MappingTagID tag,
                                            PartitionKind part_kind,
                                            const UntypedBuffer &marg,
                                            Provenance *provenance);
      virtual IndexPartition create_partition_by_image(
                                            IndexSpace handle,
                                            LogicalPartition projection,
                                            LogicalRegion parent,
                                            FieldID fid,
                                            IndexSpace color_space,
                                            PartitionKind part_kind,
                                            Color color,
                                            MapperID id, MappingTagID tag,
                                            const UntypedBuffer &marg,
                                            Provenance *provenance);
      virtual IndexPartition create_partition_by_image_range(
                                            IndexSpace handle,
                                            LogicalPartition projection,
                                            LogicalRegion parent,
                                            FieldID fid,
                                            IndexSpace color_space,
                                            PartitionKind part_kind,
                                            Color color,
                                            MapperID id, MappingTagID tag,
                                            const UntypedBuffer &marg,
                                            Provenance *provenance);
      virtual IndexPartition create_partition_by_preimage(
                                            IndexPartition projection,
                                            LogicalRegion handle,
                                            LogicalRegion parent,
                                            FieldID fid,
                                            IndexSpace color_space,
                                            PartitionKind part_kind,
                                            Color color,
                                            MapperID id, MappingTagID tag,
                                            const UntypedBuffer &marg,
                                            Provenance *provenance);
      virtual IndexPartition create_partition_by_preimage_range(
                                            IndexPartition projection,
                                            LogicalRegion handle,
                                            LogicalRegion parent,
                                            FieldID fid,
                                            IndexSpace color_space,
                                            PartitionKind part_kind,
                                            Color color,
                                            MapperID id, MappingTagID tag,
                                            const UntypedBuffer &marg,
                                            Provenance *provenance);
      virtual IndexPartition create_pending_partition(
                                            IndexSpace parent,
                                            IndexSpace color_space,
                                            PartitionKind part_kind,
                                            Color color,
                                            Provenance *provenance,
                                            bool trust = false);
      virtual IndexSpace create_index_space_union(
                                            IndexPartition parent,
                                            const void *realm_color,
                                            size_t color_size,
                                            TypeTag type_tag,
                                const std::vector<IndexSpace> &handles,
                                            Provenance *provenance);
      virtual IndexSpace create_index_space_union(
                                            IndexPartition parent,
                                            const void *realm_color,
                                            size_t color_size,
                                            TypeTag type_tag,
                                            IndexPartition handle,
                                            Provenance *provenance);
      virtual IndexSpace create_index_space_intersection(
                                            IndexPartition parent,
                                            const void *realm_color,
                                            size_t color_size,
                                            TypeTag type_tag,
                                const std::vector<IndexSpace> &handles,
                                            Provenance *provenance);
      virtual IndexSpace create_index_space_intersection(
                                            IndexPartition parent,
                                            const void *realm_color,
                                            size_t color_size,
                                            TypeTag type_tag,
                                            IndexPartition handle,
                                            Provenance *provenance);
      virtual IndexSpace create_index_space_difference(
                                            IndexPartition parent,
                                            const void *realm_color,
                                            size_t color_size,
                                            TypeTag type_tag,
                                            IndexSpace initial,
                                const std::vector<IndexSpace> &handles,
                                            Provenance *provenance);
      virtual void verify_partition(IndexPartition pid, PartitionKind kind,
                                    const char *function_name);
      virtual FieldSpace create_field_space(Provenance *provenance);
      virtual FieldSpace create_field_space(const std::vector<size_t> &sizes,
                                        std::vector<FieldID> &resulting_fields,
                                        CustomSerdezID serdez_id,
                                        Provenance *provenance);
      virtual FieldSpace create_field_space(const std::vector<Future> &sizes,
                                        std::vector<FieldID> &resulting_fields,
                                        CustomSerdezID serdez_id,
                                        Provenance *provenance);
      FieldSpace create_replicated_field_space(Provenance *provenance,
                                        ShardID *creator_shard = NULL);
      virtual void create_shared_ownership(FieldSpace handle);
      virtual void destroy_field_space(FieldSpace handle,
                                       const bool unordered,
                                       Provenance *provenance);
      virtual FieldID allocate_field(FieldSpace space, size_t field_size,
                                     FieldID fid, bool local,
                                     CustomSerdezID serdez_id,
                                     Provenance *provenance);
      virtual FieldID allocate_field(FieldSpace space, const Future &field_size,
                                     FieldID fid, bool local,
                                     CustomSerdezID serdez_id,
                                     Provenance *provenance);
      virtual void free_field(FieldAllocatorImpl *allocator, FieldSpace space, 
                              FieldID fid, const bool unordered,
                              Provenance *provenance);
      virtual void allocate_fields(FieldSpace space,
                                   const std::vector<size_t> &sizes,
                                   std::vector<FieldID> &resuling_fields,
                                   bool local, CustomSerdezID serdez_id,
                                   Provenance *provenance);
      virtual void allocate_fields(FieldSpace space,
                                   const std::vector<Future> &sizes,
                                   std::vector<FieldID> &resuling_fields,
                                   bool local, CustomSerdezID serdez_id,
                                   Provenance *provenance);
      virtual void free_fields(FieldAllocatorImpl *allocator, FieldSpace space, 
                               const std::set<FieldID> &to_free,
                               const bool unordered,
                               Provenance *provenance);
      virtual LogicalRegion create_logical_region(
                                            IndexSpace index_space,
                                            FieldSpace field_space,
                                            const bool task_local,
                                            Provenance *provenance,
                                            const bool output_region = false);
      virtual void create_shared_ownership(LogicalRegion handle);
      virtual void destroy_logical_region(LogicalRegion handle,
                                          const bool unordered,
                                          Provenance *provenance);
      virtual void reset_equivalence_sets(LogicalRegion parent, 
                                          LogicalRegion region,
                                          const std::set<FieldID> &fields);
    public:
      virtual FieldAllocatorImpl* create_field_allocator(FieldSpace handle,
                                                         bool unordered);
      virtual void destroy_field_allocator(FieldSpaceNode *node,
                                           bool from_application = true);
    public:
      void initialize_unordered_collective(void);
      void finalize_unordered_collective(AutoLock &d_lock);
      virtual void insert_unordered_ops(AutoLock &d_lock);
      virtual void progress_unordered_operations(bool end_task = false);
      virtual Future execute_task(const TaskLauncher &launcher,
                                  std::vector<OutputRequirement> *outputs);
      virtual FutureMap execute_index_space(const IndexTaskLauncher &launcher,
                                       std::vector<OutputRequirement> *outputs);
      virtual Future execute_index_space(const IndexTaskLauncher &launcher,
                                       ReductionOpID redop, bool deterministic,
                                       std::vector<OutputRequirement> *outputs);
      virtual Future reduce_future_map(const FutureMap &future_map,
                                       ReductionOpID redop, bool deterministic,
                                       MapperID map_id, MappingTagID tag,
                                       Provenance *provenance,
                                       Future initial_value);
      using InnerContext::construct_future_map;
      virtual FutureMap construct_future_map(IndexSpace space,
                                const std::map<DomainPoint,UntypedBuffer> &data,
                                             Provenance *provenance,
                                             bool collective = false,
                                             ShardingID sid = 0,
                                             bool implicit = false,
                                             bool internal = false,
                                             bool check_space = true);
      virtual FutureMap construct_future_map(IndexSpace space,
                    const std::map<DomainPoint,Future> &futures,
                                             Provenance *provenance,
                                             bool internal = false,
                                             bool collective = false,
                                             ShardingID sid = 0,
                                             bool implicit = false,
                                             bool check_space = true);
      virtual PhysicalRegion map_region(const InlineLauncher &launcher);
      virtual ApEvent remap_region(const PhysicalRegion &region,
                                   Provenance *provenance,
                                   bool internal = false);
      // Unmapping region is the same as for an inner context
      virtual void fill_fields(const FillLauncher &launcher);
      virtual void fill_fields(const IndexFillLauncher &launcher);
      virtual void discard_fields(const DiscardLauncher &launcher);
      virtual void issue_copy(const CopyLauncher &launcher);
      virtual void issue_copy(const IndexCopyLauncher &launcher);
      virtual void issue_acquire(const AcquireLauncher &launcher);
      virtual void issue_release(const ReleaseLauncher &launcher);
      virtual PhysicalRegion attach_resource(const AttachLauncher &launcher);
      virtual ExternalResources attach_resources(
                                          const IndexAttachLauncher &launcher);
      virtual RegionTreeNode* compute_index_attach_upper_bound(
                                        const IndexAttachLauncher &launcher,
                                        const std::vector<unsigned> &indexes);
      virtual Future detach_resource(PhysicalRegion region, const bool flush,
                                     const bool unordered,
                                     Provenance *provenance = NULL);
      virtual Future detach_resources(ExternalResources resources,
                                      const bool flush, const bool unordered,
                                      Provenance *provenance);
      virtual FutureMap execute_must_epoch(const MustEpochLauncher &launcher);
      virtual Future issue_timing_measurement(const TimingLauncher &launcher);
      virtual Future select_tunable_value(const TunableLauncher &launcher);
      virtual Future issue_mapping_fence(Provenance *provenance);
      virtual Future issue_execution_fence(Provenance *provenance);
      virtual void begin_trace(TraceID tid, bool logical_only,
          bool static_trace, const std::set<RegionTreeID> *managed, bool dep,
          Provenance *provenance);
      virtual void end_trace(TraceID tid, bool deprecated,
                             Provenance *provenance);
      virtual void end_task(const void *res, size_t res_size, bool owned,
                      PhysicalInstance inst, FutureFunctor *callback_future,
                      const Realm::ExternalInstanceResource *resource,
                      void (*freefunc)(const Realm::ExternalInstanceResource&),
                      const void *metadataptr, size_t metadatasize,
                      ApEvent effects);
      virtual void post_end_task(FutureInstance *instance,
                                 void *metadata, size_t metasize,
                                 FutureFunctor *callback_functor,
                                 bool own_callback_functor);
      virtual bool add_to_dependence_queue(Operation *op, 
                                           bool unordered = false,
                                           bool outermost = true);
      virtual PredicateImpl* create_predicate_impl(Operation *op);
      virtual CollectiveResult* find_or_create_collective_view(
          RegionTreeID tid, const std::vector<DistributedID> &instances, 
          RtEvent &ready);
    public:
      virtual ProjectionSummary* construct_projection_summary(
          Operation *op, unsigned index, const RegionRequirement &req,
          LogicalState *owner, const ProjectionInfo &proj_info);
      virtual bool has_interfering_shards(ProjectionSummary *one,
                                          ProjectionSummary *two);
      virtual bool match_timeouts(std::vector<LogicalUser*> &timeouts,
                                  std::vector<LogicalUser*> &to_delete,
                                  TimeoutMatchExchange *&exchange);
    public:
      virtual Lock create_lock(void);
      virtual void destroy_lock(Lock l);
      virtual Grant acquire_grant(const std::vector<LockRequest> &requests);
      virtual void release_grant(Grant grant);
    public:
      virtual PhaseBarrier create_phase_barrier(unsigned arrivals);
      virtual void destroy_phase_barrier(PhaseBarrier pb);
      virtual PhaseBarrier advance_phase_barrier(PhaseBarrier pb);
    public:
      virtual DynamicCollective create_dynamic_collective(
                                                  unsigned arrivals,
                                                  ReductionOpID redop,
                                                  const void *init_value,
                                                  size_t init_size);
      virtual void destroy_dynamic_collective(DynamicCollective dc);
      virtual void arrive_dynamic_collective(DynamicCollective dc,
                        const void *buffer, size_t size, unsigned count);
      virtual void defer_dynamic_collective_arrival(DynamicCollective dc,
                                                    const Future &future,
                                                    unsigned count);
      virtual Future get_dynamic_collective_result(DynamicCollective dc,
                                                   Provenance *provenance);
      virtual DynamicCollective advance_dynamic_collective(
                                                   DynamicCollective dc);
    public:
#ifdef DEBUG_LEGION_COLLECTIVES
      virtual MergeCloseOp* get_merge_close_op(Operation *op,
                                               RegionTreeNode *node);
      virtual RefinementOp* get_refinement_op(Operation *op,
                                              RegionTreeNode *node);
#else
      virtual MergeCloseOp* get_merge_close_op(void);
      virtual RefinementOp* get_refinement_op(void);
#endif
      virtual VirtualCloseOp* get_virtual_close_op(void);
    public:
      virtual void pack_task_context(Serializer &rez) const;
    public:
      virtual void pack_remote_context(Serializer &rez, 
                                       AddressSpaceID target,
                                       bool replicate = false);
    public:
      void handle_collective_message(Deserializer &derez);
      void register_rendezvous(ShardRendezvous *rendezvous);
      void handle_rendezvous_message(Deserializer &derez);
      void handle_resource_update(Deserializer &derez,
                                  std::set<RtEvent> &applied);
      void handle_trace_update(Deserializer &derez, AddressSpaceID source);
      ApBarrier handle_find_trace_shard_event(size_t temp_index, ApEvent event,
                                              ShardID remote_shard);
      ApBarrier handle_find_trace_shard_frontier(size_t temp_index, 
                                              ApEvent event,
                                              ShardID remote_shard);
      void record_intra_space_dependence(size_t context_index, 
          const DomainPoint &point, RtEvent point_mapped, ShardID next_shard);
      void handle_intra_space_dependence(Deserializer &derez);
    public:
      void increase_pending_index_spaces(unsigned count, bool double_buffer);
      void increase_pending_partitions(unsigned count, bool double_buffer);
      void increase_pending_field_spaces(unsigned count, bool double_buffer);
      void increase_pending_fields(unsigned count, bool double_buffer);
      void increase_pending_region_trees(unsigned count, bool double_buffer);
      void increase_pending_distributed_ids(unsigned count, bool double_buffer);
      DistributedID get_next_distributed_id(void);
      bool create_shard_partition(Operation *op, IndexPartition &pid,
          IndexSpace parent, IndexSpace color_space, Provenance *provenance,
          PartitionKind part_kind, LegionColor partition_color,
          bool color_generated);
    public:
      // Collective methods
      CollectiveID get_next_collective_index(CollectiveIndexLocation loc,
                                             bool logical = false);
      void register_collective(ShardCollective *collective);
      ShardCollective* find_or_buffer_collective(Deserializer &derez);
      void unregister_collective(ShardCollective *collective);
      ShardRendezvous* find_or_buffer_rendezvous(Deserializer &derez);
    public:
      // Physical template methods
      size_t register_trace_template(ShardedPhysicalTemplate *phy_template);
      ShardedPhysicalTemplate* find_or_buffer_trace_update(Deserializer &derez,
                                                         AddressSpaceID source);
      void unregister_trace_template(size_t template_index);
    public:
      // Support for making equivalence sets (logical analysis stage only)
      ShardID get_next_equivalence_set_origin(void);
      virtual RtEvent compute_equivalence_sets(unsigned req_index,
                      const std::vector<EqSetTracker*> &targets,
                      const std::vector<AddressSpaceID> &target_spaces,
                      AddressSpaceID creation_target_space,
                      IndexSpaceExpression *expr, const FieldMask &mask);
      virtual RtEvent record_output_equivalence_set(EqSetTracker *source,
                      AddressSpaceID source_space, unsigned req_index,
                      EquivalenceSet *set, const FieldMask &mask);
      virtual EqKDTree* create_equivalence_set_kd_tree(IndexSpaceNode *node);
      void handle_compute_equivalence_sets(Deserializer &derez);
      void handle_output_equivalence_set(Deserializer &derez);
      void handle_refine_equivalence_sets(Deserializer &derez);
    public:
      // Fence barrier methods
      inline RtBarrier get_next_mapping_fence_barrier(void)
        { return mapping_fence_barrier.next(this); }
      inline ApBarrier get_next_execution_fence_barrier(void)
        { return execution_fence_barrier.next(this); }
      inline RtBarrier get_next_resource_return_barrier(void)
        { return resource_return_barrier.next(this); }
      inline RtBarrier get_next_summary_fence_barrier(void)
        { return summary_fence_barrier.next(this); }
      inline RtBarrier get_next_deletion_ready_barrier(void)
        { return deletion_ready_barrier.next(this); }
      inline RtBarrier get_next_deletion_mapping_barrier(void)
        { return deletion_mapping_barrier.next(this); }
      inline RtBarrier get_next_deletion_execution_barrier(void)
        { return deletion_execution_barrier.next(this); }
      inline ApBarrier get_next_detach_effects_barrier(void)
        { return detach_effects_barrier.next(this); }
      inline ApBarrier get_next_future_map_wait_barrier(void)
        { return future_map_wait_barrier.next(this); }
      inline RtBarrier get_next_dependent_partition_mapping_barrier(void)
        { return dependent_partition_mapping_barrier.next(this); }
      inline ApBarrier get_next_dependent_partition_execution_barrier(void)
        { return dependent_partition_execution_barrier.next(this); }
      inline RtBarrier get_next_attach_resource_barrier(void)
        { return attach_resource_barrier.next(this); }
      inline RtBarrier get_next_concurrent_precondition_barrier(void)
        { return concurrent_precondition_barrier.next(this); }
      inline RtBarrier get_next_concurrent_postcondition_barrier(void)
        { return concurrent_postcondition_barrier.next(this); }
      inline RtBarrier get_next_output_regions_barrier(void)
        { return output_regions_barrier.next(this); }
      inline RtBarrier get_next_close_mapped_barrier(void)
        {
          const RtBarrier result =
            close_mapped_barriers[next_close_mapped_bar_index++].next(this);
          if (next_close_mapped_bar_index == close_mapped_barriers.size())
            next_close_mapped_bar_index = 0;
          return result;
        }
      inline RtBarrier get_next_refinement_mapped_barrier(void)
        {
          const RtBarrier result = refinement_mapped_barriers[
            next_refinement_mapped_bar_index++].next(this);
          if (next_refinement_mapped_bar_index == 
              refinement_mapped_barriers.size())
            next_refinement_mapped_bar_index = 0;
          return result;
        }
      inline RtBarrier get_next_refinement_barrier(void)
        {
          const RtBarrier result = refinement_ready_barriers[
            next_refinement_ready_bar_index++].next(this);
          if (next_refinement_ready_bar_index ==
              refinement_ready_barriers.size())
            next_refinement_ready_bar_index = 0;
          return result;
        }
      // Note this method always returns two barrier generations
      inline RtBarrier get_next_collective_map_barriers(void)
        {
          // Realm phase barriers do not have an even number of maximum
          // phases so we need to handle the case where the names for the
          // two barriers are not the same. If that occurs then we need
          // finish off the old barrier and use the next one
          RtBarrier result = collective_map_barriers[
            next_collective_map_bar_index].next(this);
          RtBarrier next = collective_map_barriers[
            next_collective_map_bar_index].next(this);
          if (result != Runtime::get_previous_phase(next))
          {
            // Finish off the old barrier
            Runtime::phase_barrier_arrive(result, 1);
            result = next;
            next = collective_map_barriers[
              next_collective_map_bar_index].next(this);
#ifdef DEBUG_LEGION
            assert(result == Runtime::get_previous_phase(next));
#endif
          }
          if (++next_collective_map_bar_index == collective_map_barriers.size())
            next_collective_map_bar_index = 0;
          return result;
        }
      // Note this method always returns two barrier generations
      inline ApBarrier get_next_indirection_barriers(void)
        {
          // Realm phase barriers do not have an even number of maximum
          // phases so we need to handle the case where the names for the
          // two barriers are not the same. If that occurs then we need
          // finish off the old barrier and use the next one
          ApBarrier result =
            indirection_barriers[next_indirection_bar_index].next(this);
          ApBarrier next =
            indirection_barriers[next_indirection_bar_index].next(this);
          if (result != Runtime::get_previous_phase(next))
          {
            // Finish off the old barrier
            Runtime::phase_barrier_arrive(result, 1);
            result = next;
            next = indirection_barriers[next_indirection_bar_index].next(this);
#ifdef DEBUG_LEGION
            assert(result == Runtime::get_previous_phase(next));
#endif
          }
          if (++next_indirection_bar_index == indirection_barriers.size())
            next_indirection_bar_index = 0;
          return result;
        }
    protected:
#ifdef DEBUG_LEGION_COLLECTIVES
      // Versions of the methods below but with reduction initialization
      bool create_new_replicate_barrier(RtBarrier &bar, ReductionOpID redop,
          const void *init, size_t init_size, size_t arrivals);
      bool create_new_replicate_barrier(ApBarrier &bar, ReductionOpID redop,
          const void *init, size_t init_size, size_t arrivals);
      // This one can only be called inside the logical dependence analysis
      bool create_new_logical_barrier(RtBarrier &bar, ReductionOpID redop,
          const void *init, size_t init_size, size_t arrivals);
      bool create_new_logical_barrier(ApBarrier &bar, ReductionOpID redop,
          const void *init, size_t init_size, size_t arrivals);
#else
      // These can only be called inside the task for this context
      // since they assume that all the shards are aligned and doing
      // the same calls for the same operations in the same order
      bool create_new_replicate_barrier(RtBarrier &bar, size_t arrivals);
      bool create_new_replicate_barrier(ApBarrier &bar, size_t arrivals);
      // This one can only be called inside the logical dependence analysis
      bool create_new_logical_barrier(RtBarrier &bar, size_t arrivals);
      bool create_new_logical_barrier(ApBarrier &bar, size_t arrivals);
#endif
    public:
      const DomainPoint& get_shard_point(void) const; 
    public:
      static void register_attach_detach_sharding_functor(Runtime *runtime);
      ShardingFunction* get_attach_detach_sharding_function(void);
      IndexSpaceNode* compute_index_attach_launch_spaces(
          std::vector<size_t> &shard_sizes, Provenance *provenance);
      static void register_universal_sharding_functor(Runtime *runtime);
      ShardingFunction* get_universal_sharding_function(void);
    public:
      void hash_future(Murmur3Hasher &hasher, const unsigned safe_level, 
                       const Future &future, const char *description) const;
      static void hash_future_map(Murmur3Hasher &hasher, const FutureMap &map,
                                  const char *description);
      static void hash_index_space_requirements(Murmur3Hasher &hasher,
          const std::vector<IndexSpaceRequirement> &index_requirements);
      static void hash_region_requirements(Murmur3Hasher &hasher,
          const std::vector<RegionRequirement> &region_requirements);
      static void hash_output_requirements(Murmur3Hasher &hasher,
          const std::vector<OutputRequirement> &output_requirements);
      static void hash_grants(Murmur3Hasher &hasher, 
          const std::vector<Grant> &grants);
      static void hash_phase_barriers(Murmur3Hasher &hasher,
          const std::vector<PhaseBarrier> &phase_barriers);
      static void hash_argument(Murmur3Hasher &hasher,const unsigned safe_level,
                             const UntypedBuffer &arg, const char *description);
      static void hash_predicate(Murmur3Hasher &hasher, const Predicate &pred,
                                 const char *description);
      static void hash_static_dependences(Murmur3Hasher &hasher,
          const std::vector<StaticDependence> *dependences);
      void hash_task_launcher(Murmur3Hasher &hasher, 
          const unsigned safe_level, const TaskLauncher &launcher) const;
      void hash_index_launcher(Murmur3Hasher &hasher,
          const unsigned safe_level, const IndexTaskLauncher &launcher);
      void hash_execution_constraints(Murmur3Hasher &hasher,
          const ExecutionConstraintSet &constraints);
      void hash_layout_constraints(Murmur3Hasher &hasher,
          const LayoutConstraintSet &constraints, bool hash_pointers);
    public:
      ShardTask *const owner_shard;
      ShardManager *const shard_manager;
      const size_t total_shards;
    protected: 
      typedef ReplBarrier<RtBarrier,false> RtReplBar;
      typedef ReplBarrier<ApBarrier,false> ApReplBar;
      typedef ReplBarrier<ApBarrier,false,true> ApReplSingleBar;
      typedef ReplBarrier<RtBarrier,false,true> RtReplSingleBar;
      typedef ReplBarrier<RtBarrier,true> RtLogicalBar;
      typedef ReplBarrier<ApBarrier,true> ApLogicalBar;
      // These barriers are used to identify when close operations are mapped
      std::vector<RtLogicalBar>  close_mapped_barriers;
      unsigned                   next_close_mapped_bar_index;
      // These barriers are used to identify when refinement ops are ready
      std::vector<RtLogicalBar>  refinement_ready_barriers;
      unsigned                   next_refinement_ready_bar_index;
      // These barriers are used to identify when refinement ops are mapped
      std::vector<RtLogicalBar>  refinement_mapped_barriers;
      unsigned                   next_refinement_mapped_bar_index; 
      // These barriers are for signaling when indirect copies are done
      std::vector<ApReplBar>     indirection_barriers;
      unsigned                   next_indirection_bar_index;
      // These barriers are used to identify pre and post conditions for
      // exclusive collective mapping operations 
      std::vector<RtLogicalBar>  collective_map_barriers;
      unsigned                   next_collective_map_bar_index;
    protected:
      std::map<std::pair<size_t,DomainPoint>,IntraSpaceDeps> intra_space_deps;
    protected:
      // Store the global owner shard and local owner shard for allocation
      std::map<FieldSpace,
               std::pair<ShardID,bool> > field_allocator_owner_shards;
    protected:
      ShardID distributed_id_allocator_shard;
      ShardID index_space_allocator_shard;
      ShardID index_partition_allocator_shard;
      ShardID field_space_allocator_shard;
      ShardID field_allocator_shard;
      ShardID logical_region_allocator_shard;
      ShardID dynamic_id_allocator_shard;
      ShardID equivalence_set_allocator_shard;
    protected:
      RtReplBar creation_barrier;
      RtLogicalBar deletion_ready_barrier;
      RtLogicalBar deletion_mapping_barrier;
      RtLogicalBar deletion_execution_barrier;
      RtReplBar attach_resource_barrier;
      ApLogicalBar detach_effects_barrier;
      RtLogicalBar mapping_fence_barrier;
      RtReplBar resource_return_barrier;
      RtLogicalBar summary_fence_barrier;
      ApLogicalBar execution_fence_barrier;
      RtReplBar dependent_partition_mapping_barrier;
      ApLogicalBar dependent_partition_execution_barrier;
      RtReplBar semantic_attach_barrier;
      ApReplBar future_map_wait_barrier;
      ApReplBar inorder_barrier;
      RtReplSingleBar concurrent_precondition_barrier;
      RtReplBar concurrent_postcondition_barrier;
      RtReplBar output_regions_barrier;
#ifdef DEBUG_LEGION_COLLECTIVES
    protected:
      RtReplBar collective_check_barrier;
      RtLogicalBar logical_check_barrier;
      RtLogicalBar close_check_barrier;
      RtLogicalBar refinement_check_barrier;
      bool collective_guard_reentrant;
      bool logical_guard_reentrant;
#endif
    protected:
      // local barriers to this context for handling returned
      // resources from sub-tasks
      RtBarrier returned_resource_ready_barrier;
      RtBarrier returned_resource_mapped_barrier;
      RtBarrier returned_resource_execution_barrier;
    protected:
      int shard_collective_radix;
      int shard_collective_log_radix;
      int shard_collective_stages;
      int shard_collective_participating_shards;
      int shard_collective_last_radix;
    protected:
      mutable LocalLock replication_lock;
      CollectiveID next_available_collective_index;
      // We also need to create collectives in the logical dependence
      // analysis stage of the pipeline. We'll have those count on the
      // odd numbers of the collective IDs whereas the ones from the 
      // application task will be the even numbers.
      CollectiveID next_logical_collective_index;
      std::map<CollectiveID,ShardCollective*> collectives;
      std::map<CollectiveID,std::vector<
                std::pair<void*,size_t> > > pending_collective_updates;
    protected:
      std::map<ShardID,ShardRendezvous*> shard_rendezvous;
      std::map<ShardID,std::vector<
                std::pair<void*,size_t> > > pending_rendezvous_updates;
    protected:
      // Pending allocations of various resources
      std::deque<std::pair<ValueBroadcast<ISBroadcast>*,bool> > 
                                            pending_index_spaces;
      std::deque<std::pair<ValueBroadcast<IPBroadcast>*,ShardID> >
                                            pending_index_partitions;
      std::deque<std::pair<ValueBroadcast<FSBroadcast>*,bool> >
                                            pending_field_spaces;
      std::deque<std::pair<ValueBroadcast<FIDBroadcast>*,bool> >
                                            pending_fields;
      std::deque<std::pair<ValueBroadcast<LRBroadcast>*,bool> >
                                            pending_region_trees;
      std::deque<std::pair<ValueBroadcast<DIDBroadcast>*,bool> >
                                            pending_distributed_ids;
      unsigned pending_index_space_check;
      unsigned pending_index_partition_check;
      unsigned pending_field_space_check;
      unsigned pending_field_check;
      unsigned pending_region_tree_check;
      unsigned pending_distributed_id_check;
    protected:
      std::map<size_t,ShardedPhysicalTemplate*> physical_templates;
      struct PendingTemplateUpdate {
      public:
        PendingTemplateUpdate(void)
          : ptr(NULL), size(0), source(0) { }
        PendingTemplateUpdate(void *p, size_t s, AddressSpaceID src)
          : ptr(p), size(s), source(src) { }
      public:
        void *ptr;
        size_t size;
        AddressSpaceID source;
      };
      std::map<size_t/*template index*/,
        std::vector<PendingTemplateUpdate> > pending_template_updates;
      size_t next_physical_template_index;
    protected:
      // Different from pending_top_views as this applies to our requests
      std::map<PhysicalManager*,RtUserEvent> pending_request_views;
      std::map<RegionTreeID,RtUserEvent> pending_tree_requests;
    protected:
      std::map<std::pair<unsigned,unsigned>,RtBarrier> ready_clone_barriers;
      std::map<std::pair<unsigned,unsigned>,RtUserEvent> pending_clone_barriers;
    protected:
      struct AttachLaunchSpace {
      public:
        AttachLaunchSpace(IndexSpaceNode *node) : launch_space(node) { }
      public:
        IndexSpaceNode *const launch_space;
        std::vector<size_t> shard_sizes;
      };
      std::vector<AttachLaunchSpace*> index_attach_launch_spaces;
    protected:
      unsigned next_replicate_bar_index;
      unsigned next_logical_bar_index;
    protected:
      static const unsigned MIN_UNORDERED_OPS_EPOCH = 32;
      static const unsigned MAX_UNORDERED_OPS_EPOCH = 32768;
      unsigned unordered_ops_counter;
      unsigned unordered_ops_epoch;
      UnorderedExchange *unordered_collective;
    };

    /**
     * \class RemoteTask
     * A small helper class for giving application
     * visibility to this remote context
     */
    class RemoteTask : public ExternalTask {
    public:
      RemoteTask(RemoteContext *owner);
      RemoteTask(const RemoteTask &rhs);
      virtual ~RemoteTask(void);
    public:
      RemoteTask& operator=(const RemoteTask &rhs);
    public:
      virtual int get_depth(void) const;
      virtual UniqueID get_unique_id(void) const;
      virtual Domain get_slice_domain(void) const;
      virtual size_t get_context_index(void) const; 
      virtual void set_context_index(size_t index);
      virtual bool has_parent_task(void) const;
      virtual const Task* get_parent_task(void) const;
      virtual const char* get_task_name(void) const;
      virtual ShardID get_shard_id(void) const;
      virtual size_t get_total_shards(void) const;
      virtual DomainPoint get_shard_point(void) const;
      virtual Domain get_shard_domain(void) const;
      virtual bool has_trace(void) const;
      virtual const std::string& get_provenance_string(bool human = true) const;
    public:
      RemoteContext *const owner;
      unsigned context_index;
    };

    /**
     * \class RemoteContext
     * A remote copy of a TaskContext for the 
     * execution of sub-tasks on remote notes.
     */
    class RemoteContext : public InnerContext {
    public:
      RemoteContext(DistributedID did, Runtime *runtime,
                    CollectiveMapping *mapping = NULL);
      RemoteContext(const RemoteContext &rhs) = delete;
      virtual ~RemoteContext(void);
    public:
      RemoteContext& operator=(const RemoteContext &rhs) = delete;
    public:
      virtual Task* get_task(void);
      virtual UniqueID get_unique_id(void) const;
      virtual ShardID get_shard_id(void) const { return shard_id; }
      virtual DistributedID get_replication_id(void) const { return repl_id; }
      void unpack_remote_context(Deserializer &derez);
      virtual InnerContext* find_parent_context(void);
    public:
      virtual InnerContext* find_top_context(InnerContext *previous = NULL);
    public:
      virtual RtEvent compute_equivalence_sets(unsigned req_index,
                      const std::vector<EqSetTracker*> &targets,
                      const std::vector<AddressSpaceID> &target_spaces,
                      AddressSpaceID creation_target_space,
                      IndexSpaceExpression *expr, const FieldMask &mask);
      virtual RtEvent record_output_equivalence_set(EqSetTracker *source,
                      AddressSpaceID source_space, unsigned req_index,
                      EquivalenceSet *set, const FieldMask &mask);
      virtual InnerContext* find_parent_physical_context(unsigned index);
      virtual void pack_inner_context(Serializer &rez) const;
      virtual CollectiveResult* find_or_create_collective_view(
          RegionTreeID tid, const std::vector<DistributedID> &instances, 
          RtEvent &ready);
      virtual void invalidate_region_tree_contexts(const bool is_top_level_task,
                          std::set<RtEvent> &applied,
                          const ShardMapping *shard_mapping = NULL,
                          ShardID source_shard = 0);
      virtual void receive_created_region_contexts(
                          const std::vector<RegionNode*> &created_regions,
                          const std::vector<EqKDTree*> &created_trees,
                          std::set<RtEvent> &applied_events,
                          const ShardMapping *mapping, ShardID source_shard);
      static void handle_created_region_contexts(Runtime *runtime, 
                                                 Deserializer &derez);
    public:
      const Task* get_parent_task(void);
      inline Provenance* get_provenance(void) { return provenance; }
    public:
      void unpack_local_field_update(Deserializer &derez);
      static void handle_local_field_update(Deserializer &derez, 
                                            Runtime *runtime);
    public:
      static void handle_context_request(Deserializer &derez, Runtime *runtime);
      static void handle_context_response(Deserializer &derez,Runtime *runtime);
      static void handle_physical_request(Deserializer &derez,
                      Runtime *runtime, AddressSpaceID source);
      void set_physical_context_result(unsigned index, 
                                       InnerContext *result);
      static void handle_physical_response(Deserializer &derez, 
                                           Runtime *runtime);
      static void handle_find_collective_view_request(Deserializer &derez,
                                  Runtime *runtime, AddressSpaceID source);
      static void handle_find_collective_view_response(Deserializer &derez,
                                                       Runtime *runtime);
    protected:
      DistributedID parent_context_did;
      std::atomic<InnerContext*> parent_ctx;
      ShardManager *shard_manager; // if we're lucky and one is already here
      Provenance *provenance;
    protected:
      bool top_level_context;
      RemoteTask remote_task;
      UniqueID remote_uid;
    protected:
      std::vector<unsigned> local_parent_req_indexes;
      std::vector<bool> local_virtual_mapped;
    protected:
      // Cached physical contexts recorded from the owner
      mutable LocalLock remote_lock;
      std::map<unsigned/*index*/,InnerContext*> physical_contexts;
      std::map<unsigned,RtEvent> pending_physical_contexts;
    protected:
      // For remote replicate contexts
      friend class RemoteTask;
      ShardID shard_id;
      size_t total_shards;
      DomainPoint shard_point;
      Domain shard_domain;
      DistributedID repl_id;
      std::map<ShardingID,ShardingFunction*> sharding_functions;
    };

    /**
     * \class LeafContext
     * A context for the execution of a leaf task
     */
    class LeafContext : public TaskContext,
                        public LegionHeapify<LeafContext> {
    public:
      LeafContext(Runtime *runtime, SingleTask *owner,bool inline_task = false);
      LeafContext(const LeafContext &rhs) = delete;
      virtual ~LeafContext(void);
    public:
      LeafContext& operator=(const LeafContext &rhs) = delete;
    public: // Garbage collection methods
      virtual void notify_local(void) { /* nothing to do */ }
    public:
      // Interface for task contexts
      virtual ContextID get_logical_tree_context(void) const;
      virtual ContextID get_physical_tree_context(void) const;
      virtual void compute_task_tree_coordinates(
                TaskTreeCoordinates &coordinatess) const;
      virtual bool attempt_children_complete(void);
      virtual bool attempt_children_commit(void);
      void inline_child_task(TaskOp *child);
      virtual VariantImpl* select_inline_variant(TaskOp *child,
                const std::vector<PhysicalRegion> &parent_regions,
                std::deque<InstanceSet> &physical_instances);
      virtual bool is_leaf_context(void) const;
      virtual void return_resources(ResourceTracker *target, 
                                    size_t return_index,
                                    std::set<RtEvent> &preconditions);
      virtual void pack_return_resources(Serializer &rez, size_t return_index);
      virtual void log_created_requirements(void);
      virtual void report_leaks_and_duplicates(
          std::set<RtEvent> &preconditions);
    public:
      // Interface to operations performed by a context
      virtual IndexSpace create_index_space(const Domain &bounds,
                                            TypeTag type_tag,
                                            Provenance *provenance);
      virtual IndexSpace create_index_space(const Future &future,
                                            TypeTag type_tag,
                                            Provenance *provenance);
      virtual IndexSpace create_index_space(
                           const std::vector<DomainPoint> &points,
                           Provenance *provenance);
      virtual IndexSpace create_index_space(
                           const std::vector<Domain> &rects,
                           Provenance *provenance);
      virtual IndexSpace create_unbound_index_space(TypeTag type_tag,
                                                    Provenance *provenance);
      virtual IndexSpace union_index_spaces(
                           const std::vector<IndexSpace> &spaces,
                           Provenance *provenance);
      virtual IndexSpace intersect_index_spaces(
                           const std::vector<IndexSpace> &spaces,
                           Provenance *provenance);
      virtual IndexSpace subtract_index_spaces(
                           IndexSpace left, IndexSpace right,
                           Provenance *provenance);
      virtual void create_shared_ownership(IndexSpace handle);
      virtual void destroy_index_space(IndexSpace handle, 
                                       const bool unordered,
                                       const bool recurse,
                                       Provenance *provenance);
      virtual void create_shared_ownership(IndexPartition handle);
      virtual void destroy_index_partition(IndexPartition handle,
                                           const bool unordered, 
                                           const bool recurse,
                                           Provenance *provenance);
      virtual IndexPartition create_equal_partition(
                                            IndexSpace parent,
                                            IndexSpace color_space,
                                            size_t granularity,
                                            Color color,
                                            Provenance *provenance);
      virtual IndexPartition create_partition_by_weights(IndexSpace parent,
                                            const FutureMap &weights,
                                            IndexSpace color_space,
                                            size_t granularity, 
                                            Color color,
                                            Provenance *provenance);
      virtual IndexPartition create_partition_by_union(
                                            IndexSpace parent,
                                            IndexPartition handle1,
                                            IndexPartition handle2,
                                            IndexSpace color_space,
                                            PartitionKind kind,
                                            Color color,
                                            Provenance *provenance);
      virtual IndexPartition create_partition_by_intersection(
                                            IndexSpace parent,
                                            IndexPartition handle1,
                                            IndexPartition handle2,
                                            IndexSpace color_space,
                                            PartitionKind kind,
                                            Color color,
                                            Provenance *provenance);
      virtual IndexPartition create_partition_by_intersection(
                                            IndexSpace parent,
                                            IndexPartition partition,
                                            PartitionKind kind,
                                            Color color,
                                            bool dominates,
                                            Provenance *provenance);
      virtual IndexPartition create_partition_by_difference(
                                            IndexSpace parent,
                                            IndexPartition handle1,
                                            IndexPartition handle2,
                                            IndexSpace color_space,
                                            PartitionKind kind,
                                            Color color,
                                            Provenance *provenance);
      virtual Color create_cross_product_partitions(
                                            IndexPartition handle1,
                                            IndexPartition handle2,
                              std::map<IndexSpace,IndexPartition> &handles,
                                            PartitionKind kind,
                                            Color color,
                                            Provenance *provenance);
      virtual void create_association(      LogicalRegion domain,
                                            LogicalRegion domain_parent,
                                            FieldID domain_fid,
                                            IndexSpace range,
                                            MapperID id, MappingTagID tag,
                                            const UntypedBuffer &marg,
                                            Provenance *prov);
      virtual IndexPartition create_restricted_partition(
                                            IndexSpace parent,
                                            IndexSpace color_space,
                                            const void *transform,
                                            size_t transform_size,
                                            const void *extent,
                                            size_t extent_size,
                                            PartitionKind part_kind,
                                            Color color,
                                            Provenance *provenance);
      virtual IndexPartition create_partition_by_domain(
                                            IndexSpace parent,
                                  const std::map<DomainPoint,Domain> &domains,
                                            IndexSpace color_space,
                                            bool perform_intersections,
                                            PartitionKind part_kind,
                                            Color color,
                                            Provenance *provenance);
      virtual IndexPartition create_partition_by_domain(
                                            IndexSpace parent,
                                            const FutureMap &domains,
                                            IndexSpace color_space,
                                            bool perform_intersections,
                                            PartitionKind part_kind,
                                            Color color,
                                            Provenance *provenance,
                                            bool skip_check = false);
      virtual IndexPartition create_partition_by_field(
                                            LogicalRegion handle,
                                            LogicalRegion parent_priv,
                                            FieldID fid,
                                            IndexSpace color_space,
                                            Color color,
                                            MapperID id, MappingTagID tag,
                                            PartitionKind part_kind,
                                            const UntypedBuffer &marg,
                                            Provenance *prov);
      virtual IndexPartition create_partition_by_image(
                                            IndexSpace handle,
                                            LogicalPartition projection,
                                            LogicalRegion parent,
                                            FieldID fid,
                                            IndexSpace color_space,
                                            PartitionKind part_kind,
                                            Color color,
                                            MapperID id, MappingTagID tag,
                                            const UntypedBuffer &marg,
                                            Provenance *prov);
      virtual IndexPartition create_partition_by_image_range(
                                            IndexSpace handle,
                                            LogicalPartition projection,
                                            LogicalRegion parent,
                                            FieldID fid,
                                            IndexSpace color_space,
                                            PartitionKind part_kind,
                                            Color color,
                                            MapperID id, MappingTagID tag,
                                            const UntypedBuffer &marg,
                                            Provenance *prov);
      virtual IndexPartition create_partition_by_preimage(
                                            IndexPartition projection,
                                            LogicalRegion handle,
                                            LogicalRegion parent,
                                            FieldID fid,
                                            IndexSpace color_space,
                                            PartitionKind part_kind,
                                            Color color,
                                            MapperID id, MappingTagID tag,
                                            const UntypedBuffer &marg,
                                            Provenance *prov);
      virtual IndexPartition create_partition_by_preimage_range(
                                            IndexPartition projection,
                                            LogicalRegion handle,
                                            LogicalRegion parent,
                                            FieldID fid,
                                            IndexSpace color_space,
                                            PartitionKind part_kind,
                                            Color color,
                                            MapperID id, MappingTagID tag,
                                            const UntypedBuffer &marg,
                                            Provenance *prov);
      virtual IndexPartition create_pending_partition(
                                            IndexSpace parent,
                                            IndexSpace color_space,
                                            PartitionKind part_kind,
                                            Color color,
                                            Provenance *provenance,
                                            bool trust = false);
      virtual IndexSpace create_index_space_union(
                                            IndexPartition parent,
                                            const void *realm_color,
                                            size_t color_size,
                                            TypeTag type_tag,
                                const std::vector<IndexSpace> &handles,
                                            Provenance *provenance);
      virtual IndexSpace create_index_space_union(
                                            IndexPartition parent,
                                            const void *realm_color,
                                            size_t color_size,
                                            TypeTag type_tag,
                                            IndexPartition handle,
                                            Provenance *provenance);
      virtual IndexSpace create_index_space_intersection(
                                            IndexPartition parent,
                                            const void *realm_color,
                                            size_t color_size,
                                            TypeTag type_tag,
                                const std::vector<IndexSpace> &handles,
                                            Provenance *provenance);
      virtual IndexSpace create_index_space_intersection(
                                            IndexPartition parent,
                                            const void *realm_color,
                                            size_t color_size,
                                            TypeTag type_tag,
                                            IndexPartition handle,
                                            Provenance *provenance);
      virtual IndexSpace create_index_space_difference(
                                            IndexPartition parent,
                                            const void *realm_color,
                                            size_t color_size,
                                            TypeTag type_tag,
                                            IndexSpace initial,
                                const std::vector<IndexSpace> &handles,
                                            Provenance *provenance);
      virtual FieldSpace create_field_space(Provenance *provenance);
      virtual FieldSpace create_field_space(const std::vector<size_t> &sizes,
                                        std::vector<FieldID> &resulting_fields,
                                        CustomSerdezID serdez_id,
                                        Provenance *provenance);
      virtual FieldSpace create_field_space(const std::vector<Future> &sizes,
                                        std::vector<FieldID> &resulting_fields,
                                        CustomSerdezID serdez_id,
                                        Provenance *provenance);
      virtual void create_shared_ownership(FieldSpace handle);
      virtual void destroy_field_space(FieldSpace handle, const bool unordered,
                                       Provenance *provenance);
      virtual FieldAllocatorImpl* create_field_allocator(FieldSpace handle,
                                                         bool unordered);
      virtual void destroy_field_allocator(FieldSpaceNode *node,
                                           bool from_application = true);
      virtual FieldID allocate_field(FieldSpace space, size_t field_size,
                                     FieldID fid, bool local,
                                     CustomSerdezID serdez_id,
                                     Provenance *provenance);
      virtual FieldID allocate_field(FieldSpace space, const Future &field_size,
                                     FieldID fid, bool local,
                                     CustomSerdezID serdez_id,
                                     Provenance *provenance);
      virtual void allocate_local_field(
                                     FieldSpace space, size_t field_size,
                                     FieldID fid, CustomSerdezID serdez_id,
                                     std::set<RtEvent> &done_events,
                                     Provenance *provenance);
      virtual void free_field(FieldAllocatorImpl *allocator, FieldSpace space, 
                              FieldID fid, const bool unordered,
                              Provenance *provenance);
      virtual void allocate_fields(FieldSpace space,
                                   const std::vector<size_t> &sizes,
                                   std::vector<FieldID> &resuling_fields,
                                   bool local, CustomSerdezID serdez_id,
                                   Provenance *provenance);
      virtual void allocate_fields(FieldSpace space,
                                   const std::vector<Future> &sizes,
                                   std::vector<FieldID> &resuling_fields,
                                   bool local, CustomSerdezID serdez_id,
                                   Provenance *provenance);
      virtual void allocate_local_fields(FieldSpace space,
                                   const std::vector<size_t> &sizes,
                                   const std::vector<FieldID> &resuling_fields,
                                   CustomSerdezID serdez_id,
                                   std::set<RtEvent> &done_events,
                                   Provenance *provenance);
      virtual void free_fields(FieldAllocatorImpl *allocator, FieldSpace space, 
                               const std::set<FieldID> &to_free,
                               const bool unordered,
                               Provenance *provenance);
      virtual LogicalRegion create_logical_region(
                                            IndexSpace index_space,
                                            FieldSpace field_space,
                                            bool task_local,
                                            Provenance *provenance,
                                            const bool output_region = false);
      virtual void create_shared_ownership(LogicalRegion handle);
      virtual void destroy_logical_region(LogicalRegion handle,
                                          const bool unordered,
                                          Provenance *provenance);
      virtual void reset_equivalence_sets(LogicalRegion parent, 
                                          LogicalRegion region,
                                          const std::set<FieldID> &fields);
      virtual void get_local_field_set(const FieldSpace handle,
                                       const std::set<unsigned> &indexes,
                                       std::set<FieldID> &to_set) const;
      virtual void get_local_field_set(const FieldSpace handle,
                                       const std::set<unsigned> &indexes,
                                       std::vector<FieldID> &to_set) const;
    public:
      virtual void add_physical_region(const RegionRequirement &req, 
          bool mapped, MapperID mid, MappingTagID tag, ApUserEvent &unmap_event,
          bool virtual_mapped, const InstanceSet &physical_instances);
      virtual Future execute_task(const TaskLauncher &launcher,
                                  std::vector<OutputRequirement> *outputs);
      virtual FutureMap execute_index_space(const IndexTaskLauncher &launcher,
                                       std::vector<OutputRequirement> *outputs);
      virtual Future execute_index_space(const IndexTaskLauncher &launcher,
                                       ReductionOpID redop, bool deterministic,
                                       std::vector<OutputRequirement> *outputs);
      virtual Future reduce_future_map(const FutureMap &future_map,
                                       ReductionOpID redop, bool deterministic,
                                       MapperID map_id, MappingTagID tag,
                                       Provenance *provenance,
                                       Future initial_value);
      virtual FutureMap construct_future_map(IndexSpace domain,
                               const std::map<DomainPoint,UntypedBuffer> &data,
                                             Provenance *provenance,
                                             bool collective = false,
                                             ShardingID sid = 0,
                                             bool implicit = false,
                                             bool internal = false,
                                             bool check_space = true);
      virtual FutureMap construct_future_map(const Domain &domain,
                                const std::map<DomainPoint,UntypedBuffer> &data,
                                             bool collective = false,
                                             ShardingID sid = 0,
                                             bool implicit = false);
      virtual FutureMap construct_future_map(IndexSpace domain,
                                   const std::map<DomainPoint,Future> &futures,
                                             Provenance *provenance,
                                             bool internal = false,
                                             bool collective = false,
                                             ShardingID sid = 0,
                                             bool implicit = false,
                                             bool check_space = true);
      virtual FutureMap construct_future_map(const Domain &domain,
                    const std::map<DomainPoint,Future> &futures,
                                             bool internal = false,
                                             bool collective = false,
                                             ShardingID sid = 0,
                                             bool implicit = false);
      virtual FutureMap transform_future_map(const FutureMap &fm,
                                             IndexSpace new_domain, 
                      TransformFutureMapImpl::PointTransformFnptr fnptr,
                                             Provenance *provenance);
      virtual FutureMap transform_future_map(const FutureMap &fm,
                                             IndexSpace new_domain,
                                             PointTransformFunctor *functor,
                                             bool own_functor,
                                             Provenance *provenance);
      virtual PhysicalRegion map_region(const InlineLauncher &launcher);
      virtual ApEvent remap_region(const PhysicalRegion &region,
                                   Provenance *provenance,
                                   bool internal = false);
      virtual void unmap_region(PhysicalRegion region);
      virtual void unmap_all_regions(bool external);
      virtual void fill_fields(const FillLauncher &launcher);
      virtual void fill_fields(const IndexFillLauncher &launcher);
      virtual void discard_fields(const DiscardLauncher &launcher);
      virtual void issue_copy(const CopyLauncher &launcher);
      virtual void issue_copy(const IndexCopyLauncher &launcher);
      virtual void issue_acquire(const AcquireLauncher &launcher);
      virtual void issue_release(const ReleaseLauncher &launcher);
      virtual PhysicalRegion attach_resource(const AttachLauncher &launcher);
      virtual ExternalResources attach_resources(
                                          const IndexAttachLauncher &launcher);
      virtual Future detach_resource(PhysicalRegion region, const bool flush,
                                     const bool unordered,
                                     Provenance *provenance = NULL);
      virtual Future detach_resources(ExternalResources resources,
                                      const bool flush, const bool unordered,
                                      Provenance *provenance);
      virtual void progress_unordered_operations(bool end_task = false);
      virtual FutureMap execute_must_epoch(const MustEpochLauncher &launcher);
      virtual Future issue_timing_measurement(const TimingLauncher &launcher);
      virtual Future select_tunable_value(const TunableLauncher &launcher);
      virtual Future issue_mapping_fence(Provenance *provenance);
      virtual Future issue_execution_fence(Provenance *provenance);
      virtual void complete_frame(Provenance *provenance);
      virtual Predicate create_predicate(const Future &f,
                                         Provenance *provenance);
      virtual Predicate predicate_not(const Predicate &p,
                                      Provenance *provenance);
      virtual Predicate create_predicate(const PredicateLauncher &launcher);
      virtual Future get_predicate_future(const Predicate &p,
                                          Provenance *provenance);
    public:
      virtual void begin_trace(TraceID tid, bool logical_only,
          bool static_trace, const std::set<RegionTreeID> *managed, bool dep,
          Provenance *provenance);
      virtual void end_trace(TraceID tid, bool deprecated,
                             Provenance *provenance);
      virtual void record_previous_trace(LogicalTrace *trace);
      virtual void invalidate_trace_cache(LogicalTrace *trace,
                                          Operation *invalidator);
      virtual void record_blocking_call(void);
    public:
      virtual void issue_frame(FrameOp *frame, ApEvent frame_termination);
      virtual void perform_frame_issue(FrameOp *frame, 
                                       ApEvent frame_termination);
      virtual void finish_frame(ApEvent frame_termination);
    public:
      virtual void increment_outstanding(void);
      virtual void decrement_outstanding(void);
      virtual void increment_pending(void);
      virtual void decrement_pending(TaskOp *child);
      virtual void decrement_pending(bool need_deferral);
      virtual void increment_frame(void);
      virtual void decrement_frame(void);
    public:
      virtual InnerContext* find_top_context(InnerContext *previous = NULL);
    public:
      virtual void initialize_region_tree_contexts(
          const std::vector<RegionRequirement> &clone_requirements,
          const LegionVector<VersionInfo> &version_infos,
          const std::vector<ApUserEvent> &unmap_events);
      virtual void invalidate_region_tree_contexts(const bool is_top_level_task,
                                      std::set<RtEvent> &applied,
                                      const ShardMapping *mapping = NULL,
                                      ShardID source_shard = 0);
    public:
      virtual void end_task(const void *res, size_t res_size, bool owned,
                      PhysicalInstance inst, FutureFunctor *callback_functor,
                      const Realm::ExternalInstanceResource *resource,
                      void (*freefunc)(const Realm::ExternalInstanceResource&),
                      const void *metadataptr, size_t metadatasize,
                      ApEvent effects);
      virtual void post_end_task(FutureInstance *instance,
                                 void *metadata, size_t metasize,
                                 FutureFunctor *callback_functor,
                                 bool own_callback_functor);
    public:
      virtual void destroy_lock(Lock l);
      virtual Grant acquire_grant(const std::vector<LockRequest> &requests);
      virtual void release_grant(Grant grant);
    public:
      virtual void destroy_phase_barrier(PhaseBarrier pb);
    public:
      virtual DynamicCollective create_dynamic_collective(
                                                  unsigned arrivals,
                                                  ReductionOpID redop,
                                                  const void *init_value,
                                                  size_t init_size);
      virtual void destroy_dynamic_collective(DynamicCollective dc);
      virtual void arrive_dynamic_collective(DynamicCollective dc,
                        const void *buffer, size_t size, unsigned count);
      virtual void defer_dynamic_collective_arrival(DynamicCollective dc,
                                                    const Future &future,
                                                    unsigned count);
      virtual Future get_dynamic_collective_result(DynamicCollective dc,
                                                   Provenance *provenance);
      virtual DynamicCollective advance_dynamic_collective(
                                                   DynamicCollective dc);
    protected:
      mutable LocalLock                            leaf_lock;
      size_t                                       inlined_tasks;
    public:
      virtual TaskPriority get_current_priority(void) const;
      virtual void set_current_priority(TaskPriority priority);
    };

    //--------------------------------------------------------------------------
    inline void TaskContext::begin_runtime_call(void)
    //--------------------------------------------------------------------------
    {
#ifdef DEBUG_LEGION
      assert(implicit_reference_tracker == NULL);
#endif
      if (overhead_profiler != NULL)
      {
        const long long current = Realm::Clock::current_time_in_nanoseconds();
        const long long diff = current - 
          overhead_profiler->previous_profiling_time;
        overhead_profiler->application_time += diff;
        overhead_profiler->previous_profiling_time = current;
        overhead_profiler->inside_runtime_call = true;
      }
    }

    //--------------------------------------------------------------------------
    inline void TaskContext::end_runtime_call(void)
    //--------------------------------------------------------------------------
    {
      if (implicit_reference_tracker != NULL)
      {
        delete implicit_reference_tracker;
        implicit_reference_tracker = NULL;
      }
      if (overhead_profiler != NULL)
      {
        const long long current = Realm::Clock::current_time_in_nanoseconds();
        const long long diff = current - 
          overhead_profiler->previous_profiling_time;
        overhead_profiler->runtime_time += diff;
        overhead_profiler->previous_profiling_time = current;
        overhead_profiler->inside_runtime_call = false;
      }
    }

    //--------------------------------------------------------------------------
    inline void TaskContext::begin_wait(bool from_application)
    //--------------------------------------------------------------------------
    {
      if (overhead_profiler != NULL)
      {
        const long long current = Realm::Clock::current_time_in_nanoseconds();
        const long long diff = current - 
          overhead_profiler->previous_profiling_time;
        if (overhead_profiler->inside_runtime_call)
          overhead_profiler->runtime_time += diff;
        else
          overhead_profiler->application_time += diff;
        overhead_profiler->previous_profiling_time = current;
      }
      if (implicit_profiler != NULL)
      {
        const long long current = Realm::Clock::current_time_in_nanoseconds();
        implicit_profiler->waits.emplace_back(std::make_pair(current, current));
      }
    }

    //--------------------------------------------------------------------------
    inline void TaskContext::end_wait(bool from_application)
    //--------------------------------------------------------------------------
    {
      if (overhead_profiler != NULL)
      {
        const long long current = Realm::Clock::current_time_in_nanoseconds();
        const long long diff = current - 
          overhead_profiler->previous_profiling_time;
        overhead_profiler->wait_time += diff;
        overhead_profiler->previous_profiling_time = current;
      }
      if (implicit_profiler != NULL)
      {
        const long long current = Realm::Clock::current_time_in_nanoseconds();
        implicit_profiler->waits.back().second = current;
      }
    }

  };
};


#endif // __LEGION_CONTEXT_H__

