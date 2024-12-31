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
#include "realm/cmdline.h"
#include "legion/legion_ops.h"
#include "legion/legion_tasks.h"
#include "legion/legion_context.h"
#include "legion/legion_profiling.h"
#include "legion/legion_profiling_serializer.h"

#include <string.h>
#include <stdlib.h>

namespace Legion {
  namespace Internal {

    extern Realm::Logger log_prof;

    // Keep a thread-local profiler instance so we can always
    // be thread safe no matter what Realm decides to do 
    thread_local LegionProfInstance *thread_local_profiling_instance = NULL;

    //--------------------------------------------------------------------------
    template<size_t ENTRIES>
    SmallNameClosure<ENTRIES>::SmallNameClosure(void)
    //--------------------------------------------------------------------------
    {
      for (unsigned idx = 0; idx < ENTRIES; idx++)
        instances[idx] = PhysicalInstance::NO_INST;
    }

    //--------------------------------------------------------------------------
    template<size_t ENTRIES>
    void SmallNameClosure<ENTRIES>::record_instance_name(
                                        PhysicalInstance instance, LgEvent name)
    //--------------------------------------------------------------------------
    {
      for (unsigned idx = 0; idx < ENTRIES; idx++)
      {
        if (!instances[idx].exists())
        {
          instances[idx] = instance;
          names[idx] = name;
          return;
        }
        if (instances[idx] == instance)
        {
#ifdef DEBUG_LEGION
          assert(names[idx] == name);
#endif
          return;
        }
      }
      // Should not run out of space
      assert(false);
    }

    //--------------------------------------------------------------------------
    template<size_t ENTRIES>
    LgEvent SmallNameClosure<ENTRIES>::find_instance_name(
                                                    PhysicalInstance inst) const
    //--------------------------------------------------------------------------
    {
      for (unsigned idx = 0; idx < ENTRIES; idx++)
        if (instances[idx] == inst)
          return names[idx];
      // Should always find it before this
      assert(false);
      return names[0];
    }

    // Explicit instantiations for 1 and 2
    template class SmallNameClosure<1>;
    template class SmallNameClosure<2>;

    //--------------------------------------------------------------------------
    LegionProfMarker::LegionProfMarker(const char* _name)
      : name(_name), stopped(false)
    //--------------------------------------------------------------------------
    {
      proc = Realm::Processor::get_executing_processor();
      start = Realm::Clock::current_time_in_nanoseconds();
    }

    //--------------------------------------------------------------------------
    LegionProfMarker::~LegionProfMarker()
    //--------------------------------------------------------------------------
    {
      if (!stopped) mark_stop();
      log_prof.print("Prof User Info " IDFMT " %llu %llu %s", proc.id,
		     start, stop, name);
    }

    //--------------------------------------------------------------------------
    void LegionProfMarker::mark_stop()
    //--------------------------------------------------------------------------
    {
      stop = Realm::Clock::current_time_in_nanoseconds();
      stopped = true;
    }

    //--------------------------------------------------------------------------
    LegionProfInstance::LegionProfInstance(LegionProfiler *own)
      : owner(own)
    //--------------------------------------------------------------------------
    {
    }

    //--------------------------------------------------------------------------
    LegionProfInstance::LegionProfInstance(const LegionProfInstance &rhs)
      : owner(rhs.owner)
    //--------------------------------------------------------------------------
    {
      // should never be called
      assert(false);
    }

    //--------------------------------------------------------------------------
    LegionProfInstance::~LegionProfInstance(void)
    //--------------------------------------------------------------------------
    {
    }

    //--------------------------------------------------------------------------
    LegionProfInstance& LegionProfInstance::operator=(
                                                  const LegionProfInstance &rhs)
    //--------------------------------------------------------------------------
    {
      // should never be called
      assert(false);
      return *this;
    }

    //--------------------------------------------------------------------------
    void LegionProfInstance::register_task_kind(TaskID task_id,
                                                const char *name,bool overwrite)
    //--------------------------------------------------------------------------
    {
      task_kinds.emplace_back(TaskKind());
      TaskKind &kind = task_kinds.back();
      kind.task_id = task_id;
      kind.name = strdup(name);
      kind.overwrite = overwrite;
      const size_t diff = sizeof(TaskKind) + strlen(name);
      owner->update_footprint(diff, this);
    }

    //--------------------------------------------------------------------------
    void LegionProfInstance::register_task_variant(TaskID task_id,
                                                   VariantID variant_id,
                                                   const char *variant_name)
    //--------------------------------------------------------------------------
    {
      task_variants.emplace_back(TaskVariant()); 
      TaskVariant &var = task_variants.back();
      var.task_id = task_id;
      var.variant_id = variant_id;
      var.name = strdup(variant_name);
      const size_t diff = sizeof(TaskVariant) + strlen(variant_name);
      owner->update_footprint(diff, this);
    }

    //--------------------------------------------------------------------------
    void LegionProfInstance::register_operation(Operation *op)
    //--------------------------------------------------------------------------
    {
      operation_instances.emplace_back(OperationInstance());
      OperationInstance &inst = operation_instances.back();
      inst.op_id = op->get_unique_op_id();
      InnerContext *parent_ctx = op->get_context();
      // Legion prof uses ULLONG_MAX to represent the unique IDs of the root
      inst.parent_id = 
       (parent_ctx->get_depth() < 0) ? ULLONG_MAX : parent_ctx->get_unique_id();
      inst.kind = op->get_operation_kind();
      Provenance *prov = op->get_provenance();
      if (prov != NULL)
      {
        inst.provenance = prov->clone();
        owner->update_footprint(
            sizeof(OperationInstance) + strlen(inst.provenance), this);
      }
      else
      {
        inst.provenance = NULL;
        owner->update_footprint(sizeof(OperationInstance), this);
      }
    }

    //--------------------------------------------------------------------------
    void LegionProfInstance::register_multi_task(Operation *op, TaskID task_id)
    //--------------------------------------------------------------------------
    {
      multi_tasks.emplace_back(MultiTask());
      MultiTask &task = multi_tasks.back();
      task.op_id = op->get_unique_op_id();
      task.task_id = task_id;
      owner->update_footprint(sizeof(MultiTask), this);
    }

    //--------------------------------------------------------------------------
    void LegionProfInstance::register_slice_owner(UniqueID pid, UniqueID id)
    //--------------------------------------------------------------------------
    {
      slice_owners.emplace_back(SliceOwner());
      SliceOwner &task = slice_owners.back();
      task.parent_id = pid;
      task.op_id = id;
      owner->update_footprint(sizeof(SliceOwner), this);
    }

    //--------------------------------------------------------------------------
    void LegionProfInstance::register_index_space_rect(IndexSpaceRectDesc
							&_ispace_rect_desc)
    //--------------------------------------------------------------------------
    {
      ispace_rect_desc.emplace_back(IndexSpaceRectDesc());
      IndexSpaceRectDesc &desc = ispace_rect_desc.back();
      desc = _ispace_rect_desc;
      owner->update_footprint(sizeof(IndexSpaceRectDesc), this);
    }

    //--------------------------------------------------------------------------
    void LegionProfInstance::register_index_space_point(IndexSpacePointDesc
							&_ispace_point_desc)
    //--------------------------------------------------------------------------
    {
      ispace_point_desc.emplace_back(IndexSpacePointDesc());
      IndexSpacePointDesc &desc = ispace_point_desc.back();
      desc = _ispace_point_desc;
      owner->update_footprint(sizeof(IndexSpacePointDesc), this);
    }

    //--------------------------------------------------------------------------
    void LegionProfInstance::register_empty_index_space(IDType handle)
    //--------------------------------------------------------------------------
    {
      ispace_empty_desc.emplace_back(IndexSpaceEmptyDesc());
      IndexSpaceEmptyDesc &desc = ispace_empty_desc.back();
      desc.unique_id = handle;
      owner->update_footprint(sizeof(IndexSpaceEmptyDesc), this);
    }

    //--------------------------------------------------------------------------
    void LegionProfInstance::register_field(UniqueID unique_id,
					    unsigned field_id,
					    size_t size,
					    const char* name)
    //--------------------------------------------------------------------------
    {
      field_desc.emplace_back(FieldDesc());
      FieldDesc &desc = field_desc.back();
      desc.unique_id = unique_id;
      desc.field_id = field_id;
      desc.size = (long long)size;
      desc.name = strdup(name);
      const size_t diff = sizeof(FieldDesc) + strlen(name);
      owner->update_footprint(diff, this);
    }
    //--------------------------------------------------------------------------
    void LegionProfInstance::register_field_space(UniqueID unique_id,
						  const char* name)
    //--------------------------------------------------------------------------
    {
      field_space_desc.emplace_back(FieldSpaceDesc());
      FieldSpaceDesc &desc = field_space_desc.back();
      desc.unique_id = unique_id;
      desc.name = strdup(name);
      const size_t diff = sizeof(FieldSpaceDesc) + strlen(name);
      owner->update_footprint(diff, this);
    }
    //--------------------------------------------------------------------------
    void LegionProfInstance::register_index_part(UniqueID unique_id,
						  const char* name)
    //--------------------------------------------------------------------------
    {
      index_part_desc.emplace_back(IndexPartDesc());
      IndexPartDesc &desc = index_part_desc.back();
      desc.unique_id = unique_id;
      desc.name = strdup(name);
      const size_t diff = sizeof(IndexPartDesc) + strlen(name);
      owner->update_footprint(diff, this);
    }

    //--------------------------------------------------------------------------
    void LegionProfInstance::register_index_space(UniqueID unique_id,
						  const char* name)
    //--------------------------------------------------------------------------
    {
      index_space_desc.emplace_back(IndexSpaceDesc());
      IndexSpaceDesc &desc = index_space_desc.back();
      desc.unique_id = unique_id;
      desc.name = strdup(name);
      const size_t diff = sizeof(IndexSpaceDesc) + strlen(name);
      owner->update_footprint(diff, this);
    }

    //--------------------------------------------------------------------------
    void LegionProfInstance::register_index_subspace(IDType parent_id,
						     IDType unique_id,
						     const DomainPoint &point)
    //--------------------------------------------------------------------------
    {
      index_subspace_desc.emplace_back(IndexSubSpaceDesc());
      IndexSubSpaceDesc &desc = index_subspace_desc.back();
      desc.parent_id = parent_id;
      desc.unique_id = unique_id;
      owner->update_footprint(sizeof(IndexSubSpaceDesc), this);
    }

    //--------------------------------------------------------------------------
    void LegionProfInstance::register_index_partition(IDType parent_id,
						      IDType unique_id,
						      bool disjoint,
						      LegionColor point)
    //--------------------------------------------------------------------------
    {
      index_partition_desc.emplace_back(IndexPartitionDesc());
      IndexPartitionDesc &desc = index_partition_desc.back();
      desc.parent_id = parent_id;
      desc.unique_id = unique_id;
      desc.disjoint = disjoint;
      desc.point = point;
      owner->update_footprint(sizeof(IndexPartitionDesc), this);
    }

    //--------------------------------------------------------------------------
    void LegionProfInstance::register_logical_region(IDType index_space,
						     unsigned field_space,
						     unsigned tree_id,
						     const char* name)
    //--------------------------------------------------------------------------
    {
      lr_desc.emplace_back(LogicalRegionDesc());
      LogicalRegionDesc &desc = lr_desc.back();
      desc.ispace_id = index_space;
      desc.fspace_id = field_space;
      desc.tree_id = tree_id;
      desc.name = strdup(name);
      const size_t diff = sizeof(LogicalRegionDesc) + strlen(name);
      owner->update_footprint(diff, this);
    }

    //--------------------------------------------------------------------------
    void LegionProfInstance::register_physical_instance_field(LgEvent inst_uid,
						              unsigned field_id,
						              unsigned field_sp,
                                                              unsigned align,
                                                              bool align_set,
                                                              EqualityKind eqk)
    //--------------------------------------------------------------------------
    {
      phy_inst_layout_rdesc.emplace_back(PhysicalInstLayoutDesc());
      PhysicalInstLayoutDesc &pdesc = phy_inst_layout_rdesc.back();
      pdesc.inst_uid = inst_uid;
      pdesc.field_id = field_id;
      pdesc.fspace_id = field_sp;
      pdesc.eqk = eqk;
      pdesc.alignment = align;
      pdesc.has_align = align_set;
      owner->update_footprint(sizeof(PhysicalInstLayoutDesc), this);
    }

    //--------------------------------------------------------------------------
    void LegionProfInstance::register_physical_instance_region(LgEvent inst_uid,
							       LogicalRegion
							       handle)
    //--------------------------------------------------------------------------
    {
      phy_inst_rdesc.emplace_back(PhysicalInstRegionDesc());
      PhysicalInstRegionDesc &phy_instance_rdesc = phy_inst_rdesc.back();
      phy_instance_rdesc.inst_uid = inst_uid;
      phy_instance_rdesc.ispace_id = handle.get_index_space().get_id();
      phy_instance_rdesc.fspace_id = handle.get_field_space().get_id();
      phy_instance_rdesc.tree_id = handle.get_tree_id();
      owner->update_footprint(sizeof(PhysicalInstRegionDesc), this);
    }

    //--------------------------------------------------------------------------
    void LegionProfInstance::register_physical_instance_dim_order(
                                                               LgEvent inst_uid,
                                                               unsigned dim,
                                                               DimensionKind k)
    //--------------------------------------------------------------------------
    {
      phy_inst_dim_order_rdesc.emplace_back(PhysicalInstDimOrderDesc());
      PhysicalInstDimOrderDesc &phy_instance_d_rdesc =
        phy_inst_dim_order_rdesc.back();
      phy_instance_d_rdesc.inst_uid = inst_uid;
      phy_instance_d_rdesc.dim = dim;
      phy_instance_d_rdesc.k = k;
      owner->update_footprint(sizeof(PhysicalInstDimOrderDesc), this);
    }

    //--------------------------------------------------------------------------
    void LegionProfInstance::register_physical_instance_use(LgEvent inst_uid,
             UniqueID op_id, unsigned index, const std::vector<FieldID> &fields)
    //--------------------------------------------------------------------------
    {
      const unsigned offset = phy_inst_usage.size();
      phy_inst_usage.resize(offset + fields.size());
      for (unsigned idx = 0; idx < fields.size(); idx++)
      {
        PhysicalInstanceUsage &usage = phy_inst_usage[offset+idx];
        usage.inst_uid = inst_uid;
        usage.op_id = op_id;
        usage.index = index;
        usage.field = fields[idx];
      }
      owner->update_footprint(fields.size()*sizeof(PhysicalInstanceUsage),this);
    }

    //--------------------------------------------------------------------------
    void LegionProfInstance::register_index_space_size(
                                                       UniqueID id,
                                                       unsigned long long
                                                       dense_size,
                                                       unsigned long long
                                                       sparse_size,
                                                       bool is_sparse)
    //--------------------------------------------------------------------------
    {
      index_space_size_desc.emplace_back(IndexSpaceSizeDesc());
      IndexSpaceSizeDesc &size_info = index_space_size_desc.back();
      size_info.id = id;
      size_info.dense_size = dense_size;
      size_info.sparse_size = sparse_size;
      size_info.is_sparse = is_sparse;
      owner->update_footprint(sizeof(IndexSpaceSizeDesc), this);
    }

    //--------------------------------------------------------------------------
    void LegionProfInstance::process_task(const ProfilingInfo *prof_info,
             const Realm::ProfilingResponse &response,
             const Realm::ProfilingMeasurements::OperationProcessorUsage &usage)
    //--------------------------------------------------------------------------
    {
#ifdef DEBUG_LEGION
      assert(response.has_measurement<
          Realm::ProfilingMeasurements::OperationTimeline>());
#endif
      Realm::ProfilingMeasurements::OperationTimeline timeline;
      response.get_measurement<
            Realm::ProfilingMeasurements::OperationTimeline>(timeline);
      Realm::ProfilingMeasurements::OperationEventWaits waits;
      response.get_measurement<
            Realm::ProfilingMeasurements::OperationEventWaits>(waits);
#ifdef DEBUG_LEGION
      assert(timeline.is_valid());
#endif
      Realm::ProfilingMeasurements::OperationTimelineGPU timeline_gpu;
      if (response.get_measurement<
            Realm::ProfilingMeasurements::OperationTimelineGPU>(timeline_gpu))
      {
#ifdef DEBUG_LEGION
        assert(timeline_gpu.is_valid());
#endif
        gpu_task_infos.emplace_back(GPUTaskInfo());
        GPUTaskInfo &info = gpu_task_infos.back();
        info.op_id = prof_info->op_id;
        info.task_id = prof_info->id;
        info.variant_id = prof_info->extra.id2;
        info.proc_id = usage.proc.id;
        info.create = timeline.create_time;
        info.ready = timeline.ready_time;
        info.start = timeline.start_time;
        // use complete_time instead of end_time to include async work
        info.stop = timeline.complete_time;

        // record gpu time
        info.gpu_start = timeline_gpu.start_time;
        info.gpu_stop = timeline_gpu.end_time;

        unsigned num_intervals = waits.intervals.size();
        if (num_intervals > 0)
        {
          for (unsigned idx = 0; idx < num_intervals; ++idx)
          {
            info.wait_intervals.emplace_back(WaitInfo());
            WaitInfo& wait_info = info.wait_intervals.back();
            wait_info.wait_start = waits.intervals[idx].wait_start;
            wait_info.wait_ready = waits.intervals[idx].wait_ready;
            wait_info.wait_end = waits.intervals[idx].wait_end;
          }
        }
#ifdef LEGION_PROF_PROVENANCE
        info.provenance = prof_info->provenance;
#endif
        Realm::ProfilingMeasurements::OperationFinishEvent finish;
        if (response.get_measurement(finish))
          info.finish_event = LgEvent(finish.finish_event);
        const size_t diff = sizeof(GPUTaskInfo) + 
          num_intervals * sizeof(WaitInfo);
        owner->update_footprint(diff, this);
      }
      else
      {
        task_infos.emplace_back(TaskInfo()); 
        TaskInfo &info = task_infos.back();
        info.op_id = prof_info->op_id;
        info.task_id = prof_info->id;
        info.variant_id = prof_info->extra.id2;
        info.proc_id = usage.proc.id;
        info.create = timeline.create_time;
        info.ready = timeline.ready_time;
        info.start = timeline.start_time;
        // use complete_time instead of end_time to include async work
        info.stop = timeline.complete_time;
        unsigned num_intervals = waits.intervals.size();
        if (num_intervals > 0)
        {
          for (unsigned idx = 0; idx < num_intervals; ++idx)
          {
            info.wait_intervals.emplace_back(WaitInfo());
            WaitInfo& wait_info = info.wait_intervals.back();
            wait_info.wait_start = waits.intervals[idx].wait_start;
            wait_info.wait_ready = waits.intervals[idx].wait_ready;
            wait_info.wait_end = waits.intervals[idx].wait_end;
          }
        }
#ifdef LEGION_PROF_PROVENANCE
        info.provenance = prof_info->provenance;
#endif
        Realm::ProfilingMeasurements::OperationFinishEvent finish;
        if (response.get_measurement(finish))
          info.finish_event = LgEvent(finish.finish_event);
        const size_t diff = sizeof(TaskInfo) + num_intervals * sizeof(WaitInfo);
        owner->update_footprint(diff, this);
      }
    }

    //--------------------------------------------------------------------------
    void LegionProfInstance::process_meta(const ProfilingInfo *prof_info,
             const Realm::ProfilingResponse &response,
             const Realm::ProfilingMeasurements::OperationProcessorUsage &usage)
    //--------------------------------------------------------------------------
    {
#ifdef DEBUG_LEGION
      assert(response.has_measurement<
          Realm::ProfilingMeasurements::OperationTimeline>());
#endif
      Realm::ProfilingMeasurements::OperationTimeline timeline;
      response.get_measurement<
            Realm::ProfilingMeasurements::OperationTimeline>(timeline);
      Realm::ProfilingMeasurements::OperationEventWaits waits;
      response.get_measurement<
            Realm::ProfilingMeasurements::OperationEventWaits>(waits);
#ifdef DEBUG_LEGION
      assert(timeline.is_valid());
#endif
      meta_infos.emplace_back(MetaInfo());
      MetaInfo &info = meta_infos.back();
      info.op_id = prof_info->op_id;
      info.lg_id = prof_info->id;
      info.proc_id = usage.proc.id;
      info.create = timeline.create_time;
      info.ready = timeline.ready_time;
      info.start = timeline.start_time;
      // use complete_time instead of end_time to include async work
      info.stop = timeline.complete_time;
      unsigned num_intervals = waits.intervals.size();
      if (num_intervals > 0)
      {
        for (unsigned idx = 0; idx < num_intervals; ++idx)
        {
          info.wait_intervals.emplace_back(WaitInfo());
          WaitInfo& wait_info = info.wait_intervals.back();
          wait_info.wait_start = waits.intervals[idx].wait_start;
          wait_info.wait_ready = waits.intervals[idx].wait_ready;
          wait_info.wait_end = waits.intervals[idx].wait_end;
        }
      }
#ifdef LEGION_PROF_PROVENANCE
      info.provenance = prof_info->provenance;
#endif
      Realm::ProfilingMeasurements::OperationFinishEvent finish;
      if (response.get_measurement(finish))
        info.finish_event = LgEvent(finish.finish_event);
      const size_t diff = sizeof(MetaInfo) + num_intervals * sizeof(WaitInfo);
      owner->update_footprint(diff, this);
    }

    //--------------------------------------------------------------------------
    void LegionProfInstance::process_message(const ProfilingInfo *prof_info,
             const Realm::ProfilingResponse &response,
             const Realm::ProfilingMeasurements::OperationProcessorUsage &usage)
    //--------------------------------------------------------------------------
    {
#ifdef DEBUG_LEGION
      assert(response.has_measurement<
          Realm::ProfilingMeasurements::OperationTimeline>());
#endif
      Realm::ProfilingMeasurements::OperationTimeline timeline;
      response.get_measurement<
            Realm::ProfilingMeasurements::OperationTimeline>(timeline);
      Realm::ProfilingMeasurements::OperationEventWaits waits;
      response.get_measurement<
            Realm::ProfilingMeasurements::OperationEventWaits>(waits);
#ifdef DEBUG_LEGION
      assert(timeline.is_valid());
#endif
      meta_infos.emplace_back(MetaInfo());
      MetaInfo &info = meta_infos.back();
      info.op_id = prof_info->op_id;
      info.lg_id = prof_info->id;
      info.proc_id = usage.proc.id;
      info.create = timeline.create_time;
      info.ready = timeline.ready_time;
      info.start = timeline.start_time;
      // use complete_time instead of end_time to include async work
      info.stop = timeline.complete_time;
      unsigned num_intervals = waits.intervals.size();
      if (num_intervals > 0)
      {
        for (unsigned idx = 0; idx < num_intervals; ++idx)
        {
          info.wait_intervals.emplace_back(WaitInfo());
          WaitInfo& wait_info = info.wait_intervals.back();
          wait_info.wait_start = waits.intervals[idx].wait_start;
          wait_info.wait_ready = waits.intervals[idx].wait_ready;
          wait_info.wait_end = waits.intervals[idx].wait_end;
        }
      }
#ifdef LEGION_PROF_PROVENANCE
      info.provenance = prof_info->provenance;
#endif
      Realm::ProfilingMeasurements::OperationFinishEvent finish;
      if (response.get_measurement(finish))
        info.finish_event = LgEvent(finish.finish_event);
      const size_t diff = sizeof(MetaInfo) + num_intervals * sizeof(WaitInfo);
      owner->update_footprint(diff, this);
    }

    //--------------------------------------------------------------------------
    void LegionProfInstance::process_copy(const ProfilingInfo *prof_info,
            const Realm::ProfilingResponse &response,
            const Realm::ProfilingMeasurements::OperationMemoryUsage &usage)
    //--------------------------------------------------------------------------
    {
#ifdef DEBUG_LEGION
      assert(response.has_measurement<
          Realm::ProfilingMeasurements::OperationTimeline>());
      assert(response.has_measurement<
          Realm::ProfilingMeasurements::OperationCopyInfo>());
      assert(response.has_measurement<
          Realm::ProfilingMeasurements::OperationFinishEvent>());
#endif

      Realm::ProfilingMeasurements::OperationCopyInfo cpinfo;
      response.get_measurement<
        Realm::ProfilingMeasurements::OperationCopyInfo>(cpinfo);

      Realm::ProfilingMeasurements::OperationTimeline timeline;
      response.get_measurement<
        Realm::ProfilingMeasurements::OperationTimeline>(timeline);

      Realm::ProfilingMeasurements::OperationFinishEvent fevent;
      fevent.finish_event = Realm::Event::NO_EVENT;
      response.get_measurement<
        Realm::ProfilingMeasurements::OperationFinishEvent>(fevent);

#ifdef DEBUG_LEGION
      assert(timeline.is_valid());
#endif
      copy_infos.emplace_back(CopyInfo());
      CopyInfo &info = copy_infos.back();
      info.op_id = prof_info->op_id;
      info.size = usage.size;
      info.create = timeline.create_time;
      info.ready = timeline.ready_time;
      info.start = timeline.start_time;
      // use complete_time instead of end_time to include async work
      info.stop = timeline.complete_time;
      info.fevent = LgEvent(fevent.finish_event);
      info.collective = (CollectiveKind)prof_info->id;
      assert(!cpinfo.inst_info.empty());
      InstanceNameClosure *closure = prof_info->extra.closure;
      typedef Realm::ProfilingMeasurements::OperationCopyInfo::InstInfo 
        InstInfo;
      for (std::vector<InstInfo>::const_iterator it =
            cpinfo.inst_info.begin(); it != cpinfo.inst_info.end(); it++)
      {
#ifdef DEBUG_LEGION
        assert(it->src_fields.size() == it->dst_fields.size());
#endif
        if (it->src_indirection_inst.exists() ||
            it->dst_indirection_inst.exists())
        {
          // Apparently we have to do the full cross-product of
          // everything here. I don't really understand so just
          // log what the Realm developers say and redirect any
          // questions from the profiler back to Realm
          unsigned offset = info.inst_infos.size();
          info.inst_infos.resize(offset + (it->src_insts.size() * 
                it->src_fields.size() * it->dst_insts.size() *
                it->dst_fields.size()) + 1/*extra for indirection*/);
          // Finally log the indirection instance(s)
          CopyInstInfo &indirect = info.inst_infos[offset++];
          indirect.indirect = true;
          indirect.num_hops = it->num_hops;
          if (it->src_indirection_inst.exists())
          {
            indirect.src = it->src_indirection_inst.get_location().id;
            indirect.src_fid = it->src_indirection_field;
            indirect.src_inst_uid = 
              closure->find_instance_name(it->src_indirection_inst);
          }
          else
          {
            indirect.src = 0;
            indirect.src_fid = 0;
            indirect.src_inst_uid = LgEvent::NO_LG_EVENT;
          }
          if (it->dst_indirection_inst.exists())
          {
            indirect.dst = it->dst_indirection_inst.get_location().id;
            indirect.dst_fid = it->dst_indirection_field;
            indirect.dst_inst_uid =
              closure->find_instance_name(it->dst_indirection_inst);
          }
          else
          {
            indirect.dst = 0;
            indirect.dst_fid = 0;
            indirect.dst_inst_uid = LgEvent::NO_LG_EVENT;
          }
          for (unsigned idx1 = 0; idx1 < it->src_insts.size(); idx1++)
          {
            PhysicalInstance src_inst = it->src_insts[idx1];
            Memory src_location = src_inst.get_location();
            LgEvent src_name = closure->find_instance_name(src_inst);
            for (unsigned idx2 = 0; idx2 < it->dst_insts.size(); idx2++)
            {
              PhysicalInstance dst_inst = it->dst_insts[idx2];
              Memory dst_location = dst_inst.get_location();
              LgEvent dst_name = closure->find_instance_name(dst_inst);
              for (unsigned idx3 = 0; idx3 < it->src_fields.size(); idx3++)
              {
                const FieldID src_fid = it->src_fields[idx3];
                for (unsigned idx4 = 0; idx4 < it->dst_fields.size(); idx4++)
                {
                  const FieldID dst_fid = it->dst_fields[idx4];
                  CopyInstInfo &inst_info = info.inst_infos[offset++];
                  inst_info.src = src_location.id;
                  inst_info.dst = dst_location.id;
                  inst_info.src_fid = src_fid;
                  inst_info.dst_fid = dst_fid;
                  inst_info.src_inst_uid = src_name;
                  inst_info.dst_inst_uid = dst_name;
                  inst_info.num_hops = it->num_hops;
                  inst_info.indirect = false;
                }
              }
            }
          }
        }
        else
        {
#ifdef DEBUG_LEGION
          // Ask the Realm developers about why these assertions are true
          // because I still don't completely understand the logic
          assert(it->src_insts.size() == 1);
          assert(it->dst_insts.size() == 1);
#endif
          PhysicalInstance src_inst = it->src_insts.front();
          PhysicalInstance dst_inst = it->dst_insts.front();
          Memory src_location = src_inst.get_location();
          Memory dst_location = dst_inst.get_location();
          LgEvent src_name = closure->find_instance_name(src_inst);
          LgEvent dst_name = closure->find_instance_name(dst_inst);
          const unsigned offset = info.inst_infos.size();
          info.inst_infos.resize(offset + it->src_fields.size());
          for (unsigned idx = 0; idx < it->src_fields.size(); idx++)
          {
            CopyInstInfo &inst_info = info.inst_infos[offset+idx];
            inst_info.src = src_location.id;
            inst_info.dst = dst_location.id;
            inst_info.src_fid = it->src_fields[idx];
            inst_info.dst_fid = it->dst_fields[idx];
            inst_info.src_inst_uid = src_name;
            inst_info.dst_inst_uid = dst_name;
            inst_info.num_hops = it->num_hops;
            inst_info.indirect = false;
          }
        }
      }
#ifdef LEGION_PROF_PROVENANCE
      info.provenance = prof_info->provenance;
#endif
      owner->update_footprint(sizeof(CopyInfo) +
          info.inst_infos.size() * sizeof(CopyInstInfo), this);
      if (closure->remove_reference())
        delete closure;
    }

    //--------------------------------------------------------------------------
    void LegionProfInstance::process_fill(const ProfilingInfo *prof_info,
            const Realm::ProfilingResponse &response,
            const Realm::ProfilingMeasurements::OperationMemoryUsage &usage)
    //--------------------------------------------------------------------------
    {
#ifdef DEBUG_LEGION
      assert(response.has_measurement<
          Realm::ProfilingMeasurements::OperationCopyInfo>());
      assert(response.has_measurement<
          Realm::ProfilingMeasurements::OperationTimeline>());
#endif
      Realm::ProfilingMeasurements::OperationCopyInfo cpinfo;
      response.get_measurement<
        Realm::ProfilingMeasurements::OperationCopyInfo>(cpinfo);

      Realm::ProfilingMeasurements::OperationTimeline timeline;
      response.get_measurement<
            Realm::ProfilingMeasurements::OperationTimeline>(timeline);
#ifdef DEBUG_LEGION
      assert(timeline.is_valid());
#endif
      fill_infos.emplace_back(FillInfo());
      FillInfo &info = fill_infos.back();
      info.op_id = prof_info->op_id;
      info.size = usage.size;
      info.create = timeline.create_time;
      info.ready = timeline.ready_time;
      info.start = timeline.start_time;
      // use complete_time instead of end_time to include async work
      info.stop = timeline.complete_time;
      Realm::ProfilingMeasurements::OperationFinishEvent fevent;
      if (response.get_measurement(fevent))
        info.fevent = LgEvent(fevent.finish_event);
      info.collective = (CollectiveKind)prof_info->id;
      InstanceNameClosure *closure = prof_info->extra.closure;
      typedef Realm::ProfilingMeasurements::OperationCopyInfo::InstInfo 
        InstInfo;
      for (std::vector<InstInfo>::const_iterator it =
            cpinfo.inst_info.begin(); it != cpinfo.inst_info.end(); it++)
      {
#ifdef DEBUG_LEGION
        assert(!it->dst_fields.empty());
        assert(it->dst_insts.size() == 1);
#endif
        PhysicalInstance instance = it->dst_insts.front();
        Memory location = instance.get_location();
        LgEvent name = closure->find_instance_name(instance);
        unsigned offset = info.inst_infos.size();
        info.inst_infos.resize(offset + it->dst_fields.size());
        for (unsigned idx = 0; idx < it->dst_fields.size(); idx++)
        {
          FillInstInfo &inst_info = info.inst_infos[offset+idx];
          inst_info.dst = location.id;
          inst_info.fid = it->dst_fields[idx];
          inst_info.dst_inst_uid = name; 
        }
      }
#ifdef LEGION_PROF_PROVENANCE
      info.provenance = prof_info->provenance;
#endif
      owner->update_footprint(sizeof(FillInfo) + 
          info.inst_infos.size() * sizeof(FillInstInfo), this);
      if (closure->remove_reference())
        delete closure;
    }

    //--------------------------------------------------------------------------
    void LegionProfInstance::process_inst_timeline(
                 const ProfilingInfo *prof_info,
                 const Realm::ProfilingResponse &response,
                 const Realm::ProfilingMeasurements::InstanceMemoryUsage &usage,
                 const Realm::ProfilingMeasurements::InstanceTimeline &timeline)
    //--------------------------------------------------------------------------
    {
      inst_timeline_infos.emplace_back(InstTimelineInfo());
      InstTimelineInfo &info = inst_timeline_infos.back();
      info.inst_uid.id = prof_info->id;
      info.inst_id = usage.instance.id;
      info.mem_id = usage.memory.id;
      info.size = usage.bytes;
      info.op_id = prof_info->op_id;
      info.create = timeline.create_time;
      info.ready = timeline.ready_time;
      info.destroy = timeline.delete_time;
      owner->update_footprint(sizeof(InstTimelineInfo), this);
    }

    //--------------------------------------------------------------------------
    void LegionProfInstance::process_partition(const ProfilingInfo *prof_info,
                                       const Realm::ProfilingResponse &response)
    //--------------------------------------------------------------------------
    {
#ifdef DEBUG_LEGION
      assert(response.has_measurement<
          Realm::ProfilingMeasurements::OperationTimeline>());
#endif
      Realm::ProfilingMeasurements::OperationTimeline timeline;
      response.get_measurement<
            Realm::ProfilingMeasurements::OperationTimeline>(timeline);
      partition_infos.emplace_back(PartitionInfo());
      PartitionInfo &info = partition_infos.back();
      info.op_id = prof_info->op_id;
      info.part_op = (DepPartOpKind)prof_info->id;
      info.create = timeline.create_time;
      info.ready = timeline.ready_time;
      info.start = timeline.start_time;
      // use complete_time instead of end_time to include async work
      info.stop = timeline.complete_time;
#ifdef LEGION_PROF_PROVENANCE
      info.provenance = prof_info->provenance;
#endif
      owner->update_footprint(sizeof(PartitionInfo), this);
    }

    //--------------------------------------------------------------------------
    void LegionProfInstance::process_implicit(UniqueID op_id, TaskID tid,
        Processor proc, long long start_time, long long stop_time,
        const std::vector<std::pair<long long,long long> > &waits,
        LgEvent finish_event)
    //--------------------------------------------------------------------------
    {
      task_infos.emplace_back(TaskInfo()); 
      TaskInfo &info = task_infos.back();
      info.op_id = op_id;
      info.task_id = tid;
      info.variant_id = 0; // no variants for implicit tasks
      info.proc_id = proc.id;
      // We make create, ready, and start all the same for implicit tasks
      info.create = start_time;
      info.ready = start_time;
      info.start = start_time;
      info.stop = stop_time;
      if (!waits.empty())
      {
        info.wait_intervals.resize(waits.size());
        for (unsigned idx = 0; idx < waits.size(); idx++)
        {
          info.wait_intervals[idx].wait_start = waits[idx].first;
          // For implicit tasks, these are external waits so we just
          // assume that they resume right away
          info.wait_intervals[idx].wait_ready = waits[idx].second;
          info.wait_intervals[idx].wait_end = waits[idx].second;
        }
      }
      info.finish_event = finish_event;
    }

    //--------------------------------------------------------------------------
    void LegionProfInstance::process_mem_desc(const Memory &m)
    //--------------------------------------------------------------------------
    {
      if (m == Memory::NO_MEMORY)
        return;
      if (std::binary_search(mem_ids.begin(), mem_ids.end(), m.id))
        return;
      mem_ids.push_back(m.id);
      std::sort(mem_ids.begin(), mem_ids.end());

      mem_desc_infos.emplace_back(MemDesc());
      MemDesc &info = mem_desc_infos.back();
      info.mem_id = m.id;
      info.kind  = m.kind();
      info.capacity = m.capacity();
      const size_t diff = sizeof(MemDesc);
      owner->update_footprint(diff, this);
      process_proc_mem_aff_desc(m);
    }

    //--------------------------------------------------------------------------
    void LegionProfInstance::process_proc_desc(const Processor &p)
    //--------------------------------------------------------------------------
    {
      if (std::binary_search(proc_ids.begin(), proc_ids.end(), p.id))
        return;
      proc_ids.push_back(p.id);
      std::sort(proc_ids.begin(), proc_ids.end());

      proc_desc_infos.emplace_back(ProcDesc());
      ProcDesc &info = proc_desc_infos.back();
      info.proc_id = p.id;
      info.kind = p.kind();
      const size_t diff = sizeof(ProcDesc);
      owner->update_footprint(diff, this);
      process_proc_mem_aff_desc(p);
    }

    //--------------------------------------------------------------------------
    void LegionProfInstance::process_proc_mem_aff_desc(const Memory &m)
    //--------------------------------------------------------------------------
    {
      unsigned int entry_count = 0;
      // record ALL memory<->processor affinities for consistency + if needed in the future
      std::vector<ProcessorMemoryAffinity> affinities;
      Machine::get_machine().get_proc_mem_affinity(affinities, Processor::NO_PROC, m);
      for (std::vector<ProcessorMemoryAffinity>::const_iterator it =
             affinities.begin(); it != affinities.end(); it++)
        {
          process_proc_desc(it->p);
          proc_mem_aff_desc_infos.emplace_back(ProcMemDesc());
          ProcMemDesc &info = proc_mem_aff_desc_infos.back();
          info.proc_id = it->p.id;
          info.mem_id = m.id;
          info.bandwidth = it->bandwidth;
          info.latency = it->latency;
          entry_count++;
        }
      if (entry_count > 0)
        owner->update_footprint(sizeof(ProcMemDesc)*entry_count, this);
    }

    //--------------------------------------------------------------------------
    void LegionProfInstance::process_proc_mem_aff_desc(const Processor &p)
    //--------------------------------------------------------------------------
    {
      // record ALL processor<->memory affinities for consistency
      // and for possible querying in the future
      std::vector<ProcessorMemoryAffinity> affinities;
      Machine::get_machine().get_proc_mem_affinity(affinities, p);
      for (std::vector<ProcessorMemoryAffinity>::const_iterator it =
             affinities.begin(); it != affinities.end(); it++) {
        process_mem_desc(it->m); // add memory + affinity
      }
    }

    //--------------------------------------------------------------------------
    void LegionProfInstance::record_mapper_call(Processor proc, 
                              MappingCallKind kind, UniqueID uid,
                              unsigned long long start, unsigned long long stop,
                              LgEvent finish_event)
    //--------------------------------------------------------------------------
    {
      // Check to see if it exceeds the call threshold
      if ((stop - start) < owner->minimum_call_threshold)
        return;
      mapper_call_infos.emplace_back(MapperCallInfo());
      MapperCallInfo &info = mapper_call_infos.back();
      info.kind = kind;
      info.op_id = uid;
      info.start = start;
      info.stop = stop;
      info.proc_id = proc.id;
      info.finish_event = finish_event;
      owner->update_footprint(sizeof(MapperCallInfo), this);
    }

    //--------------------------------------------------------------------------
    void LegionProfInstance::record_runtime_call(Processor proc, 
        RuntimeCallKind kind, unsigned long long start, unsigned long long stop,
        LgEvent finish_event)
    //--------------------------------------------------------------------------
    {
      // Check to see if it exceeds the call threshold
      if ((stop - start) < owner->minimum_call_threshold)
        return;
      runtime_call_infos.emplace_back(RuntimeCallInfo());
      RuntimeCallInfo &info = runtime_call_infos.back();
      info.kind = kind;
      info.start = start;
      info.stop = stop;
      info.proc_id = proc.id;
      info.finish_event = finish_event;
      owner->update_footprint(sizeof(RuntimeCallInfo), this);
    }

#ifdef LEGION_PROF_SELF_PROFILE
    //--------------------------------------------------------------------------
    void LegionProfInstance::record_proftask(Processor proc, UniqueID op_id,
					     unsigned long long start,
					     unsigned long long stop,
                                             LgEvent finish_event)
    //--------------------------------------------------------------------------
    {
      prof_task_infos.emplace_back(ProfTaskInfo());
      ProfTaskInfo &info = prof_task_infos.back();
      info.proc_id = proc.id;
      info.op_id = op_id;
      info.start = start;
      info.stop = stop;
      info.finish_event = finish_event;
      owner->update_footprint(sizeof(ProfTaskInfo), this);
    }
#endif

    //--------------------------------------------------------------------------
    void LegionProfInstance::dump_state(LegionProfSerializer *serializer)
    //--------------------------------------------------------------------------
    {
      for (std::deque<MemDesc>::const_iterator it =
            mem_desc_infos.begin(); it != mem_desc_infos.end(); it++)
      {
        serializer->serialize(*it);
      }
      for (std::deque<ProcDesc>::const_iterator it =
            proc_desc_infos.begin(); it != proc_desc_infos.end(); it++)
      {
        serializer->serialize(*it);
      }
      for (std::deque<ProcMemDesc>::const_iterator it =
            proc_mem_aff_desc_infos.begin();
           it != proc_mem_aff_desc_infos.end(); it++)
      {
        serializer->serialize(*it);
      }

      for (std::deque<TaskKind>::const_iterator it = task_kinds.begin();
            it != task_kinds.end(); it++)
      {
        serializer->serialize(*it);
        free(const_cast<char*>(it->name));
      }
      for (std::deque<TaskVariant>::const_iterator it = task_variants.begin();
            it != task_variants.end(); it++)
      {
        serializer->serialize(*it);
        free(const_cast<char*>(it->name));
      }
      for (std::deque<OperationInstance>::const_iterator it = 
            operation_instances.begin(); it != operation_instances.end(); it++)
      {
        serializer->serialize(*it);
        if (it->provenance != NULL)
          free(const_cast<char*>(it->provenance));
      }
      for (std::deque<MultiTask>::const_iterator it = 
            multi_tasks.begin(); it != multi_tasks.end(); it++)
      {
        serializer->serialize(*it);
      }
      for (std::deque<SliceOwner>::const_iterator it = 
            slice_owners.begin(); it != slice_owners.end(); it++)
      {
        serializer->serialize(*it);
      }
      for (std::deque<TaskInfo>::const_iterator it = task_infos.begin();
            it != task_infos.end(); it++)
      {
        serializer->serialize(*it);
        for (std::deque<WaitInfo>::const_iterator wit =
             it->wait_intervals.begin(); wit != it->wait_intervals.end(); wit++)
        {
          serializer->serialize(*wit, *it);
        }
      }
      for (std::deque<GPUTaskInfo>::const_iterator it = gpu_task_infos.begin();
            it != gpu_task_infos.end(); it++)
      {
        serializer->serialize(*it);
        for (std::deque<WaitInfo>::const_iterator wit =
             it->wait_intervals.begin(); wit != it->wait_intervals.end(); wit++)
        {
          serializer->serialize(*wit, *it);
        }
      }
      for (std::deque<IndexSpaceRectDesc>::const_iterator it =
	     ispace_rect_desc.begin(); it != ispace_rect_desc.end(); it++)
      {
        serializer->serialize(*it);
      }

      for (std::deque<IndexSpacePointDesc>::const_iterator it =
	     ispace_point_desc.begin(); it != ispace_point_desc.end(); it++)
      {
        serializer->serialize(*it);
      }
      for (std::deque<IndexSpaceEmptyDesc>::const_iterator it =
	     ispace_empty_desc.begin(); it != ispace_empty_desc.end(); it++)
      {
        serializer->serialize(*it);
      }
      for (std::deque<FieldDesc>::const_iterator it =
	     field_desc.begin(); it != field_desc.end(); it++)
      {
        serializer->serialize(*it);
      }
      for (std::deque<FieldSpaceDesc>::const_iterator it =
	     field_space_desc.begin(); it != field_space_desc.end(); it++)
      {
        serializer->serialize(*it);
      }
      for (std::deque<IndexPartDesc>::const_iterator it =
	     index_part_desc.begin(); it != index_part_desc.end(); it++)
      {
        serializer->serialize(*it);
      }

      for (std::deque<IndexSubSpaceDesc>::const_iterator it =
	     index_subspace_desc.begin(); it != index_subspace_desc.end(); it++)
      {
        serializer->serialize(*it);
      }

      for (std::deque<IndexPartitionDesc>::const_iterator it =
	     index_partition_desc.begin(); it != index_partition_desc.end(); it++)
      {
        serializer->serialize(*it);
      }

      for (std::deque<LogicalRegionDesc>::const_iterator it =
	     lr_desc.begin(); it != lr_desc.end(); it++)
      {
        serializer->serialize(*it);
      }

      for (std::deque<PhysicalInstRegionDesc>::const_iterator it =
	     phy_inst_rdesc.begin();
	   it != phy_inst_rdesc.end(); it++)
      {
        serializer->serialize(*it);
      }
      for (std::deque<PhysicalInstLayoutDesc>::const_iterator it =
	     phy_inst_layout_rdesc.begin();
	   it != phy_inst_layout_rdesc.end(); it++)
      {
        serializer->serialize(*it);
      }

      for (std::deque<PhysicalInstDimOrderDesc>::const_iterator it =
	     phy_inst_dim_order_rdesc.begin();
	   it != phy_inst_dim_order_rdesc.end(); it++)
      {
        serializer->serialize(*it);
      }

      for (std::deque<PhysicalInstanceUsage>::const_iterator it =
            phy_inst_usage.begin(); it != phy_inst_usage.end(); it++)
      {
        serializer->serialize(*it);
      }

      for (std::deque<IndexSpaceSizeDesc>::const_iterator it =
             index_space_size_desc.begin();
           it != index_space_size_desc.end(); it++)
        {
          serializer->serialize(*it);
        }

      for (std::deque<MetaInfo>::const_iterator it = meta_infos.begin();
            it != meta_infos.end(); it++)
      {
        serializer->serialize(*it);
        for (std::deque<WaitInfo>::const_iterator wit =
             it->wait_intervals.begin(); wit != it->wait_intervals.end(); wit++)
        {
          serializer->serialize(*wit, *it);
        }
      }
      for (std::deque<FillInfo>::const_iterator it = fill_infos.begin();
            it != fill_infos.end(); it++)
      {
        serializer->serialize(*it);
      }
      for (std::deque<CopyInfo>::const_iterator it = copy_infos.begin();
           it != copy_infos.end(); it++)
      {
        serializer->serialize(*it);
      }
      for (std::deque<InstTimelineInfo>::const_iterator it = 
            inst_timeline_infos.begin(); it != inst_timeline_infos.end(); it++)
      {
        serializer->serialize(*it);
      }
      for (std::deque<PartitionInfo>::const_iterator it = 
            partition_infos.begin(); it != partition_infos.end(); it++)
      {
        serializer->serialize(*it);
      }
      for (std::deque<MapperCallInfo>::const_iterator it = 
            mapper_call_infos.begin(); it != mapper_call_infos.end(); it++)
      {
        serializer->serialize(*it);
      }
      for (std::deque<RuntimeCallInfo>::const_iterator it = 
            runtime_call_infos.begin(); it != runtime_call_infos.end(); it++)
      {
        serializer->serialize(*it);
      }

#ifdef LEGION_PROF_SELF_PROFILE
      for (std::deque<ProfTaskInfo>::const_iterator it = 
            prof_task_infos.begin(); it != prof_task_infos.end(); it++)
      {
        serializer->serialize(*it);
      }
#endif
      task_kinds.clear();
      task_variants.clear();
      operation_instances.clear();
      multi_tasks.clear();
      task_infos.clear();
      gpu_task_infos.clear();
      ispace_rect_desc.clear();
      ispace_point_desc.clear();
      ispace_empty_desc.clear();
      field_desc.clear();
      field_space_desc.clear();
      index_part_desc.clear();
      index_space_desc.clear();
      index_subspace_desc.clear();
      index_partition_desc.clear();
      lr_desc.clear();
      phy_inst_layout_rdesc.clear();
      phy_inst_rdesc.clear();
      phy_inst_dim_order_rdesc.clear();
      index_space_size_desc.clear();
      meta_infos.clear();
      copy_infos.clear();
      fill_infos.clear();
      inst_timeline_infos.clear();
      partition_infos.clear();
      mapper_call_infos.clear();
      mem_desc_infos.clear();
      proc_desc_infos.clear();
      proc_mem_aff_desc_infos.clear();
    }

    //--------------------------------------------------------------------------
    size_t LegionProfInstance::dump_inter(LegionProfSerializer *serializer,
                                          const double over)
    //--------------------------------------------------------------------------
    {
      // Start the timing so we know how long we are taking
      const long long t_start = Realm::Clock::current_time_in_microseconds();
      // Scale our latency by how much we are over the space limit
      const long long t_stop = t_start + over * owner->output_target_latency;
      size_t diff = 0; 
      while (!mem_desc_infos.empty())
        {
          MemDesc &front = mem_desc_infos.front();
          serializer->serialize(front);
          diff += sizeof(front);
          mem_desc_infos.pop_front();
          const long long t_curr = Realm::Clock::current_time_in_microseconds();
          if (t_curr >= t_stop)
            return diff;
        }
      while (!proc_desc_infos.empty())
        {
          ProcDesc &front = proc_desc_infos.front();
          serializer->serialize(front);
          diff += sizeof(front);
          proc_desc_infos.pop_front();
          const long long t_curr = Realm::Clock::current_time_in_microseconds();
          if (t_curr >= t_stop)
            return diff;
        }
      while (!proc_mem_aff_desc_infos.empty())
        {
          ProcMemDesc &front = proc_mem_aff_desc_infos.front();
          serializer->serialize(front);
          diff += sizeof(front);
          proc_mem_aff_desc_infos.pop_front();
          const long long t_curr = Realm::Clock::current_time_in_microseconds();
          if (t_curr >= t_stop)
            return diff;
        }
      while (!task_kinds.empty())
      {
        TaskKind &front = task_kinds.front();
        serializer->serialize(front);
        diff += sizeof(front) + strlen(front.name);
        free(const_cast<char*>(front.name));
        task_kinds.pop_front();
        const long long t_curr = Realm::Clock::current_time_in_microseconds();
        if (t_curr >= t_stop)
          return diff;
      }
      while (!task_variants.empty())
      {
        TaskVariant &front = task_variants.front();
        serializer->serialize(front);
        diff += sizeof(front) + strlen(front.name);
        free(const_cast<char*>(front.name));
        task_variants.pop_front();
        const long long t_curr = Realm::Clock::current_time_in_microseconds();
        if (t_curr >= t_stop)
          return diff;
      }
      while (!operation_instances.empty())
      {
        OperationInstance &front = operation_instances.front();
        serializer->serialize(front);
        diff += sizeof(front);
        if (front.provenance != NULL)
        {
          diff += strlen(front.provenance);
          free(const_cast<char*>(front.provenance));
        }
        operation_instances.pop_front();
        const long long t_curr = Realm::Clock::current_time_in_microseconds();
        if (t_curr >= t_stop)
          return diff;
      }
      while (!multi_tasks.empty())
      {
        MultiTask &front = multi_tasks.front();
        serializer->serialize(front);
        diff += sizeof(front);
        multi_tasks.pop_front();
        const long long t_curr = Realm::Clock::current_time_in_microseconds();
        if (t_curr >= t_stop)
          return diff;
      }
      while (!slice_owners.empty())
      {
        SliceOwner &front = slice_owners.front();
        serializer->serialize(front);
        diff += sizeof(front);
        slice_owners.pop_front();
        const long long t_curr = Realm::Clock::current_time_in_microseconds();
        if (t_curr >= t_stop)
          return diff;
      }
      while (!task_infos.empty())
      {
        TaskInfo &front = task_infos.front();
        serializer->serialize(front);
        // Have to do all of these now
        for (std::deque<WaitInfo>::const_iterator wit =
              front.wait_intervals.begin(); wit != 
              front.wait_intervals.end(); wit++)
          serializer->serialize(*wit, front);
        diff += sizeof(front) + front.wait_intervals.size() * sizeof(WaitInfo);
        task_infos.pop_front();
        const long long t_curr = Realm::Clock::current_time_in_microseconds();
        if (t_curr >= t_stop)
          return diff;
      }
      while (!ispace_rect_desc.empty())
      {
        IndexSpaceRectDesc &front = ispace_rect_desc.front();
        serializer->serialize(front);
        diff += sizeof(front);
        ispace_rect_desc.pop_front();
        const long long t_curr = Realm::Clock::current_time_in_microseconds();
        if (t_curr >= t_stop)
          return diff;
      }
      while (!ispace_point_desc.empty())
      {
        IndexSpacePointDesc &front = ispace_point_desc.front();
        serializer->serialize(front);
        diff += sizeof(front);
        ispace_point_desc.pop_front();
        const long long t_curr = Realm::Clock::current_time_in_microseconds();
        if (t_curr >= t_stop)
          return diff;
      }
      while (!ispace_empty_desc.empty())
      {
        IndexSpaceEmptyDesc &front = ispace_empty_desc.front();
        serializer->serialize(front);
        diff += sizeof(front);
        ispace_empty_desc.pop_front();
        const long long t_curr = Realm::Clock::current_time_in_microseconds();
        if (t_curr >= t_stop)
          return diff;
      }
      while (!field_desc.empty())
      {
        FieldDesc &front = field_desc.front();
        serializer->serialize(front);
        diff += sizeof(front) + strlen(front.name);
        free(const_cast<char*>(front.name));
        field_desc.pop_front();
        const long long t_curr = Realm::Clock::current_time_in_microseconds();
        if (t_curr >= t_stop)
          return diff;
      }
      while (!field_space_desc.empty())
      {
        FieldSpaceDesc &front = field_space_desc.front();
        serializer->serialize(front);
        diff += sizeof(front) + strlen(front.name);
        free(const_cast<char*>(front.name));
        field_space_desc.pop_front();
        const long long t_curr = Realm::Clock::current_time_in_microseconds();
        if (t_curr >= t_stop)
          return diff;
      }
      while (!index_part_desc.empty())
      {
        IndexPartDesc &front = index_part_desc.front();
        serializer->serialize(front);
        diff += sizeof(front) + strlen(front.name);
        free(const_cast<char*>(front.name));
        index_part_desc.pop_front();
        const long long t_curr = Realm::Clock::current_time_in_microseconds();
        if (t_curr >= t_stop)
          return diff;
      }
      while (!index_space_desc.empty())
      {
        IndexSpaceDesc &front = index_space_desc.front();
        serializer->serialize(front);
        diff += sizeof(front) + strlen(front.name);
        free(const_cast<char*>(front.name));
        index_space_desc.pop_front();
        const long long t_curr = Realm::Clock::current_time_in_microseconds();
        if (t_curr >= t_stop)
          return diff;
      }
      while (!index_subspace_desc.empty())
      {
        IndexSubSpaceDesc &front = index_subspace_desc.front();
        serializer->serialize(front);
        diff += sizeof(front);
        index_subspace_desc.pop_front();
        const long long t_curr = Realm::Clock::current_time_in_microseconds();
        if (t_curr >= t_stop)
          return diff;
      }
      while (!index_partition_desc.empty())
      {
        IndexPartitionDesc &front = index_partition_desc.front();
        serializer->serialize(front);
        diff += sizeof(front);
        index_partition_desc.pop_front();
        const long long t_curr = Realm::Clock::current_time_in_microseconds();
        if (t_curr >= t_stop)
          return diff;
      }
      while (!lr_desc.empty())
      {
        LogicalRegionDesc &front = lr_desc.front();
        serializer->serialize(front);
        diff += sizeof(front) + strlen(front.name);
        free(const_cast<char*>(front.name));
        lr_desc.pop_front();
        const long long t_curr = Realm::Clock::current_time_in_microseconds();
        if (t_curr >= t_stop)
          return diff;
      }
      while (!phy_inst_rdesc.empty())
      {
        PhysicalInstRegionDesc &front = phy_inst_rdesc.front();
        serializer->serialize(front);
        diff += sizeof(front);
        phy_inst_rdesc.pop_front();
        const long long t_curr = Realm::Clock::current_time_in_microseconds();
        if (t_curr >= t_stop)
          return diff;
      }

      while (!phy_inst_dim_order_rdesc.empty())
      {
        PhysicalInstDimOrderDesc &front = phy_inst_dim_order_rdesc.front();
        serializer->serialize(front);
        diff += sizeof(front);
        phy_inst_dim_order_rdesc.pop_front();
        const long long t_curr = Realm::Clock::current_time_in_microseconds();
        if (t_curr >= t_stop)
          return diff;
      }

      while (!index_space_size_desc.empty())
      {
        IndexSpaceSizeDesc &front = index_space_size_desc.front();
        serializer->serialize(front);
        diff += sizeof(front);
        index_space_size_desc.pop_front();
        const long long t_curr = Realm::Clock::current_time_in_microseconds();
        if (t_curr >= t_stop)
          return diff;
      }

      while (!phy_inst_layout_rdesc.empty())
      {
        PhysicalInstLayoutDesc &front = phy_inst_layout_rdesc.front();
        serializer->serialize(front);
        diff += sizeof(front);
        phy_inst_layout_rdesc.pop_front();
        const long long t_curr = Realm::Clock::current_time_in_microseconds();
        if (t_curr >= t_stop)
          return diff;
      }
      while (!meta_infos.empty())
      {
        MetaInfo &front = meta_infos.front();
        serializer->serialize(front);
        // Have to do all of these now
        for (std::deque<WaitInfo>::const_iterator wit =
              front.wait_intervals.begin(); wit != 
              front.wait_intervals.end(); wit++)
          serializer->serialize(*wit, front);
        diff += sizeof(front) + front.wait_intervals.size() * sizeof(WaitInfo);
        meta_infos.pop_front();
        const long long t_curr = Realm::Clock::current_time_in_microseconds();
        if (t_curr >= t_stop)
          return diff;
      }
      while (!copy_infos.empty())
      {
        CopyInfo &front = copy_infos.front();
        serializer->serialize(front);
        diff += sizeof(front) + front.inst_infos.size() * sizeof(CopyInstInfo);
        copy_infos.pop_front();
        const long long t_curr = Realm::Clock::current_time_in_microseconds();
        if (t_curr >= t_stop)
          return diff;
      }
      while (!fill_infos.empty())
      {
        FillInfo &front = fill_infos.front();
        serializer->serialize(front);
        diff += sizeof(front) + front.inst_infos.size() * sizeof(FillInstInfo);
        fill_infos.pop_front();
        const long long t_curr = Realm::Clock::current_time_in_microseconds();
        if (t_curr >= t_stop)
          return diff;
      }
      while (!inst_timeline_infos.empty())
      {
        InstTimelineInfo &front = inst_timeline_infos.front();
        serializer->serialize(front);
        diff += sizeof(front);
        inst_timeline_infos.pop_front();
        const long long t_curr = Realm::Clock::current_time_in_microseconds();
        if (t_curr >= t_stop)
          return diff;
      }
      while (!partition_infos.empty())
      {
        PartitionInfo &front = partition_infos.front();
        serializer->serialize(front);
        diff += sizeof(front);
        partition_infos.pop_front();
        const long long t_curr = Realm::Clock::current_time_in_microseconds();
        if (t_curr >= t_stop)
          return diff;
      }
      while (!mapper_call_infos.empty())
      {
        MapperCallInfo &front = mapper_call_infos.front();
        serializer->serialize(front);
        diff += sizeof(front);
        mapper_call_infos.pop_front();
        const long long t_curr = Realm::Clock::current_time_in_microseconds();
        if (t_curr >= t_stop)
          return diff;
      }
      while (!runtime_call_infos.empty())
      {
        RuntimeCallInfo &front = runtime_call_infos.front();
        serializer->serialize(front);
        diff += sizeof(front);
        runtime_call_infos.pop_front();
        const long long t_curr = Realm::Clock::current_time_in_microseconds();
        if (t_curr >= t_stop)
          return diff;
      }

#ifdef LEGION_PROF_SELF_PROFILE
      while (!prof_task_infos.empty())
      {
        ProfTaskInfo &front = prof_task_infos.front();
        serializer->serialize(front);
        diff += sizeof(front);
        prof_task_infos.pop_front();
        const long long t_curr = Realm::Clock::current_time_in_microseconds();
        if (t_curr >= t_stop)
          return diff;
      }
#endif
      return diff;
    }

    //--------------------------------------------------------------------------
    LegionProfiler::LegionProfiler(Processor target, const Machine &machine,
                                   Runtime *rt, unsigned num_meta_tasks,
                                   const char *const *const task_descriptions,
                                   unsigned num_message_kinds,
                                   const char *const *const 
                                                         message_descriptions,
                                   unsigned num_operation_kinds,
                                   const char *const *const 
                                                  operation_kind_descriptions,
                                   const char *serializer_type,
                                   const char *prof_logfile,
                                   const size_t total_runtime_instances,
                                   const size_t footprint_threshold,
                                   const size_t target_latency,
                                   const size_t call_threshold,
                                   const bool slow_config_ok)
      : runtime(rt), done_event(Runtime::create_rt_user_event()), 
        minimum_call_threshold(call_threshold * 1000 /*convert us to ns*/),
        output_footprint_threshold(footprint_threshold), 
        output_target_latency(target_latency), target_proc(target), 
#ifndef DEBUG_LEGION
        total_outstanding_requests(1/*start with guard*/),
#endif
        total_memory_footprint(0), need_default_mapper_warning(!slow_config_ok)
    //--------------------------------------------------------------------------
    {
#ifdef DEBUG_LEGION
      assert(target_proc.exists());
#endif
      if (!strcmp(serializer_type, "binary")) 
      {
        if (prof_logfile == NULL) 
          REPORT_LEGION_ERROR(ERROR_UNKNOWN_PROFILER_OPTION,
              "ERROR: Please specify -lg:prof_logfile "
              "<logfile_name> when running with -lg:serializer binary")
        std::string filename(prof_logfile);
        size_t pct = filename.find_first_of('%', 0);
        if (pct == std::string::npos) 
        {
          // This is only an error if we have multiple runtimes
          if (total_runtime_instances > 1)
            REPORT_LEGION_ERROR(ERROR_MISSING_PROFILER_OPTION,
                "ERROR: The logfile name must contain '%%' "
                "which will be replaced with the node id\n")
          serializer = new LegionProfBinarySerializer(filename.c_str());
        }
        else
        {
          // replace % with node number
          std::stringstream ss;
          ss << filename.substr(0, pct) << target.address_space() <<
                filename.substr(pct + 1);
          serializer = new LegionProfBinarySerializer(ss.str());
        }
      } 
      else if (!strcmp(serializer_type, "ascii")) 
      {
        if (prof_logfile != NULL) 
          REPORT_LEGION_WARNING(LEGION_WARNING_UNUSED_PROFILING_FILE_NAME,
                    "You should not specify -lg:prof_logfile "
                    "<logfile_name> when running with -lg:serializer ascii\n"
                    "       legion_prof output will be written to '-logfile "
                    "<logfile_name>' instead")
        serializer = new LegionProfASCIISerializer();
      } 
      else 
        REPORT_LEGION_ERROR(ERROR_INVALID_PROFILER_SERIALIZER,
                "Invalid serializer (%s), must be 'binary' "
                "or 'ascii'\n", serializer_type)

      // log machine info, this needs to be the first log
      LegionProfDesc::MachineDesc machine_desc;

      machine_desc.node_id = static_cast<unsigned>(rt->address_space);
      machine_desc.num_nodes = static_cast<unsigned>(
        rt->total_address_spaces);

      serializer->serialize(machine_desc);

      LegionProfDesc::ZeroTime zero_time;
      zero_time.zero_time = Legion::Runtime::get_zero_time();

      serializer->serialize(zero_time);

      for (unsigned idx = 0; idx < num_meta_tasks; idx++)
      {
        LegionProfDesc::MetaDesc meta_desc;
        meta_desc.kind = idx;
        meta_desc.message = false;
        meta_desc.ordered_vc = false;
        meta_desc.name = task_descriptions[idx];
        serializer->serialize(meta_desc);
      }
      // Messages are appended as kinds of meta descriptions
      for (unsigned idx = 0; idx < num_message_kinds; idx++)
      {
        LegionProfDesc::MetaDesc meta_desc;
        meta_desc.kind = num_meta_tasks + idx;
        meta_desc.message = true;
        const VirtualChannelKind vc = 
          MessageManager::find_message_vc((MessageKind)idx);
        meta_desc.ordered_vc = (vc <= LAST_UNORDERED_VIRTUAL_CHANNEL);
        meta_desc.name = message_descriptions[idx];
        serializer->serialize(meta_desc);
      }
      for (unsigned idx = 0; idx < num_operation_kinds; idx++)
      {
        LegionProfDesc::OpDesc op_desc;
        op_desc.kind = idx;
        op_desc.name = operation_kind_descriptions[idx];
        serializer->serialize(op_desc);
      }
      // log max dim
      LegionProfDesc::MaxDimDesc max_dim_desc;
      max_dim_desc.max_dim = LEGION_MAX_DIM;
      serializer->serialize(max_dim_desc);

#ifdef DEBUG_LEGION
      for (unsigned idx = 0; idx < LEGION_PROF_LAST; idx++)
        total_outstanding_requests[idx] = 0;
      total_outstanding_requests[LEGION_PROF_META] = 1; // guard
#endif
    }

    //--------------------------------------------------------------------------
    LegionProfiler::~LegionProfiler(void)
    //--------------------------------------------------------------------------
    {
      for (std::vector<LegionProfInstance*>::const_iterator it = 
            instances.begin(); it != instances.end(); it++)
        delete (*it);

      // remove our serializer
      delete serializer;
    }

    //--------------------------------------------------------------------------
    void LegionProfiler::record_index_space_rect_desc(
    LegionProfInstance::IndexSpaceRectDesc &ispace_rect_desc)
    //--------------------------------------------------------------------------
    {
      if (thread_local_profiling_instance == NULL)
        create_thread_local_profiling_instance();
      thread_local_profiling_instance->register_index_space_rect(
                                                ispace_rect_desc);
    }

    //--------------------------------------------------------------------------
    void LegionProfiler::record_index_space_point_desc(
    LegionProfInstance::IndexSpacePointDesc &ispace_point_desc)
    //--------------------------------------------------------------------------
    {
      if (thread_local_profiling_instance == NULL)
        create_thread_local_profiling_instance();
      thread_local_profiling_instance->register_index_space_point(
                                                ispace_point_desc);
    }
    //-------------------------------------------------------------------------
    void LegionProfiler::record_empty_index_space(IDType handle)
    //--------------------------------------------------------------------------
    {
      if (thread_local_profiling_instance == NULL)
        create_thread_local_profiling_instance();
      thread_local_profiling_instance->register_empty_index_space(handle);
    }

    //--------------------------------------------------------------------------
    void LegionProfiler::record_field(UniqueID unique_id,
				      unsigned field_id,
				      size_t size,
				      const char* name)
    //--------------------------------------------------------------------------
    {
      if (thread_local_profiling_instance == NULL)
        create_thread_local_profiling_instance();
      thread_local_profiling_instance->register_field(unique_id,field_id,
						      size, name);
    }

    //--------------------------------------------------------------------------
    void LegionProfiler::record_field_space(UniqueID unique_id,
					    const char* name)
    //--------------------------------------------------------------------------
    {
      if (thread_local_profiling_instance == NULL)
        create_thread_local_profiling_instance();
      thread_local_profiling_instance->register_field_space(unique_id,name);
    }


   //--------------------------------------------------------------------------
    void LegionProfiler::record_index_part(UniqueID unique_id,
                                           const char* name)
    //--------------------------------------------------------------------------
    {
      if (thread_local_profiling_instance == NULL)
        create_thread_local_profiling_instance();
      thread_local_profiling_instance->register_index_part(unique_id,name);
    }


    //--------------------------------------------------------------------------
    void LegionProfiler::record_index_space(UniqueID unique_id,
					    const char* name)
    //--------------------------------------------------------------------------
    {
      if (thread_local_profiling_instance == NULL)
        create_thread_local_profiling_instance();
      thread_local_profiling_instance->register_index_space(unique_id,name);
    }

    //--------------------------------------------------------------------------
    void LegionProfiler::record_index_subspace(IDType parent_id, 
                                     IDType unique_id, const DomainPoint &point)
    //--------------------------------------------------------------------------
    {
      if (thread_local_profiling_instance == NULL)
        create_thread_local_profiling_instance();
      thread_local_profiling_instance->register_index_subspace(parent_id, 
                                                              unique_id, point);
    }

    //--------------------------------------------------------------------------
    void LegionProfiler::record_index_partition(IDType parent_id,
                                                IDType unique_id, bool disjoint,
                                                LegionColor point)
    //--------------------------------------------------------------------------
    {

      if (thread_local_profiling_instance == NULL)
        create_thread_local_profiling_instance();
      thread_local_profiling_instance->register_index_partition(parent_id, 
                                              unique_id, disjoint, point);
    }

    //--------------------------------------------------------------------------
    void LegionProfiler::record_index_space_size(UniqueID unique_id,
                                                 unsigned long long
                                                 dense_size,
                                                 unsigned long long
                                                 sparse_size,
                                                 bool is_sparse)
    //--------------------------------------------------------------------------
    {
      if (thread_local_profiling_instance == NULL)
        create_thread_local_profiling_instance();

      thread_local_profiling_instance->register_index_space_size(unique_id,
                                                                 dense_size,
                                                                 sparse_size,
                                                                 is_sparse);
    }

    //--------------------------------------------------------------------------
    void LegionProfiler::record_logical_region(IDType index_space,
                       unsigned field_space, unsigned tree_id, const char* name)
    //--------------------------------------------------------------------------
    {
      if (thread_local_profiling_instance == NULL)
        create_thread_local_profiling_instance();

      thread_local_profiling_instance->register_logical_region(index_space,
							       field_space,
							       tree_id, name);
    }

    //--------------------------------------------------------------------------
    void LegionProfiler::record_physical_instance_region(LgEvent unique_event,
							 LogicalRegion handle)
    //--------------------------------------------------------------------------
    {
      if (thread_local_profiling_instance == NULL)
        create_thread_local_profiling_instance();
      thread_local_profiling_instance->register_physical_instance_region(
                                                    unique_event, handle);
    }

    //--------------------------------------------------------------------------
    void LegionProfiler::record_physical_instance_use(LgEvent unique_event,
             UniqueID op_id, unsigned index, const std::vector<FieldID> &fields)
    //--------------------------------------------------------------------------
    {
      if (thread_local_profiling_instance == NULL)
        create_thread_local_profiling_instance();
      thread_local_profiling_instance->register_physical_instance_use(
                                    unique_event, op_id, index, fields);
    }

    //--------------------------------------------------------------------------
    void LegionProfiler::record_physical_instance_layout(
             LgEvent unique_event, FieldSpace fs, const LayoutConstraintSet &lc)
    //--------------------------------------------------------------------------
    {
      // get fields_constraints
      // get_alignment_constraints
      if (thread_local_profiling_instance == NULL)
        create_thread_local_profiling_instance();

      std::map<FieldID, AlignmentConstraint> align_map;
      const std::vector<AlignmentConstraint> &alignment_constraints =
        lc.alignment_constraints;
      for (std::vector<AlignmentConstraint>::const_iterator it =
             alignment_constraints.begin(); it !=
             alignment_constraints.end(); it++) {
        align_map[it->fid] = *it;
      }
      const std::vector<FieldID> &fields = lc.field_constraint.field_set;
      for (std::vector<FieldID>::const_iterator it =
             fields.begin(); it != fields.end(); it++) {
        std::map<FieldID, AlignmentConstraint>::const_iterator align =
          align_map.find(*it);
      bool has_align=false;
      unsigned alignment = 0;
      EqualityKind eqk = LEGION_LT_EK;
      if (align != align_map.end()) {
        has_align = true;
        alignment = align->second.alignment;
        eqk = align->second.eqk;
      }
      thread_local_profiling_instance->register_physical_instance_field(
                                                 unique_event, *it, fs.get_id(),
                                                 alignment, has_align,
                                                 eqk);
      }
      const std::vector<DimensionKind> &dim_ordering_constr =
        lc.ordering_constraint.ordering;
      unsigned dim=0;
      for (std::vector<DimensionKind>::const_iterator it =
             dim_ordering_constr.begin();
           it != dim_ordering_constr.end(); it++) {
        thread_local_profiling_instance->
          register_physical_instance_dim_order(unique_event, dim, *it);
        dim++;
      }
    }

    //--------------------------------------------------------------------------
    void LegionProfiler::register_task_kind(TaskID task_id,
                                          const char *task_name, bool overwrite)
    //--------------------------------------------------------------------------
    {
      if (thread_local_profiling_instance == NULL)
        create_thread_local_profiling_instance();
      thread_local_profiling_instance->register_task_kind(task_id, task_name,
                                                          overwrite);
    }

    //--------------------------------------------------------------------------
    void LegionProfiler::register_task_variant(TaskID task_id,
                                               VariantID variant_id, 
                                               const char *variant_name)
    //--------------------------------------------------------------------------
    {
      if (thread_local_profiling_instance == NULL)
        create_thread_local_profiling_instance();
      thread_local_profiling_instance->register_task_variant(task_id, 
                                                      variant_id, variant_name);
    }

    //--------------------------------------------------------------------------
    void LegionProfiler::register_operation(Operation *op)
    //--------------------------------------------------------------------------
    {
      if (thread_local_profiling_instance == NULL)
        create_thread_local_profiling_instance();
      thread_local_profiling_instance->register_operation(op);
    }

    //--------------------------------------------------------------------------
    void LegionProfiler::register_multi_task(Operation *op, TaskID task_id)
    //--------------------------------------------------------------------------
    {
      if (thread_local_profiling_instance == NULL)
        create_thread_local_profiling_instance();
      thread_local_profiling_instance->register_multi_task(op, task_id);
    }

    //--------------------------------------------------------------------------
    void LegionProfiler::register_slice_owner(UniqueID pid, UniqueID id)
    //--------------------------------------------------------------------------
    {
      if (thread_local_profiling_instance == NULL)
        create_thread_local_profiling_instance();
      thread_local_profiling_instance->register_slice_owner(pid, id);
    }

    //--------------------------------------------------------------------------
    void LegionProfiler::add_task_request(Realm::ProfilingRequestSet &requests,
                      TaskID tid, VariantID vid, UniqueID task_uid, Processor p)
    //--------------------------------------------------------------------------
    {
#ifdef DEBUG_LEGION
      increment_total_outstanding_requests(LEGION_PROF_TASK);
#else
      increment_total_outstanding_requests();
#endif
      ProfilingInfo info(this, LEGION_PROF_TASK); 
      info.id = tid;
      info.extra.id2 = vid;
      info.op_id = task_uid;
      Realm::ProfilingRequest &req = requests.add_request(target_proc,
                LG_LEGION_PROFILING_ID, &info, sizeof(info), LG_MIN_PRIORITY);
      req.add_measurement<
                Realm::ProfilingMeasurements::OperationTimeline>();
      req.add_measurement<
                Realm::ProfilingMeasurements::OperationProcessorUsage>();
      req.add_measurement<
                Realm::ProfilingMeasurements::OperationEventWaits>();
      if (p.kind() == Processor::TOC_PROC)
        req.add_measurement<
          Realm::ProfilingMeasurements::OperationTimelineGPU>();
      req.add_measurement<
                Realm::ProfilingMeasurements::OperationFinishEvent>();
    }

    //--------------------------------------------------------------------------
    void LegionProfiler::add_meta_request(Realm::ProfilingRequestSet &requests,
                                          LgTaskID tid, Operation *op)
    //--------------------------------------------------------------------------
    {
#ifdef DEBUG_LEGION
      increment_total_outstanding_requests(LEGION_PROF_META);
#else
      increment_total_outstanding_requests();
#endif
      ProfilingInfo info(this, LEGION_PROF_META); 
      info.id = tid;
      info.op_id = (op != NULL) ? op->get_unique_op_id() : 0;
      Realm::ProfilingRequest &req = requests.add_request(target_proc,
                LG_LEGION_PROFILING_ID, &info, sizeof(info), LG_MIN_PRIORITY);
      req.add_measurement<
                Realm::ProfilingMeasurements::OperationTimeline>();
      req.add_measurement<
                Realm::ProfilingMeasurements::OperationProcessorUsage>();
      req.add_measurement<
                Realm::ProfilingMeasurements::OperationEventWaits>();
      req.add_measurement<
                Realm::ProfilingMeasurements::OperationFinishEvent>();
    }

    //--------------------------------------------------------------------------
    /*static*/ void LegionProfiler::add_message_request(
     Realm::ProfilingRequestSet &requests,MessageKind k,Processor remote_target)
    //--------------------------------------------------------------------------
    {
      // Don't increment here, we'll increment on the remote side since we
      // that is where we know the profiler is going to handle the results
      ProfilingInfo info(NULL, LEGION_PROF_MESSAGE);
      info.id = LG_MESSAGE_ID + (int)k;
      info.op_id = implicit_provenance;
      Realm::ProfilingRequest &req = requests.add_request(remote_target,
                LG_LEGION_PROFILING_ID, &info, sizeof(info), LG_MIN_PRIORITY);
      req.add_measurement<
                Realm::ProfilingMeasurements::OperationTimeline>();
      req.add_measurement<
                Realm::ProfilingMeasurements::OperationProcessorUsage>();
      req.add_measurement<
                Realm::ProfilingMeasurements::OperationEventWaits>();
      req.add_measurement<
                Realm::ProfilingMeasurements::OperationFinishEvent>();
    }

    //--------------------------------------------------------------------------
    void LegionProfiler::add_copy_request(Realm::ProfilingRequestSet &requests,
                                          InstanceNameClosure *closure,
                                          Operation *op, unsigned count,
                                          CollectiveKind collective)
    //--------------------------------------------------------------------------
    {
#ifdef DEBUG_LEGION
      increment_total_outstanding_requests(LEGION_PROF_COPY, count);
#else
      increment_total_outstanding_requests(count);
#endif
      ProfilingInfo info(this, LEGION_PROF_COPY); 
      info.op_id = (op != NULL) ? op->get_unique_op_id() : 0;
      // Use ID to encode the collective copy kind
      info.id = collective;
      closure->add_reference(count);
      info.extra.closure = closure;
      Realm::ProfilingRequest &req = requests.add_request(target_proc,
                LG_LEGION_PROFILING_ID, &info, sizeof(info), LG_MIN_PRIORITY);
      req.add_measurement<
                Realm::ProfilingMeasurements::OperationTimeline>();
      req.add_measurement<
                Realm::ProfilingMeasurements::OperationMemoryUsage>();
      req.add_measurement<
                Realm::ProfilingMeasurements::OperationCopyInfo>();
      req.add_measurement<
        Realm::ProfilingMeasurements::OperationFinishEvent>();
    }

    //--------------------------------------------------------------------------
    void LegionProfiler::add_fill_request(Realm::ProfilingRequestSet &requests,
                                          InstanceNameClosure *closure,
                                          Operation *op, 
                                          CollectiveKind collective)
    //--------------------------------------------------------------------------
    {
#ifdef DEBUG_LEGION
      increment_total_outstanding_requests(LEGION_PROF_FILL);
#else
      increment_total_outstanding_requests();
#endif
      ProfilingInfo info(this, LEGION_PROF_FILL);
      info.op_id = (op != NULL) ? op->get_unique_op_id() : 0;
      // Use ID to encode the collective copy kind
      info.id = collective;
      closure->add_reference();
      info.extra.closure = closure;
      Realm::ProfilingRequest &req = requests.add_request(target_proc,
                LG_LEGION_PROFILING_ID, &info, sizeof(info), LG_MIN_PRIORITY);
      req.add_measurement<
                Realm::ProfilingMeasurements::OperationTimeline>();
      req.add_measurement<
                Realm::ProfilingMeasurements::OperationMemoryUsage>();
      req.add_measurement<
                Realm::ProfilingMeasurements::OperationCopyInfo>();
      req.add_measurement<
        Realm::ProfilingMeasurements::OperationFinishEvent>();
    }

    //--------------------------------------------------------------------------
    void LegionProfiler::add_inst_request(Realm::ProfilingRequestSet &requests,
                                          Operation *op, LgEvent unique_event)
    //--------------------------------------------------------------------------
    {
#ifdef DEBUG_LEGION
      increment_total_outstanding_requests(LEGION_PROF_INST); 
#else
      increment_total_outstanding_requests();
#endif
      ProfilingInfo info(this, LEGION_PROF_INST); 
      // No ID here
      info.op_id = (op != NULL) ? op->get_unique_op_id() : 0;
      info.id = unique_event.id;
      // Instances use two profiling requests so that we can get MemoryUsage
      // right away - the Timeline doesn't come until we delete the instance
      Realm::ProfilingRequest &req = requests.add_request(target_proc,
                 LG_LEGION_PROFILING_ID, &info, sizeof(info), LG_MIN_PRIORITY);
      req.add_measurement<
                 Realm::ProfilingMeasurements::InstanceMemoryUsage>();
      req.add_measurement<
                 Realm::ProfilingMeasurements::InstanceTimeline>();
    }

    //--------------------------------------------------------------------------
    void LegionProfiler::handle_failed_instance_allocation(void)
    //--------------------------------------------------------------------------
    {
#ifdef DEBUG_LEGION
      decrement_total_outstanding_requests(LEGION_PROF_INST);
#else
      decrement_total_outstanding_requests();
#endif
    }

    //--------------------------------------------------------------------------
    void LegionProfiler::add_partition_request(
                                           Realm::ProfilingRequestSet &requests,
                                           Operation *op, DepPartOpKind part_op)
    //--------------------------------------------------------------------------
    {
#ifdef DEBUG_LEGION
      increment_total_outstanding_requests(LEGION_PROF_PARTITION);
#else
      increment_total_outstanding_requests();
#endif
      ProfilingInfo info(this, LEGION_PROF_PARTITION);
      // Pass the part_op as the ID
      info.id = part_op;
      info.op_id = (op != NULL) ? op->get_unique_op_id() : 0;
      Realm::ProfilingRequest &req = requests.add_request((target_proc.exists())
                        ? target_proc : Processor::get_executing_processor(),
                        LG_LEGION_PROFILING_ID, &info, sizeof(info));
      req.add_measurement<
                  Realm::ProfilingMeasurements::OperationTimeline>();
    }

    //--------------------------------------------------------------------------
    void LegionProfiler::add_task_request(Realm::ProfilingRequestSet &requests,
                                        TaskID tid, VariantID vid, UniqueID uid)
    //--------------------------------------------------------------------------
    {
#ifdef DEBUG_LEGION
      increment_total_outstanding_requests(LEGION_PROF_TASK);
#else
      increment_total_outstanding_requests();
#endif
      ProfilingInfo info(this, LEGION_PROF_TASK); 
      info.id = tid;
      info.extra.id2 = vid;
      info.op_id = uid;
      Realm::ProfilingRequest &req = requests.add_request(target_proc,
                LG_LEGION_PROFILING_ID, &info, sizeof(info), LG_MIN_PRIORITY);
      req.add_measurement<
                Realm::ProfilingMeasurements::OperationTimeline>();
      req.add_measurement<
                Realm::ProfilingMeasurements::OperationProcessorUsage>();
      req.add_measurement<
                Realm::ProfilingMeasurements::OperationEventWaits>();
      req.add_measurement<
                Realm::ProfilingMeasurements::OperationFinishEvent>();
    }

    //--------------------------------------------------------------------------
    void LegionProfiler::add_meta_request(Realm::ProfilingRequestSet &requests,
                                          LgTaskID tid, UniqueID uid)
    //--------------------------------------------------------------------------
    {
#ifdef DEBUG_LEGION
      increment_total_outstanding_requests(LEGION_PROF_META);
#else
      increment_total_outstanding_requests();
#endif
      ProfilingInfo info(this, LEGION_PROF_META); 
      info.id = tid;
      info.op_id = uid;
      Realm::ProfilingRequest &req = requests.add_request(target_proc,
                LG_LEGION_PROFILING_ID, &info, sizeof(info), LG_MIN_PRIORITY);
      req.add_measurement<
                Realm::ProfilingMeasurements::OperationTimeline>();
      req.add_measurement<
                Realm::ProfilingMeasurements::OperationProcessorUsage>();
      req.add_measurement<
                Realm::ProfilingMeasurements::OperationEventWaits>();
      req.add_measurement<
                Realm::ProfilingMeasurements::OperationFinishEvent>();
    }

    //--------------------------------------------------------------------------
    void LegionProfiler::add_copy_request(Realm::ProfilingRequestSet &requests,
                                          InstanceNameClosure *closure,
                                          UniqueID uid, unsigned count,
                                          CollectiveKind collective)
    //--------------------------------------------------------------------------
    {
#ifdef DEBUG_LEGION
      increment_total_outstanding_requests(LEGION_PROF_COPY, count);
#else
      increment_total_outstanding_requests(count);
#endif
      ProfilingInfo info(this, LEGION_PROF_COPY); 
      info.op_id = uid;
      // Use ID to encode the collective copy kind
      info.id = collective;
      closure->add_reference(count);
      info.extra.closure = closure;
      Realm::ProfilingRequest &req = requests.add_request(target_proc,
                LG_LEGION_PROFILING_ID, &info, sizeof(info), LG_MIN_PRIORITY);
      req.add_measurement<
                Realm::ProfilingMeasurements::OperationTimeline>();
      req.add_measurement<
                Realm::ProfilingMeasurements::OperationMemoryUsage>();
      req.add_measurement<
                Realm::ProfilingMeasurements::OperationCopyInfo>();
      req.add_measurement<
        Realm::ProfilingMeasurements::OperationFinishEvent>();
    }

    //--------------------------------------------------------------------------
    void LegionProfiler::add_fill_request(Realm::ProfilingRequestSet &requests,
                                          InstanceNameClosure *closure,
                                          UniqueID uid,
                                          CollectiveKind collective)
    //--------------------------------------------------------------------------
    {
#ifdef DEBUG_LEGION
      increment_total_outstanding_requests(LEGION_PROF_FILL);
#else
      increment_total_outstanding_requests();
#endif
      ProfilingInfo info(this, LEGION_PROF_FILL);
      info.op_id = uid;
      // Use ID to encode the collective copy kind
      info.id = collective;
      closure->add_reference();
      info.extra.closure = closure;
      Realm::ProfilingRequest &req = requests.add_request(target_proc,
                LG_LEGION_PROFILING_ID, &info, sizeof(info), LG_MIN_PRIORITY);
      req.add_measurement<
                Realm::ProfilingMeasurements::OperationTimeline>();
      req.add_measurement<
                Realm::ProfilingMeasurements::OperationMemoryUsage>();
      req.add_measurement<
                Realm::ProfilingMeasurements::OperationCopyInfo>();
      req.add_measurement<
        Realm::ProfilingMeasurements::OperationFinishEvent>();
    }

    //--------------------------------------------------------------------------
    void LegionProfiler::add_inst_request(Realm::ProfilingRequestSet &requests,
                                          UniqueID uid, LgEvent unique_event)
    //--------------------------------------------------------------------------
    {
#ifdef DEBUG_LEGION
      increment_total_outstanding_requests(LEGION_PROF_INST);
#else
      increment_total_outstanding_requests();
#endif
      ProfilingInfo info(this, LEGION_PROF_INST); 
      // No ID here
      info.op_id = uid;
      info.id = unique_event.id;
      // Instances use two profiling requests so that we can get MemoryUsage
      // right away - the Timeline doesn't come until we delete the instance
      Realm::ProfilingRequest &req = requests.add_request(target_proc,
                 LG_LEGION_PROFILING_ID, &info, sizeof(info), LG_MIN_PRIORITY);
      req.add_measurement<
                 Realm::ProfilingMeasurements::InstanceMemoryUsage>();
      req.add_measurement<
                 Realm::ProfilingMeasurements::InstanceTimeline>();
    }

    //--------------------------------------------------------------------------
    void LegionProfiler::add_partition_request(
                                           Realm::ProfilingRequestSet &requests,
                                           UniqueID uid, DepPartOpKind part_op)
    //--------------------------------------------------------------------------
    {
#ifdef DEBUG_LEGION
      increment_total_outstanding_requests(LEGION_PROF_PARTITION);
#else
      increment_total_outstanding_requests();
#endif
      ProfilingInfo info(this, LEGION_PROF_PARTITION);
      // Pass the partition op kind as the ID
      info.id = part_op;
      info.op_id = uid;
      Realm::ProfilingRequest &req = requests.add_request(target_proc,
                  LG_LEGION_PROFILING_ID, &info, sizeof(info), LG_MIN_PRIORITY);
      req.add_measurement<
                  Realm::ProfilingMeasurements::OperationTimeline>();
    }

    //--------------------------------------------------------------------------
    void LegionProfiler::handle_profiling_response(
                                       const ProfilingResponseBase *base,
                                       const Realm::ProfilingResponse &response,
                                       const void *orig, size_t orig_length)
    //--------------------------------------------------------------------------
    {
#ifdef LEGION_PROF_SELF_PROFILE
      long long t_start = Realm::Clock::current_time_in_nanoseconds();
#endif
      if (thread_local_profiling_instance == NULL)
        create_thread_local_profiling_instance();
#ifdef DEBUG_LEGION
      assert(response.user_data_size() == sizeof(ProfilingInfo));
#endif
      const ProfilingInfo *info = (const ProfilingInfo*)response.user_data();
      switch (info->kind)
      {
        case LEGION_PROF_TASK:
          {
            Realm::ProfilingMeasurements::OperationProcessorUsage usage;
            // Check for predication and speculation
            if (response.get_measurement<
                Realm::ProfilingMeasurements::OperationProcessorUsage>(usage)) {
              thread_local_profiling_instance->process_proc_desc(usage.proc);
              thread_local_profiling_instance->process_task(info, 
                                                            response, usage);
            }
            break;
          }
        case LEGION_PROF_META:
          {
            Realm::ProfilingMeasurements::OperationProcessorUsage usage;
            // Check for predication and speculation
            if (response.get_measurement<
                Realm::ProfilingMeasurements::OperationProcessorUsage>(usage)) {
              thread_local_profiling_instance->process_proc_desc(usage.proc);
              thread_local_profiling_instance->process_meta(info, 
                                                            response, usage); 
            }
            break;
          }
        case LEGION_PROF_MESSAGE:
          {
            Realm::ProfilingMeasurements::OperationProcessorUsage usage;
            // Check for predication and speculation
            if (response.get_measurement<
                Realm::ProfilingMeasurements::OperationProcessorUsage>(usage)) {
              thread_local_profiling_instance->process_proc_desc(usage.proc);
              thread_local_profiling_instance->process_message(info, 
                                                               response, usage);
            }
            break;
          }
        case LEGION_PROF_COPY:
          {
            Realm::ProfilingMeasurements::OperationMemoryUsage usage;
            // Check for predication and speculation
            if (response.get_measurement<
                Realm::ProfilingMeasurements::OperationMemoryUsage>(usage)) {
              thread_local_profiling_instance->process_mem_desc(usage.source);
              thread_local_profiling_instance->process_mem_desc(usage.target);
              thread_local_profiling_instance->process_copy(info,
                                                            response, usage);
            }
            break;
          }
        case LEGION_PROF_FILL:
          {
            Realm::ProfilingMeasurements::OperationMemoryUsage usage;
            // Check for predication and speculation
            if (response.get_measurement<
                Realm::ProfilingMeasurements::OperationMemoryUsage>(usage)) {
              thread_local_profiling_instance->process_mem_desc(usage.target);
              thread_local_profiling_instance->process_fill(info, 
                                                            response, usage);
            }
            break;
          }
        case LEGION_PROF_INST:
          {
	    // Record data based on which measurements we got back this time
            Realm::ProfilingMeasurements::InstanceTimeline timeline;
            Realm::ProfilingMeasurements::InstanceMemoryUsage usage;
	    if (response.get_measurement<
                    Realm::ProfilingMeasurements::InstanceTimeline>(timeline) &&
                response.get_measurement<
                    Realm::ProfilingMeasurements::InstanceMemoryUsage>(usage))
            {
              thread_local_profiling_instance->process_mem_desc(usage.memory);
	      thread_local_profiling_instance->process_inst_timeline(info,
                                                      response, usage, timeline);
            }
            break;
          }
        case LEGION_PROF_PARTITION:
          {
            thread_local_profiling_instance->process_partition(info, response);
            break;
          }
        default:
          assert(false);
      }
#ifdef LEGION_PROF_SELF_PROFILE
      long long t_stop = Realm::Clock::current_time_in_nanoseconds();
      const Processor p = Realm::Processor::get_executing_processor();
      const LgEvent finish_event(Processor::get_current_finish_event());
      thread_local_profiling_instance->process_proc_desc(p);
      thread_local_profiling_instance->record_proftask(p, info->op_id, 
                                       t_start, t_stop, finish_event);
#endif
#ifdef DEBUG_LEGION
      decrement_total_outstanding_requests(info->kind);
#else
      decrement_total_outstanding_requests();
#endif
    }

    //--------------------------------------------------------------------------
    void LegionProfiler::finalize(void)
    //--------------------------------------------------------------------------
    {
      // Remove our guard outstanding request
#ifdef DEBUG_LEGION
      decrement_total_outstanding_requests(LEGION_PROF_META);
#else
      decrement_total_outstanding_requests();
#endif
      if (!done_event.has_triggered())
        done_event.wait();
      for (std::vector<LegionProfInstance*>::const_iterator it = 
            instances.begin(); it != instances.end(); it++) {
        (*it)->dump_state(serializer);
      }  
    }

    //--------------------------------------------------------------------------
    void LegionProfiler::record_mapper_call_kinds(const char *const *const
                               mapper_call_names, unsigned int num_mapper_calls)
    //--------------------------------------------------------------------------
    {
      for (unsigned idx = 0; idx < num_mapper_calls; idx++)
      {
        LegionProfDesc::MapperCallDesc mapper_call_desc;
        mapper_call_desc.kind = idx;
        mapper_call_desc.name = mapper_call_names[idx];
        serializer->serialize(mapper_call_desc);
      }
    }

    //--------------------------------------------------------------------------
    void LegionProfiler::record_mapper_call(MappingCallKind kind, UniqueID uid,
                              unsigned long long start, unsigned long long stop)
    //--------------------------------------------------------------------------
    {
      LgEvent finish_event;
      Processor current = Processor::get_executing_processor();
      if (!current.exists())
      {
        // Ignore mapper calls that happen from outside threads
        if (implicit_context->owner_task == NULL)
          return;
        // Implicit top-level task case where we're not actually running
        // on a Realm processor so we need to get the proxy processor
        // for the context instead
#ifdef DEBUG_LEGION
        assert(implicit_context != NULL);
#endif
        current = implicit_context->get_executing_processor();
        
        TaskContext *ctx = implicit_context;
        finish_event = ctx->owner_task->get_completion_event();
      }
      else
        finish_event = LgEvent(Processor::get_current_finish_event());
      if (thread_local_profiling_instance == NULL)
        create_thread_local_profiling_instance();
      thread_local_profiling_instance->process_proc_desc(current);
      thread_local_profiling_instance->record_mapper_call(current, kind, uid, 
                                                   start, stop, finish_event);
    }

    //--------------------------------------------------------------------------
    void LegionProfiler::record_runtime_call_kinds(const char *const *const
                             runtime_call_names, unsigned int num_runtime_calls)
    //--------------------------------------------------------------------------
    {
      for (unsigned idx = 0; idx < num_runtime_calls; idx++)
      {
        LegionProfDesc::RuntimeCallDesc runtime_call_desc;
        runtime_call_desc.kind = idx;
        runtime_call_desc.name = runtime_call_names[idx];
        serializer->serialize(runtime_call_desc);
      }
    }

    //--------------------------------------------------------------------------
    void LegionProfiler::record_runtime_call(RuntimeCallKind kind,
                              unsigned long long start, unsigned long long stop)
    //--------------------------------------------------------------------------
    {
      LgEvent finish_event;
      Processor current = Processor::get_executing_processor();
      if (!current.exists())
      {
        // Ignore runtime calls that happen from outside threads
        if (implicit_context->owner_task == NULL)
          return;
        // Implicit top-level task case where we're not actually running
        // on a Realm processor so we need to get the proxy processor
        // for the context instead
#ifdef DEBUG_LEGION
        assert(implicit_context != NULL);
#endif
        current = implicit_context->get_executing_processor();
        finish_event = implicit_context->owner_task->get_completion_event();
      }
      else
        finish_event = LgEvent(Processor::get_current_finish_event());
      if (thread_local_profiling_instance == NULL)
        create_thread_local_profiling_instance();
      thread_local_profiling_instance->process_proc_desc(current);
      thread_local_profiling_instance->record_runtime_call(current, kind, start,
                                                           stop, finish_event);
    }

    //--------------------------------------------------------------------------
    void LegionProfiler::record_implicit(UniqueID op_id, TaskID tid, 
        Processor proc, long long start, long long stop,
        const std::vector<std::pair<long long,long long> > &waits,
        LgEvent finish_event)
    //--------------------------------------------------------------------------
    {
      if (thread_local_profiling_instance == NULL)
        create_thread_local_profiling_instance();
      thread_local_profiling_instance->process_proc_desc(proc);
      thread_local_profiling_instance->process_implicit(op_id, tid, proc, start,
                                                     stop, waits, finish_event);
    }

#ifdef DEBUG_LEGION
    //--------------------------------------------------------------------------
    void LegionProfiler::increment_total_outstanding_requests(
                                               ProfilingKind kind, unsigned cnt)
    //--------------------------------------------------------------------------
    {
      AutoLock p_lock(profiler_lock);
      total_outstanding_requests[kind] += cnt;
    }

    //--------------------------------------------------------------------------
    void LegionProfiler::decrement_total_outstanding_requests(
                                               ProfilingKind kind, unsigned cnt)
    //--------------------------------------------------------------------------
    {
      AutoLock p_lock(profiler_lock);
      assert(total_outstanding_requests[kind] >= cnt);
      total_outstanding_requests[kind] -= cnt;
      if (total_outstanding_requests[kind] > 0)
        return;
      for (unsigned idx = 0; idx < LEGION_PROF_LAST; idx++)
      {
        if (idx == kind)
          continue;
        if (total_outstanding_requests[idx] > 0)
          return;
      }
      assert(!done_event.has_triggered());
      Runtime::trigger_event(done_event);
    }
#else
    //--------------------------------------------------------------------------
    void LegionProfiler::increment_total_outstanding_requests(unsigned cnt)
    //--------------------------------------------------------------------------
    {
      total_outstanding_requests.fetch_add(cnt);
    }

    //--------------------------------------------------------------------------
    void LegionProfiler::decrement_total_outstanding_requests(unsigned cnt)
    //--------------------------------------------------------------------------
    {
      unsigned prev = total_outstanding_requests.fetch_sub(cnt);
#ifdef DEBUG_LEGION
      assert(prev >= cnt);
#endif
      // If we were the last outstanding event we can trigger the event
      if (prev == cnt)
      {
#ifdef DEBUG_LEGION
        assert(!done_event.has_triggered());
#endif
        Runtime::trigger_event(done_event);
      }
    }
#endif

    //--------------------------------------------------------------------------
    void LegionProfiler::update_footprint(size_t diff, LegionProfInstance *inst)
    //--------------------------------------------------------------------------
    {
      size_t footprint = total_memory_footprint.fetch_add(diff) + diff;
      if (footprint > output_footprint_threshold)
      {
        // An important bit of logic here, if we're over the threshold then
        // we want to have a little bit of a feedback loop so the more over
        // the limit we are then the more time we give the profiler to dump
        // out things to the output file. We'll try to make this continuous
        // so there are no discontinuities in performance. If the threshold
        // is zero we'll just choose an arbitrarily large scale factor to 
        // ensure that things work properly.
        double over_scale = output_footprint_threshold == 0 ? double(1 << 20) :
                        double(footprint) / double(output_footprint_threshold);
        // Let's actually make this quadratic so it's not just linear
        if (output_footprint_threshold > 0)
          over_scale *= over_scale;
        if (!serializer->is_thread_safe())
        {
          // Need a lock to protect the serializer
          AutoLock p_lock(profiler_lock);
          diff = inst->dump_inter(serializer, over_scale);
        }
        else
          diff = inst->dump_inter(serializer, over_scale);
#ifdef DEBUG_LEGION
#ifndef NDEBUG
        footprint = 
#endif
#endif
          total_memory_footprint.fetch_sub(diff);
#ifdef DEBUG_LEGION
        assert(footprint >= diff); // check for wrap-around
#endif
      }
    }

    //--------------------------------------------------------------------------
    void LegionProfiler::issue_default_mapper_warning(Operation *op,
                                                   const char *mapper_call_name)
    //--------------------------------------------------------------------------
    {
      // We'll skip any warnings for now with no operation
      if (op == NULL)
        return;
      // We'll only issue this warning once on each node for now
      if (!need_default_mapper_warning.exchange(false/*no longer needed*/))
        return;
      // Give a massive warning for profilig when using the default mapper
      for (int i = 0; i < 2; i++)
        fprintf(stderr,"!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
      for (int i = 0; i < 4; i++)
        fprintf(stderr,"!WARNING WARNING WARNING WARNING WARNING WARNING!\n");
      for (int i = 0; i < 2; i++)
        fprintf(stderr,"!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
      fprintf(stderr,"!!! YOU ARE PROFILING USING THE DEFAULT MAPPER!!!\n");
      fprintf(stderr,"!!! THE DEFAULT MAPPER IS NOT FOR PERFORMANCE !!!\n");
      fprintf(stderr,"!!! PLEASE CUSTOMIZE YOUR MAPPER TO YOUR      !!!\n");
      fprintf(stderr,"!!! APPLICATION AND TO YOUR TARGET MACHINE    !!!\n");
      InnerContext *context = op->get_context();
      if (op->get_operation_kind() == Operation::TASK_OP_KIND)
      {
        TaskOp *task = static_cast<TaskOp*>(op);
        if (context->get_owner_task() != NULL) 
          fprintf(stderr,"First use of the default mapper in address space %d\n"
                         "occurred when task %s (UID %lld) in parent task %s "
                         "(UID %lld)\ninvoked the \"%s\" mapper call\n",
                         runtime->address_space, task->get_task_name(),
                         task->get_unique_op_id(), context->get_task_name(),
                         context->get_unique_id(), mapper_call_name);
        else
          fprintf(stderr,"First use of the default mapper in address space %d\n"
                         "occurred when task %s (UID %lld) invoked the \"%s\" "
                         "mapper call\n", runtime->address_space,
                         task->get_task_name(), task->get_unique_op_id(),
                         mapper_call_name);
      }
      else
        fprintf(stderr,"First use of the default mapper in address space %d\n"
                       "occurred when %s (UID %lld) in parent task %s "
                       "(UID %lld)\ninvoked the \"%s\" mapper call\n",
                       runtime->address_space, op->get_logging_name(),
                       op->get_unique_op_id(), context->get_task_name(),
                       context->get_unique_id(), mapper_call_name);
      for (int i = 0; i < 2; i++)
        fprintf(stderr,"!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
      for (int i = 0; i < 4; i++)
        fprintf(stderr,"!WARNING WARNING WARNING WARNING WARNING WARNING!\n");
      for (int i = 0; i < 2; i++)
        fprintf(stderr,"!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
      fprintf(stderr,"\n");
      fflush(stderr);
    }

    //--------------------------------------------------------------------------
    void LegionProfiler::create_thread_local_profiling_instance(void)
    //--------------------------------------------------------------------------
    {
      thread_local_profiling_instance = new LegionProfInstance(this);
      // Task the lock and save the reference
      AutoLock p_lock(profiler_lock);
      instances.push_back(thread_local_profiling_instance);
    }

    //--------------------------------------------------------------------------
    DetailedProfiler::DetailedProfiler(Runtime *runtime, RuntimeCallKind call)
      : profiler(runtime->profiler), call_kind(call), start_time(0)
    //--------------------------------------------------------------------------
    {
      if (profiler != NULL)
        start_time = Realm::Clock::current_time_in_nanoseconds();
    }

    //--------------------------------------------------------------------------
    DetailedProfiler::DetailedProfiler(const DetailedProfiler &rhs)
      : profiler(rhs.profiler), call_kind(rhs.call_kind)
    //--------------------------------------------------------------------------
    {
      // should never be called
      assert(false);
    }

    //--------------------------------------------------------------------------
    DetailedProfiler::~DetailedProfiler(void)
    //--------------------------------------------------------------------------
    {
      if (profiler != NULL)
      {
        unsigned long long stop_time = 
          Realm::Clock::current_time_in_nanoseconds();
        profiler->record_runtime_call(call_kind, start_time, stop_time);
      }
    }

    //--------------------------------------------------------------------------
    DetailedProfiler& DetailedProfiler::operator=(const DetailedProfiler &rhs)
    //--------------------------------------------------------------------------
    {
      // should never be called
      assert(false);
      return *this;
    }

  }; // namespace Internal
}; // namespace Legion

