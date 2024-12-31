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

#ifndef __MAPPER_MANAGER_H__
#define __MAPPER_MANAGER_H__

#include "legion/legion_types.h"
#include "legion/legion_mapping.h"

namespace Legion {
  namespace Internal {

    class MappingCallInfo {
    public:
      MappingCallInfo(MapperManager *man, MappingCallKind k, 
                      Operation *op, bool prioritize = false);
      ~MappingCallInfo(void);
    public:
      MapperManager*const               manager;
      RtUserEvent                       resume;
      const MappingCallKind             kind;
      Operation*const                   operation;
      std::map<PhysicalManager*,unsigned/*count*/>* acquired_instances;
      unsigned long long                start_time;
      unsigned long long                pause_time;
      bool                              reentrant_disabled;
    };

    /**
     * \class MapperManager
     * This is the base class for a bunch different kinds of mapper
     * managers. Some calls into this manager from the mapper will
     * be handled right away, while other we may need to defer and
     * possibly preempt.  This later class of calls are the ones that
     * are made virtual so that the 
     */
    class MapperManager {
    public:
      struct AcquireStatus {
      public:
        std::set<PhysicalManager*> instances;
        std::vector<bool> results;
      };
      struct DeferMessageArgs : public LgTaskArgs<DeferMessageArgs> {
      public:
        static const LgTaskID TASK_ID = LG_DEFER_MAPPER_MESSAGE_TASK_ID;
      public:
        DeferMessageArgs(MapperManager *man, Processor p, unsigned k,
                         void *m, size_t s, bool b)
          : LgTaskArgs<DeferMessageArgs>(implicit_provenance),
            manager(man), sender(p), kind(k), 
            message(m), size(s), broadcast(b) { }
      public:
        MapperManager *const manager;
        const Processor sender;
        const unsigned kind;
        void *const message;
        const size_t size;
        const bool broadcast;
      };
    public:
      MapperManager(Runtime *runtime, Mapping::Mapper *mapper, 
                    MapperID map_id, Processor p, bool is_default);
      virtual ~MapperManager(void);
    public:
      const char* get_mapper_name(void);
    public: // Task mapper calls
      void invoke_select_task_options(TaskOp *task, Mapper::TaskOptions &output,
                                      bool prioritize);
      void invoke_premap_task(TaskOp *task, Mapper::PremapTaskInput &input,
                              Mapper::PremapTaskOutput &output); 
      void invoke_slice_task(TaskOp *task, Mapper::SliceTaskInput &input,
                               Mapper::SliceTaskOutput &output); 
      void invoke_map_task(TaskOp *task, Mapper::MapTaskInput &input,
                           Mapper::MapTaskOutput &output); 
      void invoke_replicate_task(TaskOp *task, 
                                 Mapper::ReplicateTaskInput &input,
                                 Mapper::ReplicateTaskOutput &output);
      void invoke_select_task_variant(TaskOp *task, 
                                      Mapper::SelectVariantInput &input,
                                      Mapper::SelectVariantOutput &output);
      void invoke_post_map_task(TaskOp *task, Mapper::PostMapInput &input,
                                Mapper::PostMapOutput &output);
      void invoke_select_task_sources(TaskOp *task, 
                                      Mapper::SelectTaskSrcInput &input,
                                      Mapper::SelectTaskSrcOutput &output);
      void invoke_select_task_sources(RemoteTaskOp *task, 
                                      Mapper::SelectTaskSrcInput &input,
                                      Mapper::SelectTaskSrcOutput &output);
      void invoke_task_report_profiling(TaskOp *task, 
                                        Mapper::TaskProfilingInfo &input);
      void invoke_task_select_sharding_functor(TaskOp *task,
                              Mapper::SelectShardingFunctorInput &input,
                              Mapper::SelectShardingFunctorOutput &output);
    public: // Inline mapper calls
      void invoke_map_inline(MapOp *op, Mapper::MapInlineInput &input,
                             Mapper::MapInlineOutput &output); 
      void invoke_select_inline_sources(MapOp *op, 
                                        Mapper::SelectInlineSrcInput &input,
                                        Mapper::SelectInlineSrcOutput &output);
      void invoke_select_inline_sources(RemoteMapOp *op, 
                                        Mapper::SelectInlineSrcInput &input,
                                        Mapper::SelectInlineSrcOutput &output);
      void invoke_inline_report_profiling(MapOp *op, 
                                          Mapper::InlineProfilingInfo &input);
    public: // Copy mapper calls
      void invoke_map_copy(CopyOp *op,
                           Mapper::MapCopyInput &input,
                           Mapper::MapCopyOutput &output);
      void invoke_select_copy_sources(CopyOp *op,
                                      Mapper::SelectCopySrcInput &input,
                                      Mapper::SelectCopySrcOutput &output);
      void invoke_select_copy_sources(RemoteCopyOp *op,
                                      Mapper::SelectCopySrcInput &input,
                                      Mapper::SelectCopySrcOutput &output);
      void invoke_copy_report_profiling(CopyOp *op,
                                        Mapper::CopyProfilingInfo &input);
      void invoke_copy_select_sharding_functor(CopyOp *op,
                              Mapper::SelectShardingFunctorInput &input,
                              Mapper::SelectShardingFunctorOutput &output);
    public: // Close mapper calls
      void invoke_select_close_sources(CloseOp *op,
                                       Mapper::SelectCloseSrcInput &input,
                                       Mapper::SelectCloseSrcOutput &output);
      void invoke_select_close_sources(RemoteCloseOp *op,
                                       Mapper::SelectCloseSrcInput &input,
                                       Mapper::SelectCloseSrcOutput &output);
      void invoke_close_report_profiling(CloseOp *op,
                                         Mapper::CloseProfilingInfo &input);
      void invoke_close_select_sharding_functor(CloseOp *op,
                              Mapper::SelectShardingFunctorInput &input,
                              Mapper::SelectShardingFunctorOutput &output);
    public: // Acquire mapper calls
      void invoke_map_acquire(AcquireOp *op,
                              Mapper::MapAcquireInput &input,
                              Mapper::MapAcquireOutput &output);
      void invoke_acquire_report_profiling(AcquireOp *op,
                                           Mapper::AcquireProfilingInfo &input);
      void invoke_acquire_select_sharding_functor(AcquireOp *op,
                              Mapper::SelectShardingFunctorInput &input,
                              Mapper::SelectShardingFunctorOutput &output);
    public: // Release mapper calls
      void invoke_map_release(ReleaseOp *op,
                              Mapper::MapReleaseInput &input,
                              Mapper::MapReleaseOutput &output);
      void invoke_select_release_sources(ReleaseOp *op,
                                         Mapper::SelectReleaseSrcInput &input,
                                         Mapper::SelectReleaseSrcOutput &output);
      void invoke_select_release_sources(RemoteReleaseOp *op,
                                         Mapper::SelectReleaseSrcInput &input,
                                         Mapper::SelectReleaseSrcOutput &output);
      void invoke_release_report_profiling(ReleaseOp *op,
                                           Mapper::ReleaseProfilingInfo &input);
      void invoke_release_select_sharding_functor(ReleaseOp *op,
                              Mapper::SelectShardingFunctorInput &input,
                              Mapper::SelectShardingFunctorOutput &output);
    public: // Partition mapper calls
      void invoke_select_partition_projection(DependentPartitionOp *op,
                          Mapper::SelectPartitionProjectionInput &input,
                          Mapper::SelectPartitionProjectionOutput &output);
      void invoke_map_partition(DependentPartitionOp *op,
                          Mapper::MapPartitionInput &input,
                          Mapper::MapPartitionOutput &output);
      void invoke_select_partition_sources(DependentPartitionOp *op,
                          Mapper::SelectPartitionSrcInput &input,
                          Mapper::SelectPartitionSrcOutput &output);
      void invoke_select_partition_sources(RemotePartitionOp *op,
                          Mapper::SelectPartitionSrcInput &input,
                          Mapper::SelectPartitionSrcOutput &output);
      void invoke_partition_report_profiling(DependentPartitionOp *op,
                          Mapper::PartitionProfilingInfo &input);
      void invoke_partition_select_sharding_functor(DependentPartitionOp *op,
                              Mapper::SelectShardingFunctorInput &input,
                              Mapper::SelectShardingFunctorOutput &output);
    public: // Fill mapper calls
      void invoke_fill_select_sharding_functor(FillOp *op,
                              Mapper::SelectShardingFunctorInput &input,
                              Mapper::SelectShardingFunctorOutput &output);
    public: // All reduce 
      void invoke_map_future_map_reduction(AllReduceOp *op,
                              Mapper::FutureMapReductionInput &input,
                              Mapper::FutureMapReductionOutput &output);
    public: // Task execution mapper calls
      void invoke_configure_context(TaskOp *task,
                                    Mapper::ContextConfigOutput &output);
      void invoke_select_tunable_value(TaskOp *task,
                                       Mapper::SelectTunableInput &input,
                                       Mapper::SelectTunableOutput &output);
    public: // must epoch and graph mapper calls
      void invoke_must_epoch_select_sharding_functor(MustEpochOp *op,
                              Mapper::SelectShardingFunctorInput &input,
                              Mapper::MustEpochShardingFunctorOutput &output);
      void invoke_map_must_epoch(MustEpochOp *op,
                                 Mapper::MapMustEpochInput &input,
                                 Mapper::MapMustEpochOutput &output);
      void invoke_map_dataflow_graph(Mapper::MapDataflowGraphInput &input,
                                     Mapper::MapDataflowGraphOutput &output);
    public: // memoization mapper calls
      void invoke_memoize_operation(Mappable *mappable,
                                    Mapper::MemoizeInput &input,
                                    Mapper::MemoizeOutput &output);
    public: // scheduling and stealing mapper calls
      void invoke_select_tasks_to_map(Mapper::SelectMappingInput &input,
                                      Mapper::SelectMappingOutput &output);
      void invoke_select_steal_targets(Mapper::SelectStealingInput &input,
                                       Mapper::SelectStealingOutput &output);
      void invoke_permit_steal_request(Mapper::StealRequestInput &input,
                                       Mapper::StealRequestOutput &output);
    public: // handling mapper calls
      void invoke_handle_message(Mapper::MapperMessage *message,
                                 bool check_defer = true);
      void invoke_handle_task_result(Mapper::MapperTaskResult &result);
    public:
      virtual bool is_locked(MappingCallInfo *info) = 0;
      virtual void lock_mapper(MappingCallInfo *info, bool read_only) = 0;
      virtual void unlock_mapper(MappingCallInfo *info) = 0;
    public:
      virtual bool is_reentrant(MappingCallInfo *info) = 0;
      virtual void enable_reentrant(MappingCallInfo *info) = 0;
      virtual void disable_reentrant(MappingCallInfo *info) = 0;
    protected:
      friend class Runtime;
      friend class MappingCallInfo;
      friend class Mapping::AutoLock;
      virtual void begin_mapper_call(MappingCallInfo *info,
                                     bool prioritize = false) = 0;
      virtual void pause_mapper_call(MappingCallInfo *info) = 0;
      virtual void resume_mapper_call(MappingCallInfo *info,
                                      RuntimeCallKind kind) = 0;
      virtual void finish_mapper_call(MappingCallInfo *info) = 0;
    public:
      void update_mappable_tag(MappingCallInfo *info,
                               const Mappable &mappable, MappingTagID tag);
      void update_mappable_data(MappingCallInfo *info, const Mappable &mappable,
                                const void *mapper_data, size_t data_size);
    public:
      void send_message(MappingCallInfo *info, Processor target, 
                        const void *message, size_t message_size, 
                        unsigned message_kind);
      void broadcast(MappingCallInfo *info, const void *message, 
                     size_t message_size, unsigned message_kind, int radix);
    public:
      void pack_physical_instance(MappingCallInfo *info, Serializer &rez,
                                  MappingInstance instance);
      void unpack_physical_instance(MappingCallInfo *info, Deserializer &derez,
                                    MappingInstance &instance);
    public:
      MapperEvent create_mapper_event(MappingCallInfo *ctx);
      bool has_mapper_event_triggered(MappingCallInfo *ctx, MapperEvent event);
      void trigger_mapper_event(MappingCallInfo *ctx, MapperEvent event);
      void wait_on_mapper_event(MappingCallInfo *ctx, MapperEvent event);
    public:
      const ExecutionConstraintSet& 
        find_execution_constraints(MappingCallInfo *ctx, 
            TaskID task_id, VariantID vid);
      const TaskLayoutConstraintSet&
        find_task_layout_constraints(MappingCallInfo *ctx, 
            TaskID task_id, VariantID vid);
      const LayoutConstraintSet&
        find_layout_constraints(MappingCallInfo *ctx, LayoutConstraintID id);
      LayoutConstraintID register_layout(MappingCallInfo *ctx, 
                                const LayoutConstraintSet &constraints,
                                FieldSpace handle);
      void release_layout(MappingCallInfo *ctx, LayoutConstraintID layout_id);
      bool do_constraints_conflict(MappingCallInfo *ctx, 
                    LayoutConstraintID set1, LayoutConstraintID set2,
                    const LayoutConstraint **conflict_constraint);
      bool do_constraints_entail(MappingCallInfo *ctx,
                    LayoutConstraintID source, LayoutConstraintID target,
                    const LayoutConstraint **failed_constraint);
    public:
      void find_valid_variants(MappingCallInfo *ctx, TaskID task_id,
                               std::vector<VariantID> &valid_variants,
                               Processor::Kind kind);
      const char* find_task_variant_name(MappingCallInfo *ctx,
                    TaskID task_id, VariantID vid);
      bool is_leaf_variant(MappingCallInfo *ctx, TaskID task_id, 
                           VariantID variant_id);
      bool is_inner_variant(MappingCallInfo *ctx, TaskID task_id, 
                            VariantID variant_id);
      bool is_idempotent_variant(MappingCallInfo *ctx,
                                 TaskID task_id, VariantID variant_id);
      bool is_replicable_variant(MappingCallInfo *ctx,
                                 TaskID task_id, VariantID variant_id);
    public:
      VariantID register_task_variant(MappingCallInfo *ctx,
                                      const TaskVariantRegistrar &registrar,
				      const CodeDescriptor &codedesc,
				      const void *user_data,
				      size_t user_len,
                                      size_t return_type_size,
                                      bool has_return_type);
    public:
      void filter_variants(MappingCallInfo *ctx, const Task &task,
            const std::vector<std::vector<MappingInstance> > &chosen_instances,
                           std::vector<VariantID> &variants);
      void filter_instances(MappingCallInfo *ctx, const Task &task,
                            VariantID chosen_variant,
                  std::vector<std::vector<MappingInstance> > &chosen_instances,
                  std::vector<std::set<FieldID> > &missing_fields);
      void filter_instances(MappingCallInfo *ctx, const Task &task,
                            unsigned index, VariantID chosen_variant,
                            std::vector<MappingInstance> &instances,
                            std::set<FieldID> &missing_fields);
    public:
      bool create_physical_instance(MappingCallInfo *ctx, Memory target_memory,
                                    const LayoutConstraintSet &constraints, 
                                    const std::vector<LogicalRegion> &regions,
                                    MappingInstance &result, 
                                    bool acquire, GCPriority priority,
                                    bool tight_region_bounds, size_t *footprint,
                                    const LayoutConstraint **unsat);
      bool create_physical_instance(MappingCallInfo *ctx, Memory target_memory,
                                    LayoutConstraintID layout_id,
                                    const std::vector<LogicalRegion> &regions,
                                    MappingInstance &result,
                                    bool acquire, GCPriority priority,
                                    bool tight_region_bounds, size_t *footprint,
                                    const LayoutConstraint **unsat);
      bool find_or_create_physical_instance(
                                    MappingCallInfo *ctx, Memory target_memory,
                                    const LayoutConstraintSet &constraints, 
                                    const std::vector<LogicalRegion> &regions,
                                    MappingInstance &result, bool &created, 
                                    bool acquire, GCPriority priority,
                                    bool tight_region_bounds, size_t *footprint,
                                    const LayoutConstraint **unsat);
      bool find_or_create_physical_instance(
                                    MappingCallInfo *ctx, Memory target_memory,
                                    LayoutConstraintID layout_id,
                                    const std::vector<LogicalRegion> &regions,
                                    MappingInstance &result, bool &created, 
                                    bool acquire, GCPriority priority,
                                    bool tight_region_bounds, size_t *footprint,
                                    const LayoutConstraint **unsat);
      bool find_physical_instance(  MappingCallInfo *ctx, Memory target_memory,
                                    const LayoutConstraintSet &constraints,
                                    const std::vector<LogicalRegion> &regions,
                                    MappingInstance &result, bool acquire,
                                    bool tight_region_bounds);
      bool find_physical_instance(  MappingCallInfo *ctx, Memory target_memory,
                                    LayoutConstraintID layout_id,
                                    const std::vector<LogicalRegion> &regions,
                                    MappingInstance &result, bool acquire,
                                    bool tight_region_bounds);
      void find_physical_instances( MappingCallInfo *ctx, Memory target_memory,
                                    const LayoutConstraintSet &constraints,
                                    const std::vector<LogicalRegion> &regions,
                                    std::vector<MappingInstance> &results, 
                                    bool acquire, bool tight_region_bounds);
      void find_physical_instances( MappingCallInfo *ctx, Memory target_memory,
                                    LayoutConstraintID layout_id,
                                    const std::vector<LogicalRegion> &regions,
                                    std::vector<MappingInstance> &results, 
                                    bool acquire, bool tight_region_bounds);
      void set_garbage_collection_priority(MappingCallInfo *ctx, 
                                    const MappingInstance &instance, 
                                    GCPriority priority);
      bool acquire_instance(        MappingCallInfo *ctx, 
                                    const MappingInstance &instance);
      bool acquire_instances(       MappingCallInfo *ctx,
                                    const std::vector<MappingInstance> &insts);
      bool acquire_and_filter_instances(MappingCallInfo *ctx,
                                    std::vector<MappingInstance> &instances,
                                    const bool filter_acquired_instances);
      bool acquire_instances(       MappingCallInfo *ctx, const std::vector<
                                    std::vector<MappingInstance> > &instances);
      bool acquire_and_filter_instances(MappingCallInfo *ctx, std::vector<
                                    std::vector<MappingInstance> > &instances,
                                    const bool filter_acquired_instances);
      void release_instance(        MappingCallInfo *ctx, 
                                    const MappingInstance &instance);
      void release_instances(       MappingCallInfo *ctx,
                                    const std::vector<MappingInstance> &insts);
      void release_instances(       MappingCallInfo *ctx, const std::vector<
                                    std::vector<MappingInstance> > &instances);
      bool acquire_future(MappingCallInfo *ctx, const Future &f, Memory memory);
    public:
      void record_acquired_instance(MappingCallInfo *info, 
                                    InstanceManager *manager, bool created);
      void release_acquired_instance(MappingCallInfo *info,
                                     InstanceManager *manager);
      void check_region_consistency(MappingCallInfo *info, const char *call,
                                    const std::vector<LogicalRegion> &regions);
      bool perform_acquires(MappingCallInfo *info,
                            const std::vector<MappingInstance> &instances,
                            std::vector<unsigned> *to_erase = NULL,
                            const bool filter_acquired_instances = false);
    public:
      IndexSpace create_index_space(MappingCallInfo *info, const Domain &domain,
                                    TypeTag type_tag, const char *provenance);
      IndexSpace union_index_spaces(MappingCallInfo *info, 
                                    const std::vector<IndexSpace> &sources,
                                    const char *provenance);
      IndexSpace intersect_index_spaces(MappingCallInfo *info,
                                        const std::vector<IndexSpace> &sources,
                                        const char *provenance);
      IndexSpace subtract_index_spaces(MappingCallInfo *info, IndexSpace left,
                                       IndexSpace right,const char *provenance);
      bool is_index_space_empty(MappingCallInfo *info, IndexSpace handle);
      bool index_spaces_overlap(MappingCallInfo *info, 
                                IndexSpace one, IndexSpace two);
      bool index_space_dominates(MappingCallInfo *info,
                                 IndexSpace left, IndexSpace right);
      bool has_index_partition(MappingCallInfo *info,
                               IndexSpace parent, Color color);
      IndexPartition get_index_partition(MappingCallInfo *info,
                                         IndexSpace parent, Color color);
      IndexSpace get_index_subspace(MappingCallInfo *info,
                                    IndexPartition p, Color c);
      IndexSpace get_index_subspace(MappingCallInfo *info,
                                    IndexPartition p, 
                                    const DomainPoint &color);
      bool has_multiple_domains(MappingCallInfo *info, IndexSpace handle);
      Domain get_index_space_domain(MappingCallInfo *info, IndexSpace handle);
      void get_index_space_domains(MappingCallInfo *info, IndexSpace handle,
                                   std::vector<Domain> &domains);
      Domain get_index_partition_color_space(MappingCallInfo *info,
                                             IndexPartition p);
      IndexSpace get_index_partition_color_space_name(MappingCallInfo *info,
                                                      IndexPartition p);
      void get_index_space_partition_colors(MappingCallInfo *info, 
                                            IndexSpace sp, 
                                            std::set<Color> &colors);
      bool is_index_partition_disjoint(MappingCallInfo *info,
                                       IndexPartition p);
      bool is_index_partition_complete(MappingCallInfo *info,
                                       IndexPartition p);
      Color get_index_space_color(MappingCallInfo *info, IndexSpace handle);
      DomainPoint get_index_space_color_point(MappingCallInfo *info,
                                              IndexSpace handle);
      Color get_index_partition_color(MappingCallInfo *info, 
                                      IndexPartition handle);
      IndexSpace get_parent_index_space(MappingCallInfo *info,
                                        IndexPartition handle);
      bool has_parent_index_partition(MappingCallInfo *info,
                                      IndexSpace handle);
      IndexPartition get_parent_index_partition(MappingCallInfo *info,
                                                IndexSpace handle);
      unsigned get_index_space_depth(MappingCallInfo *info, IndexSpace handle);
      unsigned get_index_partition_depth(MappingCallInfo *info, 
                                         IndexPartition handle);
    public:
      size_t get_field_size(MappingCallInfo *info, 
                            FieldSpace handle, FieldID fid);
      void get_field_space_fields(MappingCallInfo *info, FieldSpace handle, 
                                  std::vector<FieldID> &fields);
    public:
      LogicalPartition get_logical_partition(MappingCallInfo *info,
                                             LogicalRegion parent, 
                                             IndexPartition handle);
      LogicalPartition get_logical_partition_by_color(MappingCallInfo *info,
                                                      LogicalRegion parent, 
                                                      Color color);
      LogicalPartition get_logical_partition_by_color(MappingCallInfo *info,
                                                      LogicalRegion parent,
                                                      const DomainPoint &color);
      LogicalPartition get_logical_partition_by_tree(MappingCallInfo *info,
                                                     IndexPartition handle, 
                                           FieldSpace fspace, RegionTreeID tid);
      LogicalRegion get_logical_subregion(MappingCallInfo *info,
                                          LogicalPartition parent, 
                                          IndexSpace handle);
      LogicalRegion get_logical_subregion_by_color(MappingCallInfo *info,
                                                   LogicalPartition parent, 
                                                   Color color);
      LogicalRegion get_logical_subregion_by_color(MappingCallInfo *info,
                                                   LogicalPartition parent,
                                                   const DomainPoint &color);
      LogicalRegion get_logical_subregion_by_tree(MappingCallInfo *info,
                                                  IndexSpace handle, 
                                          FieldSpace fspace, RegionTreeID tid);
      Color get_logical_region_color(MappingCallInfo *info, 
                                     LogicalRegion handle);
      DomainPoint get_logical_region_color_point(MappingCallInfo *info, 
                                                 LogicalRegion handle);
      Color get_logical_partition_color(MappingCallInfo *info,
                                        LogicalPartition handle);
      LogicalRegion get_parent_logical_region(MappingCallInfo *info,
                                              LogicalPartition handle);
      bool has_parent_logical_partition(MappingCallInfo *info, 
                                        LogicalRegion handle);
      LogicalPartition get_parent_logical_partition(MappingCallInfo *info,
                                                    LogicalRegion handle);
    public:
      bool retrieve_semantic_information(MappingCallInfo *ctx, TaskID task_id,
          SemanticTag tag, const void *&result, size_t &size, 
          bool can_fail, bool wait_until_ready);
      bool retrieve_semantic_information(MappingCallInfo *ctx,IndexSpace handle,
          SemanticTag tag, const void *&result, size_t &size,
          bool can_fail, bool wait_until_ready);
      bool retrieve_semantic_information(MappingCallInfo *ctx, 
          IndexPartition handle, SemanticTag tag, const void *&result,
          size_t &size, bool can_fail, bool wait_until_ready);
      bool retrieve_semantic_information(MappingCallInfo *ctx,FieldSpace handle,
          SemanticTag tag, const void *&result, size_t &size, 
          bool can_fail, bool wait_until_ready);
      bool retrieve_semantic_information(MappingCallInfo *ctx,FieldSpace handle,
          FieldID fid, SemanticTag tag, const void *&result, size_t &size,
          bool can_fail, bool wait_until_ready);
      bool retrieve_semantic_information(MappingCallInfo *ctx, 
          LogicalRegion handle, SemanticTag tag, const void *&result, 
          size_t &size, bool can_fail, bool wait_until_ready);
      bool retrieve_semantic_information(MappingCallInfo *ctx,
          LogicalPartition handle, SemanticTag tag, const void *&result,
          size_t &size, bool can_fail, bool wait_until_ready);
    public:
      void retrieve_name(MappingCallInfo *ctx, TaskID task_id, 
                         const char *&result);
      void retrieve_name(MappingCallInfo *ctx, IndexSpace handle,
                         const char *&result);
      void retrieve_name(MappingCallInfo *ctx, IndexPartition handle,
                         const char *&result);
      void retrieve_name(MappingCallInfo *ctx, FieldSpace handle,
                         const char *&result);
      void retrieve_name(MappingCallInfo *ctx, FieldSpace handle, 
                         FieldID fid, const char *&result);
      void retrieve_name(MappingCallInfo *ctx, LogicalRegion handle,
                         const char *&result);
      void retrieve_name(MappingCallInfo *ctx, LogicalPartition handle,
                         const char *&result);
    public:
      bool is_MPI_interop_configured(void);
      const std::map<int,AddressSpace>& find_forward_MPI_mapping(
                         MappingCallInfo *ctx);
      const std::map<AddressSpace,int>& find_reverse_MPI_mapping(
                         MappingCallInfo *ctx);
      int find_local_MPI_rank(void);
    public:
      static const char* get_mapper_call_name(MappingCallKind kind);
    public:
      void defer_message(Mapper::MapperMessage *message);
      static void handle_deferred_message(const void *args);
    public:
      // For stealing
      void process_advertisement(Processor advertiser); 
      void perform_stealing(std::multimap<Processor,MapperID> &targets);
    public:
      // For advertising
      void process_failed_steal(Processor thief);
      void perform_advertisements(std::set<Processor> &failed_waiters);
    public:
      Runtime *const runtime;
      Mapping::Mapper *const mapper;
      const MapperID mapper_id;
      const Processor processor;
      const bool profile_mapper;
      const bool request_valid_instances;
      const bool is_default_mapper;
    protected:
      mutable LocalLock mapper_lock;
    protected: // Steal request information
      // Mappers on other processors that we've tried to steal from and failed
      std::set<Processor> steal_blacklist;
      // Mappers that have tried to steal from us and which we
      // should advertise work when we have it
      std::set<Processor> failed_thiefs;
    };

    /**
     * \class SerializingManager
     * In this class at most one mapper call can be running at 
     * a time. Mapper calls that invoke expensive runtime operations
     * can be pre-empted and it is up to the mapper to control
     * whether additional mapper calls when the call is blocked.
     */
    class SerializingManager : public MapperManager {
    public:
      SerializingManager(Runtime *runtime, Mapping::Mapper *mapper,
         MapperID map_id, Processor p, bool reentrant, bool is_default = false);
      SerializingManager(const SerializingManager &rhs);
      virtual ~SerializingManager(void);
    public:
      SerializingManager& operator=(const SerializingManager &rhs);
    public:
      virtual bool is_locked(MappingCallInfo *info);
      virtual void lock_mapper(MappingCallInfo *info, bool read_only);
      virtual void unlock_mapper(MappingCallInfo *info);
    public:
      virtual bool is_reentrant(MappingCallInfo *info);
      virtual void enable_reentrant(MappingCallInfo *info);
      virtual void disable_reentrant(MappingCallInfo *info);
    protected:
      virtual void begin_mapper_call(MappingCallInfo *info,
                                     bool prioritize = false);
      virtual void pause_mapper_call(MappingCallInfo *info);
      virtual void resume_mapper_call(MappingCallInfo *info,
                                      RuntimeCallKind kind);
      virtual void finish_mapper_call(MappingCallInfo *info);
    protected:
      // Must be called while holding the mapper reservation
      RtUserEvent complete_pending_pause_mapper_call(void);
      RtUserEvent complete_pending_finish_mapper_call(void);
    protected:
      // The one executing call if any otherwise NULL
      MappingCallInfo *executing_call; 
      // Calls yet to start running
      std::deque<MappingCallInfo*> pending_calls; 
      // Calls that are ready to resume after runtime work
      std::deque<MappingCallInfo*> ready_calls;
      // Number of calls paused due to runtime work
      unsigned paused_calls;
      // Whether this mapper supports reentrant mapper calls
      const bool allow_reentrant;
      // Whether or not we are currently supporting reentrant calls
      bool permit_reentrant;
      // A flag checking whether we have a pending paused mapper call
      std::atomic<bool> pending_pause_call;
      // A flag checking whether we have a pending finished call
      std::atomic<bool> pending_finish_call;
    };

    /**
     * \class ConcurrentManager
     * In this class many mapper calls can be running concurrently.
     * It is upper to the mapper to lock itself when necessary to 
     * protect internal state. Mappers can be locked in exclusive
     * or non-exclusive modes.
     */
    class ConcurrentManager : public MapperManager {
    public:
      enum LockState {
        UNLOCKED_STATE,
        READ_ONLY_STATE,
        EXCLUSIVE_STATE,
      };
    public:
      ConcurrentManager(Runtime *runtime, Mapping::Mapper *mapper,
                        MapperID map_id, Processor p, bool is_default = false);
      ConcurrentManager(const ConcurrentManager &rhs);
      virtual ~ConcurrentManager(void);
    public:
      ConcurrentManager& operator=(const ConcurrentManager &rhs);
    public:
      virtual bool is_locked(MappingCallInfo *info);
      virtual void lock_mapper(MappingCallInfo *info, bool read_only);
      virtual void unlock_mapper(MappingCallInfo *info);
    public:
      virtual bool is_reentrant(MappingCallInfo *info);
      virtual void enable_reentrant(MappingCallInfo *info);
      virtual void disable_reentrant(MappingCallInfo *info);
    protected:
      virtual void begin_mapper_call(MappingCallInfo *info,
                                     bool prioritize = false);
      virtual void pause_mapper_call(MappingCallInfo *info);
      virtual void resume_mapper_call(MappingCallInfo *info,
                                      RuntimeCallKind kind);
      virtual void finish_mapper_call(MappingCallInfo *info);
    protected:
      // Must be called while holding the lock
      void release_lock(std::vector<RtUserEvent> &to_trigger); 
    protected:
      LockState lock_state;
      std::set<MappingCallInfo*> current_holders;
      std::deque<MappingCallInfo*> read_only_waiters;
      std::deque<MappingCallInfo*> exclusive_waiters;
    };

  };
};

#endif // __MAPPER_MANAGER_H__
