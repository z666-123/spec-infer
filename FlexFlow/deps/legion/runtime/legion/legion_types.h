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

#ifndef __LEGION_TYPES_H__
#define __LEGION_TYPES_H__

/**
 * \file legion_types.h
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdint.h>
#include <limits.h>

#include <map>
#include <set>
#include <list>
#include <deque>
#include <vector>
#include <typeinfo>
#include <type_traits>
#include <unordered_set>
#include <unordered_map>

#include "legion/legion_config.h"
#include "legion/legion_template_help.h"

// Make sure we have the appropriate defines in place for including realm
#include "realm.h"
#include "realm/dynamic_templates.h"

// this may be set before including legion.h to eliminate deprecation warnings
//  for just the Legion API
#ifndef LEGION_DEPRECATED
#if __cplusplus >= 201402L
#define LEGION_DEPRECATED(x) [[deprecated(x)]]
#else
#define LEGION_DEPRECATED(x)
#endif
#endif

// Macros for disabling and re-enabling deprecated warnings
#if defined(__GNUC__)
#define LEGION_DISABLE_DEPRECATED_WARNINGS \
  _Pragma("GCC diagnostic push") \
  _Pragma("GCC diagnostic ignored \"-Wdeprecated-declarations\"")
#define LEGION_REENABLE_DEPRECATED_WARNINGS \
  _Pragma("GCC diagnostic pop")
#elif defined(__clang__)
#define LEGION_DISABLE_DEPRECATED_WARNINGS \
  _Pragma("clang diagnostic push") \
  _Pragma("clang diagnostic ignored \"-Wdeprecated-declarations\"")
#define LEGION_REENABLE_DEPRECATED_WARNINGS \
  _Pragma("clang diagnostic pop")
#elif defined(__PGIC__)
#define LEGION_DISABLE_DEPRECATED_WARNINGS \
  _Pragma("warning (push)") \
  _Pragma("diag_suppress 1445")
#define LEGION_REENABLE_DEPRECATED_WARNINGS \
  _Pragma("warning (pop)")
#elif defined(__INTEL_COMPILER) || defined(__INTEL_LLVM_COMPILER)
#define LEGION_DISABLE_DEPRECATED_WARNINGS \
  _Pragma("warning push") \
  _Pragma("warning disable 1478")
#define LEGION_REENABLE_DEPRECATED_WARNINGS \
  _Pragma("warning pop")
#else
#warning "Don't know how to suppress deprecated warnings for this compiler"
#define LEGION_DISABLE_DEPRECATED_WARNINGS
#define LEGION_REENABLE_DEPRECATED_WARNINGS
#endif

// If we're doing full LEGION_SPY then turn off event pruning
#ifdef LEGION_SPY
#ifndef LEGION_DISABLE_EVENT_PRUNING
#define LEGION_DISABLE_EVENT_PRUNING
#endif
#endif

// forward declarations from bitmask.h
template<typename T, unsigned int MAX,
         unsigned SHIFT, unsigned MASK> class BitMask;
template<typename T, unsigned int MAX,
         unsigned SHIFT, unsigned MASK> class TLBitMask;
#ifdef __SSE2__
template<unsigned int MAX> class SSEBitMask;
template<unsigned int MAX> class SSETLBitMask;
#endif
#ifdef __AVX__
template<unsigned int MAX> class AVXBitMask;
template<unsigned int MAX> class AVXTLBitMask;
#endif
#ifdef __ALTIVEC__
template<unsigned int MAX> class PPCBitMask;
template<unsigned int MAX> class PPCTLBitMask;
#endif
#ifdef __ARM_NEON
template<unsigned int MAX> class NeonBitMask;
template<unsigned int MAX> class NeonTLBitMask;
#endif
template<typename DT, unsigned BLOAT, bool BIDIR> class CompoundBitMask;

namespace BindingLib { class Utility; } // BindingLib namespace

namespace Legion { 

  typedef ::legion_error_t LegionErrorType;
  typedef ::legion_privilege_mode_t PrivilegeMode;
  typedef ::legion_allocate_mode_t AllocateMode;
  typedef ::legion_coherence_property_t CoherenceProperty;
  typedef ::legion_region_flags_t RegionFlags;
  typedef ::legion_projection_type_t ProjectionType;
  typedef ::legion_partition_kind_t PartitionKind;
  typedef ::legion_external_resource_t ExternalResource;
  typedef ::legion_timing_measurement_t TimingMeasurement;
  typedef ::legion_dependence_type_t DependenceType;
  typedef ::legion_mappable_type_id_t MappableType;
  typedef ::legion_file_mode_t LegionFileMode;
  typedef ::legion_execution_constraint_t ExecutionConstraintKind;
  typedef ::legion_layout_constraint_t LayoutConstraintKind;
  typedef ::legion_equality_kind_t EqualityKind;
  typedef ::legion_dimension_kind_t DimensionKind;
  typedef ::legion_isa_kind_t ISAKind;
  typedef ::legion_resource_constraint_t ResourceKind;
  typedef ::legion_launch_constraint_t LaunchKind;
  typedef ::legion_specialized_constraint_t SpecializedKind;

  // Forward declarations for user level objects
  // legion.h
  class IndexSpace;
  template<int DIM, typename T> class IndexSpaceT;
  class IndexPartition;
  template<int DIM, typename T> class IndexPartitionT;
  class FieldSpace;
  class LogicalRegion;
  template<int DIM, typename T> class LogicalRegionT;
  class LogicalPartition;
  template<int DIM, typename T> class LogicalPartitionT;
  class IndexAllocator;
  class FieldAllocator;
  class UntypedBuffer;
  class ArgumentMap;
  class Lock;
  struct LockRequest;
  class Grant;
  class PhaseBarrier;
  struct RegionRequirement;
  struct OutputRequirement;
  struct IndexSpaceRequirement;
  struct FieldSpaceRequirement;
  struct TaskLauncher;
  struct IndexTaskLauncher;
  typedef IndexTaskLauncher IndexLauncher; // for backwards compatibility
  struct InlineLauncher;
  struct CopyLauncher;
  struct AcquireLauncher;
  struct ReleaseLauncher;
  struct FillLauncher;
  struct LayoutConstraintRegistrar;
  struct TaskVariantRegistrar;
  class Future;
  class FutureMap;
  class Predicate;
  class PhysicalRegion;
  class OutputRegion;
  class ExternalResources;
  class UntypedDeferredValue;
  template<typename>
    class DeferredValue;
  template<typename, bool>
    class DeferredReduction;
  template<typename, int, typename, bool>
    class DeferredBuffer;
  template<typename COORD_T>
    class UntypedDeferredBuffer;
  template<PrivilegeMode,typename,int,typename,typename,bool> 
    class FieldAccessor;
  template<typename, bool, int, typename, typename, bool>
    class ReductionAccessor;
  template<typename, int, typename, typename, bool>
    class PaddingAccessor;
#ifdef LEGION_MULTI_REGION_ACCESSOR
  template<typename, int,typename,typename,bool,bool,int>
    class MultiRegionAccessor;
#endif
  template<typename,int,typename,typename>
    class UnsafeFieldAccessor;
  namespace ArraySyntax {
    template<typename, PrivilegeMode> class AccessorRefHelper;
    template<typename> class AffineRefHelper;
  }
  class PieceIterator;
  template<int,typename>
  class PieceIteratorT;
  template<PrivilegeMode,typename,int,typename>
  class SpanIterator;
  class IndexIterator;
  template<typename T> struct ColoredPoints; 
  struct InputArgs;
  struct RegistrationCallbackArgs;
  class ProjectionFunctor;
  class ShardingFunctor;
  class Task;
  class Copy;
  class InlineMapping;
  class Acquire;
  class Release;
  class Close;
  class Fill;
  class Partition;
  class MustEpoch;
  class PointTransformFunctor;
  class Runtime;
  class LegionHandshake;
  class MPILegionHandshake;
  // For backwards compatibility
  typedef Runtime HighLevelRuntime;
  // Helper for saving instantiated template functions
  struct SerdezRedopFns;
  // Some typedefs for making things nicer for users with C++11 support
  template<typename FT, int N, typename T = ::legion_coord_t>
  using GenericAccessor = Realm::GenericAccessor<FT,N,T>;
  template<typename FT, int N, typename T = ::legion_coord_t>
  using AffineAccessor = Realm::AffineAccessor<FT,N,T>;
  template<typename FT, int N, typename T = ::legion_coord_t>
  using MultiAffineAccessor = Realm::MultiAffineAccessor<FT,N,T>;

  // Forward declarations for compiler level objects
  // legion.h
  class ColoringSerializer;
  class DomainColoringSerializer;

  // Forward declarations for wrapper tasks
  // legion.h
  class LegionTaskWrapper;
  class LegionSerialization;

  // Forward declarations for C wrapper objects
  // legion_c_util.h
  class TaskResult;
  class CObjectWrapper;

  // legion_domain.h
  class DomainPoint;
  class Domain;
  class IndexSpaceAllocator; 

  // legion_utilities.h
  class Serializer;
  class Deserializer;

  // legion_constraint.h
  class ISAConstraint;
  class ProcessorConstraint;
  class ResourceConstraint;
  class LaunchConstraint;
  class ColocationConstraint;
  class ExecutionConstraintSet;

  class SpecializedConstraint;
  class MemoryConstraint;
  class FieldConstraint;
  class PaddingConstraint;
  class OrderingConstraint;
  class TilingConstraint;
  class DimensionConstraint;
  class AlignmentConstraint;
  class OffsetConstraint;
  class PointerConstraint;
  class LayoutConstraintSet;
  class TaskLayoutConstraintSet;

  namespace Mapping {
    class PhysicalInstance;
    class CollectiveView;
    class MapperEvent;
    class ProfilingRequestSet;
    class Mapper;
    class MapperRuntime;
    class AutoLock;
    class DefaultMapper;
    class ShimMapper;
    class TestMapper;
    class DebugMapper;
    class ReplayMapper;

    // The following types are effectively overlaid on the Realm versions
    // to allow for Legion-specific profiling measurements
    enum ProfilingMeasurementID {
      PMID_LEGION_FIRST = Realm::PMID_REALM_LAST,
      PMID_RUNTIME_OVERHEAD,
    };
  };
  
  namespace Internal { 

    enum OpenState {
      NOT_OPEN                = 0,
      OPEN_READ_ONLY          = 1,
      OPEN_READ_WRITE         = 2, // unknown dirty information below
      OPEN_REDUCE             = 3, // make sure to check reduction value
    }; 

    // Internal reduction operators
    // Currently we don't use any, but 0 is reserved
    enum {
      REDOP_ID_AVAILABLE    = 1,
    }; 

    // Realm dependent partitioning kinds
    enum DepPartOpKind {
      DEP_PART_UNION = 0, // a single union
      DEP_PART_UNIONS = 1, // many parallel unions
      DEP_PART_UNION_REDUCTION = 2, // union reduction to a single space
      DEP_PART_INTERSECTION = 3, // a single intersection
      DEP_PART_INTERSECTIONS = 4, // many parallel intersections
      DEP_PART_INTERSECTION_REDUCTION = 5, // intersection reduction to a space
      DEP_PART_DIFFERENCE = 6, // a single difference
      DEP_PART_DIFFERENCES = 7, // many parallel differences
      DEP_PART_EQUAL = 8, // an equal partition operation
      DEP_PART_BY_FIELD = 9, // create a partition from a field
      DEP_PART_BY_IMAGE = 10, // create partition by image
      DEP_PART_BY_IMAGE_RANGE = 11, // create partition by image range
      DEP_PART_BY_PREIMAGE = 12, // create partition by preimage
      DEP_PART_BY_PREIMAGE_RANGE = 13, // create partition by preimage range
      DEP_PART_ASSOCIATION = 14, // create an association
      DEP_PART_WEIGHTS = 15, // create partition by weights
    };

    // Collective copy kinds
    enum CollectiveKind {
      COLLECTIVE_NONE = 0,
      // Filling a collective instance (both normal and reductions)
      COLLECTIVE_FILL = 1,
      // Broadcasting one normal instance to a collective normal instance
      COLLECTIVE_BROADCAST = 2,
      // Reducing a collective reduction instance to either a
      // single normal or a single reduction instance
      COLLECTIVE_REDUCTION = 3,
      // Performing an all-reduce from a collective reduction instance
      // to a collective normal or reduction instance using a butterfly
      // network reduction (both instances using the same nodes)
      COLLECTIVE_BUTTERFLY_ALLREDUCE = 4,
      // Performing an all-reduce by doing a reduction down to a single
      // instance and then broadcasting the result out from that instance
      // (instances don't exist on the same set of nodes)
      COLLECTIVE_HOURGLASS_ALLREDUCE = 5,
      // Copy from one collective normal instanace to another collective
      // normal instance for each of the points in the destination
      COLLECTIVE_POINT_TO_POINT = 6,
      // Apply a reduction from a single reduction instance to 
      // a collective normal instance
      COLLECTIVE_REDUCECAST = 7,
      // Degenerate case: apply a copy-across from a collective reduction
      // instance to any kind of other instance without doing an all-reduce
      COLLECTIVE_HAMMER_REDUCTION = 8,
    };

    // Enumeration of Legion runtime tasks
    enum LgTaskID {
      LG_SCHEDULER_ID,
      LG_POST_END_ID,
      LG_TRIGGER_READY_ID,
      LG_TRIGGER_EXECUTION_ID,
      LG_TRIGGER_RESOLUTION_ID,
      LG_TRIGGER_COMMIT_ID,
      LG_DEFERRED_EXECUTION_ID,
      LG_DEFERRED_COMPLETION_ID,
      LG_DEFERRED_COMMIT_ID,
      LG_PRE_PIPELINE_ID,
      LG_TRIGGER_DEPENDENCE_ID,
      LG_TRIGGER_COMPLETION_ID,
      LG_TRIGGER_OP_ID,
      LG_TRIGGER_TASK_ID,
      LG_DEFER_MAPPER_SCHEDULER_TASK_ID,
      LG_MUST_INDIV_ID,
      LG_MUST_INDEX_ID,
      LG_MUST_MAP_ID,
      LG_MUST_DIST_ID,
      LG_MUST_LAUNCH_ID,
      LG_CONTRIBUTE_COLLECTIVE_ID,
      LG_FUTURE_CALLBACK_TASK_ID,
      LG_CALLBACK_RELEASE_TASK_ID,
      LG_FUTURE_BROADCAST_TASK_ID,
      LG_TOP_FINISH_TASK_ID,
      LG_MAPPER_TASK_ID,
      LG_DISJOINTNESS_TASK_ID,
      LG_ISSUE_FRAME_TASK_ID,
      LG_TASK_IMPL_SEMANTIC_INFO_REQ_TASK_ID,
      LG_INDEX_SPACE_SEMANTIC_INFO_REQ_TASK_ID,
      LG_INDEX_PART_SEMANTIC_INFO_REQ_TASK_ID,
      LG_FIELD_SPACE_SEMANTIC_INFO_REQ_TASK_ID,
      LG_FIELD_SEMANTIC_INFO_REQ_TASK_ID,
      LG_DEFER_FIELD_INFOS_TASK_ID,
      LG_REGION_SEMANTIC_INFO_REQ_TASK_ID,
      LG_PARTITION_SEMANTIC_INFO_REQ_TASK_ID,
      LG_INDEX_SPACE_DEFER_CHILD_TASK_ID,
      LG_INDEX_PART_DEFER_CHILD_TASK_ID,
      LG_INDEX_PART_DEFER_SHARD_RECTS_TASK_ID,
      LG_DEFERRED_ENQUEUE_TASK_ID,
      LG_DEFER_MAPPER_MESSAGE_TASK_ID,
      LG_REMOTE_VIEW_CREATION_TASK_ID,
      LG_DEFERRED_DISTRIBUTE_TASK_ID,
      LG_DEFER_PERFORM_MAPPING_TASK_ID,
      LG_FINALIZE_OUTPUT_TREE_TASK_ID,
      LG_DEFERRED_LAUNCH_TASK_ID,
      LG_MISPREDICATION_TASK_ID,
      LG_DEFER_TRIGGER_TASK_COMPLETE_TASK_ID,
      LG_DEFER_MATERIALIZED_VIEW_TASK_ID,
      LG_DEFER_REDUCTION_VIEW_TASK_ID,
      LG_DEFER_PHI_VIEW_REGISTRATION_TASK_ID,
      LG_DEFER_COMPOSITE_COPY_TASK_ID,
      LG_TIGHTEN_INDEX_SPACE_TASK_ID,
      LG_REPLAY_SLICE_TASK_ID,
      LG_TRANSITIVE_REDUCTION_TASK_ID,
      LG_DELETE_TEMPLATE_TASK_ID,
      LG_DEFER_MAKE_OWNER_TASK_ID,
      LG_DEFER_APPLY_STATE_TASK_ID,
      LG_COPY_FILL_AGGREGATION_TASK_ID,
      LG_COPY_FILL_DELETION_TASK_ID,
      LG_FINALIZE_EQ_SETS_TASK_ID,
      LG_FINALIZE_OUTPUT_EQ_SET_TASK_ID,
      LG_DEFERRED_COPY_ACROSS_TASK_ID,
      LG_DEFER_REMOTE_OP_DELETION_TASK_ID,
      LG_DEFER_REMOTE_INSTANCE_TASK_ID,
      LG_DEFER_REMOTE_REDUCTION_TASK_ID,
      LG_DEFER_REMOTE_UPDATE_TASK_ID,
      LG_DEFER_REMOTE_ACQUIRE_TASK_ID,
      LG_DEFER_REMOTE_RELEASE_TASK_ID,
      LG_DEFER_REMOTE_COPIES_ACROSS_TASK_ID,
      LG_DEFER_REMOTE_OVERWRITE_TASK_ID,
      LG_DEFER_REMOTE_FILTER_TASK_ID,
      LG_DEFER_PERFORM_TRAVERSAL_TASK_ID,
      LG_DEFER_PERFORM_ANALYSIS_TASK_ID,
      LG_DEFER_PERFORM_REMOTE_TASK_ID,
      LG_DEFER_PERFORM_UPDATE_TASK_ID,
      LG_DEFER_PERFORM_REGISTRATION_TASK_ID,
      LG_DEFER_PERFORM_OUTPUT_TASK_ID,
      LG_DEFER_PHYSICAL_MANAGER_TASK_ID,
      LG_DEFER_DELETE_PHYSICAL_MANAGER_TASK_ID,
      LG_DEFER_VERIFY_PARTITION_TASK_ID,
      LG_DEFER_RELEASE_ACQUIRED_TASK_ID,
      LG_DEFER_COPY_ACROSS_TASK_ID,
      LG_DEFER_COLLECTIVE_MESSAGE_TASK_ID,
      LG_FREE_EAGER_INSTANCE_TASK_ID,
      LG_MALLOC_INSTANCE_TASK_ID,
      LG_FREE_INSTANCE_TASK_ID,
      LG_DEFER_TRACE_PRECONDITION_TASK_ID,
      LG_DEFER_TRACE_POSTCONDITION_TASK_ID,
      LG_DEFER_TRACE_UPDATE_TASK_ID,
      LG_FINALIZE_OUTPUT_ID,
      LG_DEFER_DELETE_FUTURE_INSTANCE_TASK_ID,
      LG_FREE_EXTERNAL_TASK_ID,
      LG_DEFER_CONCURRENT_ANALYSIS_TASK_ID,
      LG_DEFER_CONSENSUS_MATCH_TASK_ID,
      LG_DEFER_COLLECTIVE_TASK_ID,
      LG_DEFER_RECORD_COMPLETE_REPLAY_TASK_ID,
      LG_DEFER_ISSUE_FILL_TASK_ID,
      LG_DEFER_MUST_EPOCH_RETURN_TASK_ID,
      LG_YIELD_TASK_ID,
      // this marks the beginning of task IDs tracked by the shutdown algorithm
      LG_BEGIN_SHUTDOWN_TASK_IDS,
      LG_RETRY_SHUTDOWN_TASK_ID = LG_BEGIN_SHUTDOWN_TASK_IDS,
      // Message ID goes at the end so we can append additional 
      // message IDs here for the profiler and separate meta-tasks
      LG_MESSAGE_ID,
      LG_LAST_TASK_ID, // This one should always be last
    };  

    // Make this a macro so we can keep it close to 
    // declaration of the task IDs themselves
#define LG_TASK_DESCRIPTIONS(name)                               \
      const char *name[LG_LAST_TASK_ID] = {                      \
        "Scheduler",                                              \
        "Post-Task Execution",                                    \
        "Trigger Ready",                                          \
        "Trigger Execution",                                      \
        "Trigger Resolution",                                     \
        "Trigger Commit",                                         \
        "Deferred Execution",                                     \
        "Deferred Completion",                                    \
        "Deferred Commit",                                        \
        "Prepipeline Stage",                                      \
        "Logical Dependence Analysis",                            \
        "Trigger Completion",                                     \
        "Trigger Operation Mapping",                              \
        "Trigger Task Mapping",                                   \
        "Defer Mapper Scheduler",                                 \
        "Must Individual Task Dependence Analysis",               \
        "Must Index Task Dependence Analysis",                    \
        "Must Task Physical Dependence Analysis",                 \
        "Must Task Distribution",                                 \
        "Must Task Launch",                                       \
        "Contribute Collective",                                  \
        "Future Callback",                                        \
        "Future Callback Release",                                \
        "Future Broadcast",                                       \
        "Top Finish",                                             \
        "Mapper Task",                                            \
        "Disjointness Test",                                      \
        "Issue Frame",                                            \
        "Task Impl Semantic Request",                             \
        "Index Space Semantic Request",                           \
        "Index Partition Semantic Request",                       \
        "Field Space Semantic Request",                           \
        "Field Semantic Request",                                 \
        "Defer Field Infos Request",                              \
        "Region Semantic Request",                                \
        "Partition Semantic Request",                             \
        "Defer Index Space Child Request",                        \
        "Defer Index Partition Child Request",                    \
        "Defer Index Partition Find Shard Rects",                 \
        "Deferred Enqueue Task",                                  \
        "Deferred Mapper Message",                                \
        "Remote View Creation",                                   \
        "Deferred Distribute Task",                               \
        "Defer Task Perform Mapping",                             \
        "Finalize Output Regions Eq KD Tree",                     \
        "Deferred Task Launch",                                   \
        "Handle Mapping Mispredication",                          \
        "Defer Trigger Task Complete",                            \
        "Defer Materialized View Registration",                   \
        "Defer Reduction View Registration",                      \
        "Defer Phi View Registration",                            \
        "Defer Composite Copy",                                   \
        "Tighten Index Space",                                    \
        "Replay Physical Trace",                                  \
        "Template Transitive Reduction",                          \
        "Delete Physical Template",                               \
        "Defer Equivalence Set Make Owner",                       \
        "Defer Equivalence Set Apply State",                      \
        "Copy Fill Aggregation",                                  \
        "Copy Fill Deletion",                                     \
        "Finalize Equivalence Sets",                              \
        "Finalize Output Equivalence Set",                        \
        "Deferred Copy Across",                                   \
        "Defer Remote Op Deletion",                               \
        "Defer Remote Instance Request",                          \
        "Defer Remote Reduction Request",                         \
        "Defer Remote Update Equivalence Set",                    \
        "Defer Remote Acquire",                                   \
        "Defer Remote Release",                                   \
        "Defer Remote Copy Across",                               \
        "Defer Remote Overwrite Equivalence Set",                 \
        "Defer Remote Filter Equivalence Set",                    \
        "Defer Physical Analysis Traversal Stage",                \
        "Defer Physical Analysis Analyze Equivalence Set Stage",  \
        "Defer Physical Analysis Remote Stage",                   \
        "Defer Physical Analysis Update Stage",                   \
        "Defer Physical Analysis Registration Stage",             \
        "Defer Physical Analysis Output Stage",                   \
        "Defer Physical Manager Registration",                    \
        "Defer Physical Manager Deletion",                        \
        "Defer Verify Partition",                                 \
        "Defer Release Acquired Instances",                       \
        "Defer Copy-Across Execution for Preimages",              \
        "Defer Collective Instance Message",                      \
        "Free Eager Instance",                                    \
        "Malloc Instance",                                        \
        "Free Instance",                                          \
        "Defer Trace Precondition Test",                          \
        "Defer Trace Postcondition Test",                         \
        "Defer Trace Update",                                     \
        "Finalize Output Region Instance",                        \
        "Defer Delete Future Instance",                           \
        "Free External Allocation",                               \
        "Defer Concurrent Analysis",                              \
        "Defer Consensus Match",                                  \
        "Defer Collective Async",                                 \
        "Defer Record Complete Replay",                           \
        "Defer Issue Fill",                                       \
        "Defer Must Epoch Return Resources",                      \
        "Yield",                                                  \
        "Retry Shutdown",                                         \
        "Remote Message",                                         \
      };

    enum MappingCallKind {
      GET_MAPPER_NAME_CALL,
      GET_MAPER_SYNC_MODEL_CALL,
      SELECT_TASK_OPTIONS_CALL,
      PREMAP_TASK_CALL,
      SLICE_TASK_CALL,
      MAP_TASK_CALL,
      REPLICATE_TASK_CALL,
      SELECT_VARIANT_CALL,
      POSTMAP_TASK_CALL,
      TASK_SELECT_SOURCES_CALL,
      TASK_SPECULATE_CALL,
      TASK_REPORT_PROFILING_CALL,
      TASK_SELECT_SHARDING_FUNCTOR_CALL,
      MAP_INLINE_CALL,
      INLINE_SELECT_SOURCES_CALL,
      INLINE_REPORT_PROFILING_CALL,
      MAP_COPY_CALL,
      COPY_SELECT_SOURCES_CALL,
      COPY_SPECULATE_CALL,
      COPY_REPORT_PROFILING_CALL,
      COPY_SELECT_SHARDING_FUNCTOR_CALL,
      CLOSE_SELECT_SOURCES_CALL,
      CLOSE_REPORT_PROFILING_CALL,
      CLOSE_SELECT_SHARDING_FUNCTOR_CALL,
      MAP_ACQUIRE_CALL,
      ACQUIRE_SPECULATE_CALL,
      ACQUIRE_REPORT_PROFILING_CALL,
      ACQUIRE_SELECT_SHARDING_FUNCTOR_CALL,
      MAP_RELEASE_CALL,
      RELEASE_SELECT_SOURCES_CALL,
      RELEASE_SPECULATE_CALL,
      RELEASE_REPORT_PROFILING_CALL,
      RELEASE_SELECT_SHARDING_FUNCTOR_CALL,
      SELECT_PARTITION_PROJECTION_CALL,
      MAP_PARTITION_CALL,
      PARTITION_SELECT_SOURCES_CALL,
      PARTITION_REPORT_PROFILING_CALL,
      PARTITION_SELECT_SHARDING_FUNCTOR_CALL,
      FILL_SELECT_SHARDING_FUNCTOR_CALL,
      MAP_FUTURE_MAP_REDUCTION_CALL,
      CONFIGURE_CONTEXT_CALL,
      SELECT_TUNABLE_VALUE_CALL,
      MUST_EPOCH_SELECT_SHARDING_FUNCTOR_CALL,
      MAP_MUST_EPOCH_CALL,
      MAP_DATAFLOW_GRAPH_CALL,
      MEMOIZE_OPERATION_CALL,
      SELECT_TASKS_TO_MAP_CALL,
      SELECT_STEAL_TARGETS_CALL,
      PERMIT_STEAL_REQUEST_CALL,
      HANDLE_MESSAGE_CALL,
      HANDLE_TASK_RESULT_CALL,
      APPLICATION_MAPPER_CALL,
      LAST_MAPPER_CALL,
    };

#define MAPPER_CALL_NAMES(name)                     \
    const char *name[LAST_MAPPER_CALL] = {          \
      "get_mapper_name",                            \
      "get_mapper_sync_model",                      \
      "select_task_options",                        \
      "premap_task",                                \
      "slice_task",                                 \
      "map_task",                                   \
      "replicate_task",                             \
      "select_task_variant",                        \
      "postmap_task",                               \
      "select_task_sources",                        \
      "speculate (for task)",                       \
      "report profiling (for task)",                \
      "select sharding functor (for task)",         \
      "map_inline",                                 \
      "select_inline_sources",                      \
      "report profiling (for inline)",              \
      "map_copy",                                   \
      "select_copy_sources",                        \
      "speculate (for copy)",                       \
      "report_profiling (for copy)",                \
      "select sharding functor (for copy)",         \
      "select_close_sources",                       \
      "report_profiling (for close)",               \
      "select sharding functor (for close)",        \
      "map_acquire",                                \
      "speculate (for acquire)",                    \
      "report_profiling (for acquire)",             \
      "select sharding functor (for acquire)",      \
      "map_release",                                \
      "select_release_sources",                     \
      "speculate (for release)",                    \
      "report_profiling (for release)",             \
      "select sharding functor (for release)",      \
      "select partition projection",                \
      "map_partition",                              \
      "select_partition_sources",                   \
      "report_profiling (for partition)",           \
      "select sharding functor (for partition)",    \
      "select sharding functor (for fill)",         \
      "map future map reduction",                   \
      "configure_context",                          \
      "select_tunable_value",                       \
      "select sharding functor (for must epoch)",   \
      "map_must_epoch",                             \
      "map_dataflow_graph",                         \
      "memoize_operation",                          \
      "select_tasks_to_map",                        \
      "select_steal_targets",                       \
      "permit_steal_request",                       \
      "handle_message",                             \
      "handle_task_result",                         \
      "application mapper call",                    \
    }

    // Methodology for assigning priorities to meta-tasks:
    // Minimum and low priority are for things like profiling
    // that we don't want to interfere with normal execution.
    // Resource priority is reserved for tasks that have been 
    // granted resources like reservations. Running priority
    // is the highest and guarantees that we drain out any 
    // previously running tasks over starting new ones. The rest
    // of the priorities are classified as either 'throughput'
    // or 'latency' sensitive. Under each of these two major
    // categories there are four sub-priorities:
    //  - work: general work to be done
    //  - deferred: work that was already scheduled but 
    //              for which a continuation had to be 
    //              made so we don't want to wait behind
    //              work that hasn't started yet
    //  - messsage: a message from a remote node that we
    //              should handle sooner than our own
    //              work since work on the other node is
    //              blocked waiting on our response
    //  - response: a response message from a remote node
    //              that we should handle to unblock work
    //              on our own node
    enum LgPriority {
      LG_MIN_PRIORITY = INT_MIN,
      LG_LOW_PRIORITY = -1,
      // Throughput priorities
      LG_THROUGHPUT_WORK_PRIORITY = 0,
      LG_THROUGHPUT_DEFERRED_PRIORITY = 1,
      LG_THROUGHPUT_MESSAGE_PRIORITY = 2,
      LG_THROUGHPUT_RESPONSE_PRIORITY = 3,
      // Latency priorities
      LG_LATENCY_WORK_PRIORITY = 4,
      LG_LATENCY_DEFERRED_PRIORITY = 5,
      LG_LATENCY_MESSAGE_PRIORITY = 6,
      LG_LATENCY_RESPONSE_PRIORITY = 7,
      // Resource priorities
      LG_RESOURCE_PRIORITY = 8,
      // Running priorities
      LG_RUNNING_PRIORITY = 9,
    };

    enum VirtualChannelKind {
      // The default and work virtual channels are unordered
      DEFAULT_VIRTUAL_CHANNEL = 0, // latency priority
      THROUGHPUT_VIRTUAL_CHANNEL = 1, // throughput priority
      LAST_UNORDERED_VIRTUAL_CHANNEL = THROUGHPUT_VIRTUAL_CHANNEL,
      // All the rest of these are ordered (latency-priority) channels
      MAPPER_VIRTUAL_CHANNEL = 1, 
      TASK_VIRTUAL_CHANNEL = 2,
      INDEX_SPACE_VIRTUAL_CHANNEL = 3,
      FIELD_SPACE_VIRTUAL_CHANNEL = 4,
      REFERENCE_VIRTUAL_CHANNEL = 6,
      UPDATE_VIRTUAL_CHANNEL = 7, // deferred-priority
      SUBSET_VIRTUAL_CHANNEL = 8,
      COLLECTIVE_VIRTUAL_CHANNEL = 9,
      LAYOUT_CONSTRAINT_VIRTUAL_CHANNEL = 10,
      EXPRESSION_VIRTUAL_CHANNEL = 11,
      MIGRATION_VIRTUAL_CHANNEL = 12,
      TRACING_VIRTUAL_CHANNEL = 13,
      RENDEZVOUS_VIRTUAL_CHANNEL = 14,
      MAX_NUM_VIRTUAL_CHANNELS = 15, // this one must be last
    };

    enum MessageKind {
      SEND_STARTUP_BARRIER,
      TASK_MESSAGE,
      STEAL_MESSAGE,
      ADVERTISEMENT_MESSAGE,
      SEND_REGISTRATION_CALLBACK,
      SEND_REMOTE_TASK_REPLAY,
      SEND_REMOTE_TASK_PROFILING_RESPONSE,
      SEND_SHARED_OWNERSHIP,
      SEND_INDEX_SPACE_REQUEST,
      SEND_INDEX_SPACE_RESPONSE,
      SEND_INDEX_SPACE_RETURN,
      SEND_INDEX_SPACE_SET,
      SEND_INDEX_SPACE_CHILD_REQUEST,
      SEND_INDEX_SPACE_CHILD_RESPONSE,
      SEND_INDEX_SPACE_COLORS_REQUEST,
      SEND_INDEX_SPACE_COLORS_RESPONSE,
      SEND_INDEX_SPACE_REMOTE_EXPRESSION_REQUEST,
      SEND_INDEX_SPACE_REMOTE_EXPRESSION_RESPONSE,
      SEND_INDEX_SPACE_GENERATE_COLOR_REQUEST,
      SEND_INDEX_SPACE_GENERATE_COLOR_RESPONSE,
      SEND_INDEX_SPACE_RELEASE_COLOR,
      SEND_INDEX_PARTITION_NOTIFICATION,
      SEND_INDEX_PARTITION_REQUEST,
      SEND_INDEX_PARTITION_RESPONSE,
      SEND_INDEX_PARTITION_RETURN,
      SEND_INDEX_PARTITION_CHILD_REQUEST,
      SEND_INDEX_PARTITION_CHILD_RESPONSE,
      SEND_INDEX_PARTITION_CHILD_REPLICATION,
      SEND_INDEX_PARTITION_DISJOINT_UPDATE,
      SEND_INDEX_PARTITION_SHARD_RECTS_REQUEST,
      SEND_INDEX_PARTITION_SHARD_RECTS_RESPONSE,
      SEND_INDEX_PARTITION_REMOTE_INTERFERENCE_REQUEST,
      SEND_INDEX_PARTITION_REMOTE_INTERFERENCE_RESPONSE,
      SEND_FIELD_SPACE_NODE,
      SEND_FIELD_SPACE_REQUEST,
      SEND_FIELD_SPACE_RETURN,
      SEND_FIELD_SPACE_ALLOCATOR_REQUEST,
      SEND_FIELD_SPACE_ALLOCATOR_RESPONSE,
      SEND_FIELD_SPACE_ALLOCATOR_INVALIDATION,
      SEND_FIELD_SPACE_ALLOCATOR_FLUSH,
      SEND_FIELD_SPACE_ALLOCATOR_FREE,
      SEND_FIELD_SPACE_INFOS_REQUEST,
      SEND_FIELD_SPACE_INFOS_RESPONSE,
      SEND_FIELD_ALLOC_REQUEST,
      SEND_FIELD_SIZE_UPDATE,
      SEND_FIELD_FREE,
      SEND_FIELD_FREE_INDEXES,
      SEND_FIELD_SPACE_LAYOUT_INVALIDATION,
      SEND_LOCAL_FIELD_ALLOC_REQUEST,
      SEND_LOCAL_FIELD_ALLOC_RESPONSE,
      SEND_LOCAL_FIELD_FREE,
      SEND_LOCAL_FIELD_UPDATE,
      SEND_TOP_LEVEL_REGION_REQUEST,
      SEND_TOP_LEVEL_REGION_RETURN,
      INDEX_SPACE_DESTRUCTION_MESSAGE,
      INDEX_PARTITION_DESTRUCTION_MESSAGE,
      FIELD_SPACE_DESTRUCTION_MESSAGE,
      LOGICAL_REGION_DESTRUCTION_MESSAGE,
      INDIVIDUAL_REMOTE_FUTURE_SIZE,
      INDIVIDUAL_REMOTE_OUTPUT_REGISTRATION,
      INDIVIDUAL_REMOTE_COMPLETE,
      INDIVIDUAL_REMOTE_COMMIT,
      SLICE_REMOTE_MAPPED,
      SLICE_REMOTE_COMPLETE,
      SLICE_REMOTE_COMMIT,
      SLICE_VERIFY_CONCURRENT_EXECUTION,
      SLICE_FIND_INTRA_DEP,
      SLICE_RECORD_INTRA_DEP,
      SLICE_REMOTE_COLLECTIVE_RENDEZVOUS,
      SLICE_REMOTE_VERSIONING_COLLECTIVE_RENDEZVOUS,
      SLICE_REMOTE_OUTPUT_EXTENTS,
      SLICE_REMOTE_OUTPUT_REGISTRATION,
      DISTRIBUTED_REMOTE_REGISTRATION,
      DISTRIBUTED_DOWNGRADE_REQUEST,
      DISTRIBUTED_DOWNGRADE_RESPONSE,
      DISTRIBUTED_DOWNGRADE_SUCCESS,
      DISTRIBUTED_DOWNGRADE_UPDATE,
      DISTRIBUTED_GLOBAL_ACQUIRE_REQUEST,
      DISTRIBUTED_GLOBAL_ACQUIRE_RESPONSE,
      DISTRIBUTED_VALID_ACQUIRE_REQUEST,
      DISTRIBUTED_VALID_ACQUIRE_RESPONSE,
      SEND_ATOMIC_RESERVATION_REQUEST,
      SEND_ATOMIC_RESERVATION_RESPONSE,
      SEND_PADDED_RESERVATION_REQUEST,
      SEND_PADDED_RESERVATION_RESPONSE,
      SEND_CREATED_REGION_CONTEXTS,
      SEND_MATERIALIZED_VIEW,
      SEND_FILL_VIEW,
      SEND_FILL_VIEW_VALUE,
      SEND_PHI_VIEW,
      SEND_REDUCTION_VIEW,
      SEND_REPLICATED_VIEW,
      SEND_ALLREDUCE_VIEW,
      SEND_INSTANCE_MANAGER,
      SEND_MANAGER_UPDATE,
      SEND_COLLECTIVE_DISTRIBUTE_FILL,
      SEND_COLLECTIVE_DISTRIBUTE_POINT,
      SEND_COLLECTIVE_DISTRIBUTE_POINTWISE,
      SEND_COLLECTIVE_DISTRIBUTE_REDUCTION,
      SEND_COLLECTIVE_DISTRIBUTE_BROADCAST,
      SEND_COLLECTIVE_DISTRIBUTE_REDUCECAST,
      SEND_COLLECTIVE_DISTRIBUTE_HOURGLASS,
      SEND_COLLECTIVE_DISTRIBUTE_ALLREDUCE,
      SEND_COLLECTIVE_HAMMER_REDUCTION,
      SEND_COLLECTIVE_FUSE_GATHER,
      SEND_COLLECTIVE_USER_REQUEST,
      SEND_COLLECTIVE_USER_RESPONSE,
      SEND_COLLECTIVE_REGISTER_USER,
      SEND_COLLECTIVE_REMOTE_INSTANCES_REQUEST,
      SEND_COLLECTIVE_REMOTE_INSTANCES_RESPONSE,
      SEND_COLLECTIVE_NEAREST_INSTANCES_REQUEST,
      SEND_COLLECTIVE_NEAREST_INSTANCES_RESPONSE,
      SEND_COLLECTIVE_REMOTE_REGISTRATION,
      SEND_COLLECTIVE_FINALIZE_MAPPING,
      SEND_COLLECTIVE_VIEW_CREATION,
      SEND_COLLECTIVE_VIEW_DELETION,
      SEND_COLLECTIVE_VIEW_RELEASE,
      SEND_COLLECTIVE_VIEW_NOTIFICATION,
      SEND_COLLECTIVE_VIEW_MAKE_VALID,
      SEND_COLLECTIVE_VIEW_MAKE_INVALID,
      SEND_COLLECTIVE_VIEW_INVALIDATE_REQUEST,
      SEND_COLLECTIVE_VIEW_INVALIDATE_RESPONSE,
      SEND_COLLECTIVE_VIEW_ADD_REMOTE_REFERENCE,
      SEND_COLLECTIVE_VIEW_REMOVE_REMOTE_REFERENCE,
      SEND_CREATE_TOP_VIEW_REQUEST,
      SEND_CREATE_TOP_VIEW_RESPONSE,
      SEND_VIEW_REQUEST,
      SEND_VIEW_REGISTER_USER,
      SEND_VIEW_FIND_COPY_PRE_REQUEST,
      SEND_VIEW_ADD_COPY_USER,
      SEND_VIEW_FIND_LAST_USERS_REQUEST,
      SEND_VIEW_FIND_LAST_USERS_RESPONSE,
      SEND_VIEW_REPLICATION_REQUEST,
      SEND_VIEW_REPLICATION_RESPONSE,
      SEND_VIEW_REPLICATION_REMOVAL,
      SEND_MANAGER_REQUEST,
      SEND_FUTURE_RESULT,
      SEND_FUTURE_RESULT_SIZE,
      SEND_FUTURE_SUBSCRIPTION,
      SEND_FUTURE_CREATE_INSTANCE_REQUEST,
      SEND_FUTURE_CREATE_INSTANCE_RESPONSE,
      SEND_FUTURE_MAP_REQUEST,
      SEND_FUTURE_MAP_RESPONSE,
      SEND_REPL_COMPUTE_EQUIVALENCE_SETS,
      SEND_REPL_OUTPUT_EQUIVALENCE_SET,
      SEND_REPL_REFINE_EQUIVALENCE_SETS,
      SEND_REPL_EQUIVALENCE_SET_NOTIFICATION,
      SEND_REPL_INTRA_SPACE_DEP,
      SEND_REPL_BROADCAST_UPDATE,
      SEND_REPL_CREATED_REGIONS,
      SEND_REPL_TRACE_EVENT_REQUEST,
      SEND_REPL_TRACE_EVENT_RESPONSE,
      SEND_REPL_TRACE_FRONTIER_REQUEST,
      SEND_REPL_TRACE_FRONTIER_RESPONSE,
      SEND_REPL_TRACE_UPDATE,
      SEND_REPL_IMPLICIT_RENDEZVOUS,
      SEND_REPL_FIND_COLLECTIVE_VIEW,
      SEND_MAPPER_MESSAGE,
      SEND_MAPPER_BROADCAST,
      SEND_TASK_IMPL_SEMANTIC_REQ,
      SEND_INDEX_SPACE_SEMANTIC_REQ,
      SEND_INDEX_PARTITION_SEMANTIC_REQ,
      SEND_FIELD_SPACE_SEMANTIC_REQ,
      SEND_FIELD_SEMANTIC_REQ,
      SEND_LOGICAL_REGION_SEMANTIC_REQ,
      SEND_LOGICAL_PARTITION_SEMANTIC_REQ,
      SEND_TASK_IMPL_SEMANTIC_INFO,
      SEND_INDEX_SPACE_SEMANTIC_INFO,
      SEND_INDEX_PARTITION_SEMANTIC_INFO,
      SEND_FIELD_SPACE_SEMANTIC_INFO,
      SEND_FIELD_SEMANTIC_INFO,
      SEND_LOGICAL_REGION_SEMANTIC_INFO,
      SEND_LOGICAL_PARTITION_SEMANTIC_INFO,
      SEND_REMOTE_CONTEXT_REQUEST,
      SEND_REMOTE_CONTEXT_RESPONSE,
      SEND_REMOTE_CONTEXT_PHYSICAL_REQUEST,
      SEND_REMOTE_CONTEXT_PHYSICAL_RESPONSE,
      SEND_REMOTE_CONTEXT_FIND_COLLECTIVE_VIEW_REQUEST,
      SEND_REMOTE_CONTEXT_FIND_COLLECTIVE_VIEW_RESPONSE,
      SEND_COMPUTE_EQUIVALENCE_SETS_REQUEST,
      SEND_COMPUTE_EQUIVALENCE_SETS_RESPONSE,
      SEND_COMPUTE_EQUIVALENCE_SETS_PENDING,
      SEND_OUTPUT_EQUIVALENCE_SET_REQUEST,
      SEND_OUTPUT_EQUIVALENCE_SET_RESPONSE,
      SEND_CANCEL_EQUIVALENCE_SETS_SUBSCRIPTION,
      SEND_INVALIDATE_EQUIVALENCE_SETS_SUBSCRIPTION,
      SEND_EQUIVALENCE_SET_CREATION,
      SEND_EQUIVALENCE_SET_REUSE,
      SEND_EQUIVALENCE_SET_REQUEST,
      SEND_EQUIVALENCE_SET_RESPONSE,
      SEND_EQUIVALENCE_SET_REPLICATION_REQUEST,
      SEND_EQUIVALENCE_SET_REPLICATION_RESPONSE,
      SEND_EQUIVALENCE_SET_MIGRATION,
      SEND_EQUIVALENCE_SET_OWNER_UPDATE,
      SEND_EQUIVALENCE_SET_CLONE_REQUEST,
      SEND_EQUIVALENCE_SET_CLONE_RESPONSE,
      SEND_EQUIVALENCE_SET_CAPTURE_REQUEST,
      SEND_EQUIVALENCE_SET_CAPTURE_RESPONSE,
      SEND_EQUIVALENCE_SET_REMOTE_REQUEST_INSTANCES,
      SEND_EQUIVALENCE_SET_REMOTE_REQUEST_INVALID,
      SEND_EQUIVALENCE_SET_REMOTE_REQUEST_ANTIVALID,
      SEND_EQUIVALENCE_SET_REMOTE_UPDATES,
      SEND_EQUIVALENCE_SET_REMOTE_ACQUIRES,
      SEND_EQUIVALENCE_SET_REMOTE_RELEASES,
      SEND_EQUIVALENCE_SET_REMOTE_COPIES_ACROSS,
      SEND_EQUIVALENCE_SET_REMOTE_OVERWRITES,
      SEND_EQUIVALENCE_SET_REMOTE_FILTERS,
      SEND_EQUIVALENCE_SET_REMOTE_CLONES,
      SEND_EQUIVALENCE_SET_REMOTE_INSTANCES,
      SEND_INSTANCE_REQUEST,
      SEND_INSTANCE_RESPONSE,
      SEND_EXTERNAL_CREATE_REQUEST,
      SEND_EXTERNAL_CREATE_RESPONSE,
      SEND_EXTERNAL_ATTACH,
      SEND_EXTERNAL_DETACH,
      SEND_GC_PRIORITY_UPDATE,
      SEND_GC_REQUEST,
      SEND_GC_RESPONSE,
      SEND_GC_ACQUIRE,
      SEND_GC_FAILED,
      SEND_GC_MISMATCH,
      SEND_GC_NOTIFY,
      SEND_GC_DEBUG_REQUEST,
      SEND_GC_DEBUG_RESPONSE,
      SEND_GC_RECORD_EVENT,
      SEND_ACQUIRE_REQUEST,
      SEND_ACQUIRE_RESPONSE,
      SEND_VARIANT_BROADCAST,
      SEND_CONSTRAINT_REQUEST,
      SEND_CONSTRAINT_RESPONSE,
      SEND_CONSTRAINT_RELEASE,
      SEND_TOP_LEVEL_TASK_COMPLETE,
      SEND_MPI_RANK_EXCHANGE,
      SEND_REPLICATE_DISTRIBUTION,
      SEND_REPLICATE_COLLECTIVE_VERSIONING,
      SEND_REPLICATE_COLLECTIVE_MAPPING,
      SEND_REPLICATE_VIRTUAL_RENDEZVOUS,
      SEND_REPLICATE_POST_MAPPED,
      SEND_REPLICATE_POST_EXECUTION,
      SEND_REPLICATE_TRIGGER_COMPLETE,
      SEND_REPLICATE_TRIGGER_COMMIT,
      SEND_CONTROL_REPLICATE_RENDEZVOUS_MESSAGE,
      SEND_LIBRARY_MAPPER_REQUEST,
      SEND_LIBRARY_MAPPER_RESPONSE,
      SEND_LIBRARY_TRACE_REQUEST,
      SEND_LIBRARY_TRACE_RESPONSE,
      SEND_LIBRARY_PROJECTION_REQUEST,
      SEND_LIBRARY_PROJECTION_RESPONSE,
      SEND_LIBRARY_SHARDING_REQUEST,
      SEND_LIBRARY_SHARDING_RESPONSE,
      SEND_LIBRARY_TASK_REQUEST,
      SEND_LIBRARY_TASK_RESPONSE,
      SEND_LIBRARY_REDOP_REQUEST,
      SEND_LIBRARY_REDOP_RESPONSE,
      SEND_LIBRARY_SERDEZ_REQUEST,
      SEND_LIBRARY_SERDEZ_RESPONSE,
      SEND_REMOTE_OP_REPORT_UNINIT,
      SEND_REMOTE_OP_PROFILING_COUNT_UPDATE,
      SEND_REMOTE_OP_COMPLETION_EFFECT,
      SEND_REMOTE_TRACE_UPDATE,
      SEND_REMOTE_TRACE_RESPONSE,
      SEND_FREE_EXTERNAL_ALLOCATION,
      SEND_CREATE_FUTURE_INSTANCE_REQUEST,
      SEND_CREATE_FUTURE_INSTANCE_RESPONSE,
      SEND_FREE_FUTURE_INSTANCE,
      SEND_REMOTE_DISTRIBUTED_ID_REQUEST,
      SEND_REMOTE_DISTRIBUTED_ID_RESPONSE,
      SEND_CONCURRENT_RESERVATION_CREATION,
      SEND_CONCURRENT_EXECUTION_ANALYSIS,
      SEND_CONTROL_REPLICATION_FUTURE_ALLREDUCE,
      SEND_CONTROL_REPLICATION_FUTURE_BROADCAST,
      SEND_CONTROL_REPLICATION_FUTURE_REDUCTION,
      SEND_CONTROL_REPLICATION_VALUE_ALLREDUCE,
      SEND_CONTROL_REPLICATION_VALUE_BROADCAST,
      SEND_CONTROL_REPLICATION_VALUE_EXCHANGE,
      SEND_CONTROL_REPLICATION_BUFFER_BROADCAST,
      SEND_CONTROL_REPLICATION_SHARD_SYNC_TREE,
      SEND_CONTROL_REPLICATION_SHARD_EVENT_TREE,
      SEND_CONTROL_REPLICATION_SINGLE_TASK_TREE,
      SEND_CONTROL_REPLICATION_CROSS_PRODUCT_PARTITION,
      SEND_CONTROL_REPLICATION_SHARDING_GATHER_COLLECTIVE,
      SEND_CONTROL_REPLICATION_INDIRECT_COPY_EXCHANGE,
      SEND_CONTROL_REPLICATION_FIELD_DESCRIPTOR_EXCHANGE,
      SEND_CONTROL_REPLICATION_FIELD_DESCRIPTOR_GATHER,
      SEND_CONTROL_REPLICATION_DEPPART_RESULT_SCATTER,
      SEND_CONTROL_REPLICATION_BUFFER_EXCHANGE,
      SEND_CONTROL_REPLICATION_FUTURE_NAME_EXCHANGE,
      SEND_CONTROL_REPLICATION_MUST_EPOCH_MAPPING_BROADCAST,
      SEND_CONTROL_REPLICATION_MUST_EPOCH_MAPPING_EXCHANGE,
      SEND_CONTROL_REPLICATION_MUST_EPOCH_DEPENDENCE_EXCHANGE,
      SEND_CONTROL_REPLICATION_MUST_EPOCH_COMPLETION_EXCHANGE,
      SEND_CONTROL_REPLICATION_CHECK_COLLECTIVE_MAPPING,
      SEND_CONTROL_REPLICATION_CHECK_COLLECTIVE_SOURCES,
      SEND_CONTROL_REPLICATION_TEMPLATE_INDEX_EXCHANGE,
      SEND_CONTROL_REPLICATION_UNORDERED_EXCHANGE,
      SEND_CONTROL_REPLICATION_CONSENSUS_MATCH,
      SEND_CONTROL_REPLICATION_VERIFY_CONTROL_REPLICATION_EXCHANGE,
      SEND_CONTROL_REPLICATION_OUTPUT_SIZE_EXCHANGE,
      SEND_CONTROL_REPLICATION_INDEX_ATTACH_LAUNCH_SPACE,
      SEND_CONTROL_REPLICATION_INDEX_ATTACH_UPPER_BOUND,
      SEND_CONTROL_REPLICATION_INDEX_ATTACH_EXCHANGE,
      SEND_CONTROL_REPLICATION_SHARD_PARTICIPANTS_EXCHANGE,
      SEND_CONTROL_REPLICATION_IMPLICIT_SHARDING_FUNCTOR,
      SEND_CONTROL_REPLICATION_CREATE_FILL_VIEW,
      SEND_CONTROL_REPLICATION_VERSIONING_RENDEZVOUS,
      SEND_CONTROL_REPLICATION_VIEW_RENDEZVOUS,
      SEND_CONTROL_REPLICATION_CONCURRENT_EXECUTION_VALIDATION,
      SEND_CONTROL_REPLICATION_PROJECTION_TREE_EXCHANGE,
      SEND_CONTROL_REPLICATION_TIMEOUT_MATCH_EXCHANGE,
      SEND_CONTROL_REPLICATION_MASK_EXCHANGE,
      SEND_CONTROL_REPLICATION_PREDICATE_EXCHANGE,
      SEND_CONTROL_REPLICATION_CROSS_PRODUCT_EXCHANGE,
      SEND_CONTROL_REPLICATION_SLOW_BARRIER,
      SEND_SHUTDOWN_NOTIFICATION,
      SEND_SHUTDOWN_RESPONSE,
      LAST_SEND_KIND, // This one must be last
    };

#define LG_MESSAGE_DESCRIPTIONS(name)                                 \
      const char *name[LAST_SEND_KIND] = {                            \
        "Send Startup Barrier",                                       \
        "Task Message",                                               \
        "Steal Message",                                              \
        "Advertisement Message",                                      \
        "Send Registration Callback",                                 \
        "Send Remote Task Replay",                                    \
        "Send Remote Task Profiling Response",                        \
        "Send Shared Ownership",                                      \
        "Send Index Space Request",                                   \
        "Send Index Space Response",                                  \
        "Send Index Space Return",                                    \
        "Send Index Space Set",                                       \
        "Send Index Space Child Request",                             \
        "Send Index Space Child Response",                            \
        "Send Index Space Colors Request",                            \
        "Send Index Space Colors Response",                           \
        "Send Index Space Remote Expression Request",                 \
        "Send Index Space Remote Expression Response",                \
        "Send Index Space Generate Color Request",                    \
        "Send Index Space Generate Color Response",                   \
        "Send Index Space Release Color",                             \
        "Send Index Partition Notification",                          \
        "Send Index Partition Request",                               \
        "Send Index Partition Response",                              \
        "Send Index Partition Return",                                \
        "Send Index Partition Child Request",                         \
        "Send Index Partition Child Response",                        \
        "Send Index Partition Child Replication",                     \
        "Send Index Partition Disjoint Update",                       \
        "Send Index Partition Shard Rects Request",                   \
        "Send Index Partition Shard Rects Response",                  \
        "Send Index Partition Remote Interference Request",           \
        "Send Index Partition Remote Interference Response",          \
        "Send Field Space Node",                                      \
        "Send Field Space Request",                                   \
        "Send Field Space Return",                                    \
        "Send Field Space Allocator Request",                         \
        "Send Field Space Allocator Response",                        \
        "Send Field Space Allocator Invalidation",                    \
        "Send Field Space Allocator Flush",                           \
        "Send Field Space Allocator Free",                            \
        "Send Field Space Infos Request",                             \
        "Send Field Space Infos Response",                            \
        "Send Field Alloc Request",                                   \
        "Send Field Size Update",                                     \
        "Send Field Free",                                            \
        "Send Field Free Indexes",                                    \
        "Send Field Space Layout Invalidation",                       \
        "Send Local Field Alloc Request",                             \
        "Send Local Field Alloc Response",                            \
        "Send Local Field Free",                                      \
        "Send Local Field Update",                                    \
        "Send Top Level Region Request",                              \
        "Send Top Level Region Return",                               \
        "Index Space Destruction",                                    \
        "Index Partition Destruction",                                \
        "Field Space Destruction",                                    \
        "Logical Region Destruction",                                 \
        "Individual Remote Future Size",                              \
        "Individual Remote Output Region Registration",               \
        "Individual Remote Complete",                                 \
        "Individual Remote Commit",                                   \
        "Slice Remote Mapped",                                        \
        "Slice Remote Complete",                                      \
        "Slice Remote Commit",                                        \
        "Slice Verify Concurrent Execution",                          \
        "Slice Find Intra-Space Dependence",                          \
        "Slice Record Intra-Space Dependence",                        \
        "Slice Remote Collective Rendezvous",                         \
        "Slice Remote Collective Versioning Rendezvous",              \
        "Slice Remote Output Region Extents",                         \
        "Slice Remote Output Region Registration",                    \
        "Distributed Remote Registration",                            \
        "Distributed Downgrade Request",                              \
        "Distributed Downgrade Response",                             \
        "Distributed Downgrade Success",                              \
        "Distributed Downgrade Update",                               \
        "Distributed Global Acquire Request",                         \
        "Distributed Global Acquire Response",                        \
        "Distributed Valid Acquire Request",                          \
        "Distributed Valid Acquire Response",                         \
        "Send Atomic Reservation Request",                            \
        "Send Atomic Reservation Response",                           \
        "Send Padded Reservation Request",                            \
        "Send Padded Reservation Response",                           \
        "Send Created Region Contexts",                               \
        "Send Materialized View",                                     \
        "Send Fill View",                                             \
        "Send Fill View Value",                                       \
        "Send Phi View",                                              \
        "Send Reduction View",                                        \
        "Send Replicated View",                                       \
        "Send Allreduce View",                                        \
        "Send Instance Manager",                                      \
        "Send Manager Update",                                        \
        "Send Collective Distribute Fill",                            \
        "Send Collective Distribute Point",                           \
        "Send Collective Distribute Pointwise",                       \
        "Send Collective Distribute Reduction",                       \
        "Send Collective Distribute Broadcast",                       \
        "Send Collective Distribute Reducecast",                      \
        "Send Collective Distribute Hourglass",                       \
        "Send Collective Distribute Allreduce",                       \
        "Send Collective Hammer Reduction",                           \
        "Send Collective Fuse Gather",                                \
        "Send Collective User Request",                               \
        "Send Collective User Response",                              \
        "Send Collective Individual Register User",                   \
        "Send Collective Remote Instances Request",                   \
        "Send Collective Remote Instances Response",                  \
        "Send Collective Nearest Instances Request",                  \
        "Send Collective Nearest Instances Response",                 \
        "Send Collective Remote Registration",                        \
        "Send Collective Finalize Mapping",                           \
        "Send Collective View Creation",                              \
        "Send Collective View Deletion",                              \
        "Send Collective View Release",                               \
        "Send Collective View Deletion Notification",                 \
        "Send Collective View Make Valid",                            \
        "Send Collective View Make Invalid",                          \
        "Send Collective View Invalidate Request",                    \
        "Send Collective View Invalidate Response",                   \
        "Send Collective View Add Remote Reference",                  \
        "Send Collective View Remove Remote Reference",               \
        "Send Create Top View Request",                               \
        "Send Create Top View Response",                              \
        "Send View Request",                                          \
        "Send View Register User",                                    \
        "Send View Find Copy Preconditions Request",                  \
        "Send View Add Copy User",                                    \
        "Send View Find Last Users Request",                          \
        "Send View Find Last Users Response",                         \
        "Send View Replication Request",                              \
        "Send View Replication Response",                             \
        "Send View Replication Removal",                              \
        "Send Manager Request",                                       \
        "Send Future Result",                                         \
        "Send Future Result Size",                                    \
        "Send Future Subscription",                                   \
        "Send Future Create Instance Request",                        \
        "Send Future Create Instance Response",                       \
        "Send Future Map Future Request",                             \
        "Send Future Map Future Response",                            \
        "Send Replicate Compute Equivalence Sets",                    \
        "Send Replicate Register Output Equivalence Set",             \
        "Send Replicate Refine Equivalence Sets",                     \
        "Send Replicate Equivalence Set Notification",                \
        "Send Replicate Intra Space Dependence",                      \
        "Send Replicate Broadcast Update",                            \
        "Send Replicate Created Regions Return",                      \
        "Send Replicate Trace Event Request",                         \
        "Send Replicate Trace Event Response",                        \
        "Send Replicate Trace Frontier Request",                      \
        "Send Replicate Trace Frontier Response",                     \
        "Send Replicate Trace Update",                                \
        "Send Replicate Implicit Rendezvous",                         \
        "Send Replicate Find or Create Collective View",              \
        "Send Mapper Message",                                        \
        "Send Mapper Broadcast",                                      \
        "Send Task Impl Semantic Req",                                \
        "Send Index Space Semantic Req",                              \
        "Send Index Partition Semantic Req",                          \
        "Send Field Space Semantic Req",                              \
        "Send Field Semantic Req",                                    \
        "Send Logical Region Semantic Req",                           \
        "Send Logical Partition Semantic Req",                        \
        "Send Task Impl Semantic Info",                               \
        "Send Index Space Semantic Info",                             \
        "Send Index Partition Semantic Info",                         \
        "Send Field Space Semantic Info",                             \
        "Send Field Semantic Info",                                   \
        "Send Logical Region Semantic Info",                          \
        "Send Logical Partition Semantic Info",                       \
        "Send Remote Context Request",                                \
        "Send Remote Context Response",                               \
        "Send Remote Context Physical Request",                       \
        "Send Remote Context Physical Response",                      \
        "Send Remote Context Find Collective View Request",           \
        "Send Remote Context Find Collective View Response",          \
        "Send Compute Equivalence Sets Request",                      \
        "Send Compute Equivalence Sets Response",                     \
        "Send Compute Equivalence Sets Pending",                      \
        "Send Register Output Equivalence Set Request",               \
        "Send Register Output Equivalence Set Response",              \
        "Send Cancel Equivalence Sets Subscription",                  \
        "Send Invalidate Equivalence Sets Subscription",              \
        "Send Equivalence Set Creation",                              \
        "Send Equivalence Set Reuse",                                 \
        "Send Equivalence Set Request",                               \
        "Send Equivalence Set Response",                              \
        "Send Equivalence Set Replication Request",                   \
        "Send Equivalence Set Replication Response",                  \
        "Send Equivalence Set Migration",                             \
        "Send Equivalence Set Owner Update",                          \
        "Send Equivalence Set Clone Request",                         \
        "Send Equivalence Set Clone Response",                        \
        "Send Equivalence Set Tracing Capture Request",               \
        "Send Equivalence Set Tracing Capture Response",              \
        "Send Equivalence Set Remote Request Instances",              \
        "Send Equivalence Set Remote Request Invalid",                \
        "Send Equivalence Set Remote Request Antivalid",              \
        "Send Equivalence Set Remote Updates",                        \
        "Send Equivalence Set Remote Acquires",                       \
        "Send Equivalence Set Remote Releases",                       \
        "Send Equivalence Set Remote Copies Across",                  \
        "Send Equivalence Set Remote Overwrites",                     \
        "Send Equivalence Set Remote Filters",                        \
        "Send Equivalence Set Remote Clones",                         \
        "Send Equivalence Set Remote Instances",                      \
        "Send Instance Request",                                      \
        "Send Instance Response",                                     \
        "Send External Create Request",                               \
        "Send External Create Response",                              \
        "Send External Attach",                                       \
        "Send External Detach",                                       \
        "Send GC Priority Update",                                    \
        "Send GC Request",                                            \
        "Send GC Response",                                           \
        "Send GC Acquire Request",                                    \
        "Send GC Acquire Failed",                                     \
        "Send GC Packed Reference Mismatch",                          \
        "Send GC Notify Collected",                                   \
        "Send GC Debug Request",                                      \
        "Send GC Debug Response",                                     \
        "Send GC Record Event",                                       \
        "Send Acquire Request",                                       \
        "Send Acquire Response",                                      \
        "Send Task Variant Broadcast",                                \
        "Send Constraint Request",                                    \
        "Send Constraint Response",                                   \
        "Send Constraint Release",                                    \
        "Top Level Task Complete",                                    \
        "Send MPI Rank Exchange",                                     \
        "Send Replication Distribution",                              \
        "Send Replication Collective Versioning",                     \
        "Send Replication Collective Mapping",                        \
        "Send Replication Virtual Mapping Rendezvous",                \
        "Send Replication Post Mapped",                               \
        "Send Replication Post Execution",                            \
        "Send Replication Trigger Complete",                          \
        "Send Replication Trigger Commit",                            \
        "Send Control Replication Rendezvous Message",                \
        "Send Library Mapper Request",                                \
        "Send Library Mapper Response",                               \
        "Send Library Trace Request",                                 \
        "Send Library Trace Response",                                \
        "Send Library Projection Request",                            \
        "Send Library Projection Response",                           \
        "Send Library Sharding Request",                              \
        "Send Library Sharding Response",                             \
        "Send Library Task Request",                                  \
        "Send Library Task Response",                                 \
        "Send Library Redop Request",                                 \
        "Send Library Redop Response",                                \
        "Send Library Serdez Request",                                \
        "Send Library Serdez Response",                               \
        "Remote Op Report Uninitialized",                             \
        "Remote Op Profiling Count Update",                           \
        "Remote Op Completion Effect",                                \
        "Send Remote Trace Update",                                   \
        "Send Remote Trace Response",                                 \
        "Send Free External Allocation",                              \
        "Send Create Future Instance Request",                        \
        "Send Create Future Instance Response",                       \
        "Send Free Future Instance",                                  \
        "Send Remote Distributed ID Request",                         \
        "Send Remote Distributed ID Response",                        \
        "Send Concurrent Reservation Creation",                       \
        "Send Concurrent Execution Analysis",                         \
        "Control Replication Collective Future All-Reduce",           \
        "Control Replication Collective Future Broadcast",            \
        "Control Replication Collective Future Reduction",            \
        "Control Replication Collective Value All-Reduce",            \
        "Control Replication Collective Value Broadcast",             \
        "Control Replication Collective Value Exchange",              \
        "Control Replication Collective Buffer Broadcast",            \
        "Control Replication Collective Shard Sync Tree",             \
        "Control Replication Collective Shard Event Tree",            \
        "Control Replication Collective Single Task Tree",            \
        "Control Replication Collective Cross Product Partition",     \
        "Control Replication Collective Sharding Gather Collective",  \
        "Control Replication Collective Indirect Copy Exchange",      \
        "Control Replication Collective Field Descriptor Exchange",   \
        "Control Replication Collective Field Descriptor Gather",     \
        "Control Replication Collective Deppart Result Scatter",      \
        "Control Replication Collective Buffer Exchange",             \
        "Control Replication Collective Future Name Exchange",        \
        "Control Replication Collective Must Epoch Mapping Broadcast",\
        "Control Replication Collective Must Epoch Mapping Exchange", \
        "Control Replication Collective Must Epoch Dependence Exchange",\
        "Control Replication Collective Must Epoch Completion Exchange",\
        "Control Replication Collective Check Mapping",               \
        "Control Replication Collective Check Sources",               \
        "Control Replication Collective Template Index Exchange",     \
        "Control Replication Collective Unordered Exchange",          \
        "Control Replication Collective Consensus Match",             \
        "Control Replication Collective Verify Control Replication Exchange",\
        "Control Replication Collective Output Size Exchange",        \
        "Control Replication Collective Index Attach Launch Space",   \
        "Control Replication Collective Index Attach Upper Bound",    \
        "Control Replication Collective Index Attach Exchange",       \
        "Control Replication Collective Shard Participants Exchange", \
        "Control Replication Collective Implicit Sharding Functor",   \
        "Control Replication Collective Create Fill View",            \
        "Control Replication Collective Versioning Rendezvous",       \
        "Control Replication Collective View Rendezvous",             \
        "Control Replication Collective Concurrent Execution Validation",\
        "Control Replication Collective Projection Tree Exchange",    \
        "Control Replication Collective Timeout Match Exchange",      \
        "Control Replication Collective Mask Exchange",               \
        "Control Replication Collective Predicate Exchange",          \
        "Control Replication Collective Cross Product Exchange",      \
        "Control Replication Collective Slow Barrier",                \
        "Send Shutdown Notification",                                 \
        "Send Shutdown Response",                                     \
      };

    // Runtime task numbering 
    enum {
      LG_STARTUP_TASK_ID      = Realm::Processor::TASK_ID_PROCESSOR_INIT,
      LG_SHUTDOWN_TASK_ID     = Realm::Processor::TASK_ID_PROCESSOR_SHUTDOWN,
      LG_TASK_ID              = Realm::Processor::TASK_ID_FIRST_AVAILABLE,
#ifdef LEGION_SEPARATE_META_TASKS
      LG_LEGION_PROFILING_ID  = LG_TASK_ID+LG_LAST_TASK_ID+LAST_SEND_KIND,
      LG_ENDPOINT_TASK_ID     = LG_TASK_ID+LG_LAST_TASK_ID+LAST_SEND_KIND+1,
      LG_APP_PROC_TASK_ID     = LG_TASK_ID+LG_LAST_TASK_ID+LAST_SEND_KIND+2,
      LG_TASK_ID_AVAILABLE    = LG_APP_PROC_TASK_ID+LG_LAST_TASK_ID,
#else
      LG_LEGION_PROFILING_ID  = LG_TASK_ID+1,
      LG_ENDPOINT_TASK_ID     = LG_TASK_ID+2,
      LG_APP_PROC_TASK_ID     = LG_TASK_ID+3,
      LG_TASK_ID_AVAILABLE    = LG_TASK_ID+4,
#endif
    };

    enum RuntimeCallKind {
      // Mapper runtime call kinds
      MAPPER_SEND_MESSAGE_CALL,
      MAPPER_BROADCAST_CALL,
      MAPPER_UNPACK_INSTANCE_CALL,
      MAPPER_CREATE_EVENT_CALL,
      MAPPER_HAS_TRIGGERED_CALL,
      MAPPER_TRIGGER_EVENT_CALL,
      MAPPER_WAIT_EVENT_CALL,
      MAPPER_FIND_EXECUTION_CONSTRAINTS_CALL,
      MAPPER_FIND_TASK_LAYOUT_CONSTRAINTS_CALL,
      MAPPER_FIND_LAYOUT_CONSTRAINTS_CALL,
      MAPPER_REGISTER_LAYOUT_CALL,
      MAPPER_RELEASE_LAYOUT_CALL,
      MAPPER_CONSTRAINTS_CONFLICT_CALL,
      MAPPER_CONSTRAINTS_ENTAIL_CALL,
      MAPPER_FIND_VALID_VARIANTS_CALL,
      MAPPER_FIND_TASK_VARIANT_NAME_CALL,
      MAPPER_IS_LEAF_VARIANT_CALL,
      MAPPER_IS_INNER_VARIANT_CALL,
      MAPPER_IS_IDEMPOTENT_VARIANT_CALL,
      MAPPER_IS_REPLICABLE_VARIANT_CALL,
      MAPPER_REGISTER_TASK_VARIANT_CALL,
      MAPPER_FILTER_VARIANTS_CALL,
      MAPPER_FILTER_INSTANCES_CALL,
      MAPPER_CREATE_PHYSICAL_INSTANCE_CALL,
      MAPPER_FIND_OR_CREATE_PHYSICAL_INSTANCE_CALL,
      MAPPER_FIND_PHYSICAL_INSTANCE_CALL,
      MAPPER_FIND_PHYSICAL_INSTANCES_CALL,
      MAPPER_SET_GC_PRIORITY_CALL,
      MAPPER_ACQUIRE_INSTANCE_CALL,
      MAPPER_ACQUIRE_INSTANCES_CALL,
      MAPPER_ACQUIRE_AND_FILTER_INSTANCES_CALL,
      MAPPER_RELEASE_INSTANCE_CALL,
      MAPPER_RELEASE_INSTANCES_CALL,
      MAPPER_ACQUIRE_FUTURE_CALL,
      MAPPER_CREATE_INDEX_SPACE_CALL,
      MAPPER_UNION_INDEX_SPACES_CALL,
      MAPPER_INTERSECT_INDEX_SPACES_CALL,
      MAPPER_SUBTRACT_INDEX_SPACES_CALL,
      MAPPER_INDEX_SPACE_EMPTY_CALL,
      MAPPER_INDEX_SPACES_OVERLAP_CALL,
      MAPPER_INDEX_SPACE_DOMINATES_CALL,
      MAPPER_HAS_INDEX_PARTITION_CALL,
      MAPPER_GET_INDEX_PARTITION_CALL,
      MAPPER_GET_INDEX_SUBSPACE_CALL,
      MAPPER_GET_INDEX_SPACE_DOMAIN_CALL,
      MAPPER_GET_INDEX_PARTITION_CS_CALL,
      MAPPER_GET_INDEX_PARTITION_CS_NAME_CALL,
      MAPPER_GET_INDEX_SPACE_PARTITION_COLORS_CALL,
      MAPPER_IS_INDEX_PARTITION_DISJOINT_CALL,
      MAPPER_IS_INDEX_PARTITION_COMPLETE_CALL,
      MAPPER_GET_INDEX_SPACE_COLOR_CALL,
      MAPPER_GET_INDEX_SPACE_COLOR_POINT_CALL,
      MAPPER_GET_INDEX_PARTITION_COLOR_CALL,
      MAPPER_GET_PARENT_INDEX_SPACE_CALL,
      MAPPER_HAS_PARENT_INDEX_PARTITION_CALL,
      MAPPER_GET_PARENT_INDEX_PARTITION_CALL,
      MAPPER_GET_INDEX_SPACE_DEPTH_CALL,
      MAPPER_GET_INDEX_PARTITION_DEPTH_CALL,
      MAPPER_GET_FIELD_SIZE_CALL,
      MAPPER_GET_FIELD_SPACE_FIELDS_CALL,
      MAPPER_GET_LOGICAL_PARTITION_CALL,
      MAPPER_GET_LOGICAL_PARTITION_BY_COLOR_CALL,
      MAPPER_GET_LOGICAL_PARTITION_BY_TREE_CALL,
      MAPPER_GET_LOGICAL_SUBREGION_CALL,
      MAPPER_GET_LOGICAL_SUBREGION_BY_COLOR_CALL,
      MAPPER_GET_LOGICAL_SUBREGION_BY_TREE_CALL,
      MAPPER_GET_LOGICAL_REGION_COLOR_CALL,
      MAPPER_GET_LOGICAL_REGION_COLOR_POINT_CALL,
      MAPPER_GET_LOGICAL_PARTITION_COLOR_CALL,
      MAPPER_GET_PARENT_LOGICAL_REGION_CALL,
      MAPPER_HAS_PARENT_LOGICAL_PARTITION_CALL,
      MAPPER_GET_PARENT_LOGICAL_PARTITION_CALL,
      MAPPER_RETRIEVE_SEMANTIC_INFO_CALL,
      MAPPER_RETRIEVE_NAME_CALL,
      MAPPER_AUTO_LOCK_CALL,
      // Old runtime call kinds
      PACK_BASE_TASK_CALL, 
      UNPACK_BASE_TASK_CALL,
      TASK_PRIVILEGE_CHECK_CALL,
      CLONE_TASK_CALL,
      COMPUTE_POINT_REQUIREMENTS_CALL,
      INTRA_TASK_ALIASING_CALL,
      ACTIVATE_SINGLE_CALL,
      DEACTIVATE_SINGLE_CALL,
      SELECT_INLINE_VARIANT_CALL,
      INLINE_CHILD_TASK_CALL,
      PACK_SINGLE_TASK_CALL,
      UNPACK_SINGLE_TASK_CALL,
      PACK_REMOTE_CONTEXT_CALL,
      HAS_CONFLICTING_INTERNAL_CALL,
      FIND_CONFLICTING_CALL,
      FIND_CONFLICTING_INTERNAL_CALL,
      CHECK_REGION_DEPENDENCE_CALL,
      FIND_PARENT_REGION_REQ_CALL,
      FIND_PARENT_REGION_CALL,
      CHECK_PRIVILEGE_CALL,
      TRIGGER_SINGLE_CALL,
      INITIALIZE_MAP_TASK_CALL,
      FINALIZE_MAP_TASK_CALL,
      VALIDATE_VARIANT_SELECTION_CALL,
      MAP_ALL_REGIONS_CALL,
      INITIALIZE_REGION_TREE_CONTEXTS_CALL,
      INVALIDATE_REGION_TREE_CONTEXTS_CALL,
      CREATE_INSTANCE_TOP_VIEW_CALL,
      LAUNCH_TASK_CALL,
      ACTIVATE_MULTI_CALL,
      DEACTIVATE_MULTI_CALL,
      SLICE_INDEX_SPACE_CALL,
      CLONE_MULTI_CALL,
      MULTI_TRIGGER_EXECUTION_CALL,
      PACK_MULTI_CALL,
      UNPACK_MULTI_CALL,
      ACTIVATE_INDIVIDUAL_CALL,
      DEACTIVATE_INDIVIDUAL_CALL,
      INDIVIDUAL_PERFORM_MAPPING_CALL,
      INDIVIDUAL_RETURN_VIRTUAL_CALL,
      INDIVIDUAL_TRIGGER_COMPLETE_CALL,
      INDIVIDUAL_TRIGGER_COMMIT_CALL,
      INDIVIDUAL_POST_MAPPED_CALL,
      INDIVIDUAL_PACK_TASK_CALL,
      INDIVIDUAL_UNPACK_TASK_CALL,
      INDIVIDUAL_PACK_REMOTE_COMPLETE_CALL,
      INDIVIDUAL_UNPACK_REMOTE_COMPLETE_CALL,
      POINT_ACTIVATE_CALL,
      POINT_DEACTIVATE_CALL,
      POINT_TASK_COMPLETE_CALL,
      POINT_TASK_COMMIT_CALL,
      POINT_PACK_TASK_CALL,
      POINT_UNPACK_TASK_CALL,
      POINT_TASK_POST_MAPPED_CALL,
      REMOTE_TASK_ACTIVATE_CALL,
      REMOTE_TASK_DEACTIVATE_CALL,
      REMOTE_UNPACK_CONTEXT_CALL,
      INDEX_ACTIVATE_CALL,
      INDEX_DEACTIVATE_CALL,
      INDEX_COMPUTE_FAT_PATH_CALL,
      INDEX_PREMAP_TASK_CALL,
      INDEX_DISTRIBUTE_CALL,
      INDEX_PERFORM_MAPPING_CALL,
      INDEX_COMPLETE_CALL,
      INDEX_COMMIT_CALL,
      INDEX_PERFORM_INLINING_CALL,
      INDEX_CLONE_AS_SLICE_CALL,
      INDEX_HANDLE_FUTURE,
      INDEX_RETURN_SLICE_MAPPED_CALL,
      INDEX_RETURN_SLICE_COMPLETE_CALL,
      INDEX_RETURN_SLICE_COMMIT_CALL,
      SLICE_ACTIVATE_CALL,
      SLICE_DEACTIVATE_CALL,
      SLICE_APPLY_VERSION_INFO_CALL,
      SLICE_DISTRIBUTE_CALL,
      SLICE_PERFORM_MAPPING_CALL,
      SLICE_LAUNCH_CALL,
      SLICE_MAP_AND_LAUNCH_CALL,
      SLICE_PACK_TASK_CALL,
      SLICE_UNPACK_TASK_CALL,
      SLICE_CLONE_AS_SLICE_CALL,
      SLICE_HANDLE_FUTURE_CALL,
      SLICE_CLONE_AS_POINT_CALL,
      SLICE_ENUMERATE_POINTS_CALL,
      SLICE_MAPPED_CALL,
      SLICE_COMPLETE_CALL,
      SLICE_COMMIT_CALL,
      REALM_SPAWN_META_CALL,
      REALM_SPAWN_TASK_CALL,
      REALM_CREATE_INSTANCE_CALL,
      REALM_ISSUE_COPY_CALL,
      REALM_ISSUE_FILL_CALL,
      REGION_TREE_LOGICAL_ANALYSIS_CALL,
      REGION_TREE_LOGICAL_FENCE_CALL,
      REGION_TREE_VERSIONING_ANALYSIS_CALL,
      REGION_TREE_ADVANCE_VERSION_NUMBERS_CALL,
      REGION_TREE_INITIALIZE_CONTEXT_CALL,
      REGION_TREE_INVALIDATE_CONTEXT_CALL,
      REGION_TREE_PREMAP_ONLY_CALL,
      REGION_TREE_PHYSICAL_REGISTER_ONLY_CALL,
      REGION_TREE_PHYSICAL_REGISTER_USERS_CALL,
      REGION_TREE_PHYSICAL_PERFORM_CLOSE_CALL,
      REGION_TREE_PHYSICAL_CLOSE_CONTEXT_CALL,
      REGION_TREE_PHYSICAL_COPY_ACROSS_CALL,
      REGION_TREE_PHYSICAL_REDUCE_ACROSS_CALL,
      REGION_TREE_PHYSICAL_CONVERT_MAPPING_CALL,
      REGION_TREE_PHYSICAL_FILL_FIELDS_CALL,
      REGION_TREE_PHYSICAL_ATTACH_EXTERNAL_CALL,
      REGION_TREE_PHYSICAL_DETACH_EXTERNAL_CALL,
      REGION_NODE_REGISTER_LOGICAL_USER_CALL,
      REGION_NODE_CLOSE_LOGICAL_NODE_CALL,
      REGION_NODE_SIPHON_LOGICAL_CHILDREN_CALL,
      REGION_NODE_SIPHON_LOGICAL_PROJECTION_CALL,
      REGION_NODE_PERFORM_LOGICAL_CLOSES_CALL,
      REGION_NODE_FIND_VALID_INSTANCE_VIEWS_CALL,
      REGION_NODE_FIND_VALID_REDUCTION_VIEWS_CALL,
      REGION_NODE_ISSUE_UPDATE_COPIES_CALL,
      REGION_NODE_SORT_COPY_INSTANCES_CALL,
      REGION_NODE_ISSUE_GROUPED_COPIES_CALL,
      REGION_NODE_ISSUE_UPDATE_REDUCTIONS_CALL,
      REGION_NODE_PREMAP_REGION_CALL,
      REGION_NODE_REGISTER_REGION_CALL,
      REGION_NODE_CLOSE_STATE_CALL,
      CURRENT_STATE_RECORD_VERSION_NUMBERS_CALL,
      CURRENT_STATE_ADVANCE_VERSION_NUMBERS_CALL,
      PHYSICAL_STATE_CAPTURE_STATE_CALL,
      PHYSICAL_STATE_APPLY_PATH_ONLY_CALL,
      PHYSICAL_STATE_APPLY_STATE_CALL,
      PHYSICAL_STATE_MAKE_LOCAL_CALL,
      MATERIALIZED_VIEW_FIND_LOCAL_PRECONDITIONS_CALL,
      MATERIALIZED_VIEW_FIND_LOCAL_COPY_PRECONDITIONS_CALL,
      MATERIALIZED_VIEW_FILTER_PREVIOUS_USERS_CALL,
      MATERIALIZED_VIEW_FILTER_CURRENT_USERS_CALL,
      MATERIALIZED_VIEW_FILTER_LOCAL_USERS_CALL,
      REDUCTION_VIEW_PERFORM_REDUCTION_CALL,
      REDUCTION_VIEW_PERFORM_DEFERRED_REDUCTION_CALL,
      REDUCTION_VIEW_PERFORM_DEFERRED_REDUCTION_ACROSS_CALL,
      REDUCTION_VIEW_FIND_COPY_PRECONDITIONS_CALL,
      REDUCTION_VIEW_FIND_USER_PRECONDITIONS_CALL,
      REDUCTION_VIEW_FILTER_LOCAL_USERS_CALL,
      PHYSICAL_TRACE_EXECUTE_CALL,
      PHYSICAL_TRACE_PRECONDITION_CHECK_CALL,
      PHYSICAL_TRACE_OPTIMIZE_CALL,
      LAST_RUNTIME_CALL_KIND, // This one must be last
    };

#define RUNTIME_CALL_DESCRIPTIONS(name)                               \
    const char *name[LAST_RUNTIME_CALL_KIND] = {                      \
      "MapperRuntime::send_message",                                  \
      "MapperRuntime::broadcast",                                     \
      "MapperRuntime::unpack_physical_instance",                      \
      "MapperRuntime::create_mapper_event",                           \
      "MapperRuntime::has_mapper_event_triggered",                    \
      "MapperRuntime::trigger_mapper_event",                          \
      "MapperRuntime::wait_on_mapper_event",                          \
      "MapperRuntime::find_execution_constraints",                    \
      "MapperRuntime::find_task_layout_constraints",                  \
      "MapperRuntime::find_layout_constraints",                       \
      "MapperRuntime::register_layout",                               \
      "MapperRuntime::release_layout",                                \
      "MapperRuntime::do_constraints_conflict",                       \
      "MapperRuntime::do_constraints_entail",                         \
      "MapperRuntime::find_valid_variants",                           \
      "MapperRuntime::find_task_variant_name",                        \
      "MapperRuntime::is_leaf_variant",                               \
      "MapperRuntime::is_inner_variant",                              \
      "MapperRuntime::is_idempotent_variant",                         \
      "MapperRuntime::is_replicable_variant",                         \
      "MapperRuntime::register_task_variant",                         \
      "MapperRuntime::filter_variants",                               \
      "MapperRuntime::filter_instances",                              \
      "MapperRuntime::create_physical_instance",                      \
      "MapperRuntime::find_or_create_physical_instance",              \
      "MapperRuntime::find_physical_instance",                        \
      "MapperRuntime::find_physical_instances",                       \
      "MapperRuntime::set_garbage_collection_priority",               \
      "MapperRuntime::acquire_instance",                              \
      "MapperRuntime::acquire_instances",                             \
      "MapperRuntime::acquire_and_filter_instances",                  \
      "MapperRuntime::release_instance",                              \
      "MapperRuntime::release_instances",                             \
      "MapperRuntime::acquire_future",                                \
      "MapperRuntime::create_index_space",                            \
      "MapperRuntime::union_index_spaces",                            \
      "MapperRuntime::intersect_index_spaces",                        \
      "MapperRuntime::subtract_index_spaces",                         \
      "MapperRuntime::is_index_space_empty",                          \
      "MapperRuntime::index_spaces_overlap",                          \
      "MapperRuntime::index_space_dominates",                         \
      "MapperRuntime::has_index_partition",                           \
      "MapperRuntime::get_index_partition",                           \
      "MapperRuntime::get_index_subspace",                            \
      "MapperRuntime::get_index_space_domain",                        \
      "MapperRuntime::get_index_partition_color_space",               \
      "MapperRuntime::get_index_partition_color_space_name",          \
      "MapperRuntime::get_index_space_parition_colors",               \
      "MapperRuntime::is_index_partition_disjoint",                   \
      "MapperRuntime::is_index_partition_complete",                   \
      "MapperRuntime::get_index_space_color",                         \
      "MapperRuntime::get_index_space_color_point",                   \
      "MapperRuntime::get_index_partition_color",                     \
      "MapperRuntime::get_parent_index_space",                        \
      "MapperRuntime::has_parent_index_partition",                    \
      "MapperRuntime::get_parent_index_partition",                    \
      "MapperRuntime::get_index_space_depth",                         \
      "MapperRuntime::get_index_partition_depth",                     \
      "MapperRuntime::get_field_size",                                \
      "MapperRuntime::get_field_space_fields",                        \
      "MapperRuntime::get_logical_partition",                         \
      "MapperRuntime::get_logical_partition_by_color",                \
      "MapperRuntime::get_logical_partition_by_tree",                 \
      "MapperRuntime::get_logical_subregion",                         \
      "MapperRuntime::get_logical_subregion_by_color",                \
      "MapperRuntime::get_logical_subregion_by_tree",                 \
      "MapperRuntime::get_logical_region_color",                      \
      "MapperRuntime::get_logical_region_color_point",                \
      "MapperRuntime::get_logical_partition_color",                   \
      "MapperRuntime::get_parent_logical_region",                     \
      "MapperRuntime::has_parent_logical_partition",                  \
      "MapperRuntime::get_parent_logical_partition",                  \
      "MapperRuntime::retrieve_semantic_information",                 \
      "MapperRuntime::retrieve_name",                                 \
      "MapperRuntime::AutoLock",                                      \
      "Pack Base Task",                                               \
      "Unpack Base Task",                                             \
      "Task Privilege Check",                                         \
      "Clone Base Task",                                              \
      "Compute Point Requirements",                                   \
      "Intra-Task Aliasing",                                          \
      "Activate Single",                                              \
      "Deactivate Single",                                            \
      "Select Inline Variant",                                        \
      "Inline Child Task",                                            \
      "Pack Single Task",                                             \
      "Unpack Single Task",                                           \
      "Pack Remote Context",                                          \
      "Has Conflicting Internal",                                     \
      "Find Conflicting",                                             \
      "Find Conflicting Internal",                                    \
      "Check Region Dependence",                                      \
      "Find Parent Region Requirement",                               \
      "Find Parent Region",                                           \
      "Check Privilege",                                              \
      "Trigger Single",                                               \
      "Initialize Map Task",                                          \
      "Finalized Map Task",                                           \
      "Validate Variant Selection",                                   \
      "Map All Regions",                                              \
      "Initialize Region Tree Contexts",                              \
      "Invalidate Region Tree Contexts",                              \
      "Create Instance Top View",                                     \
      "Launch Task",                                                  \
      "Activate Multi",                                               \
      "Deactivate Multi",                                             \
      "Slice Index Space",                                            \
      "Clone Multi Call",                                             \
      "Multi Trigger Execution",                                      \
      "Pack Multi",                                                   \
      "Unpack Multi",                                                 \
      "Activate Individual",                                          \
      "Deactivate Individual",                                        \
      "Individual Perform Mapping",                                   \
      "Individual Return Virtual",                                    \
      "Individual Trigger Complete",                                  \
      "Individual Trigger Commit",                                    \
      "Individual Post Mapped",                                       \
      "Individual Pack Task",                                         \
      "Individual Unpack Task",                                       \
      "Individual Pack Remote Complete",                              \
      "Individual Unpack Remote Complete",                            \
      "Activate Point",                                               \
      "Deactivate Point",                                             \
      "Point Task Complete",                                          \
      "Point Task Commit",                                            \
      "Point Task Pack",                                              \
      "Point Task Unpack",                                            \
      "Point Task Post Mapped",                                       \
      "Remote Task Activate",                                         \
      "Remote Task Deactivate",                                       \
      "Remote Unpack Context",                                        \
      "Index Activate",                                               \
      "Index Deactivate",                                             \
      "Index Compute Fat Path",                                       \
      "Index PreMap Task",                                            \
      "Index Distribute",                                             \
      "Index Perform Mapping",                                        \
      "Index Complete",                                               \
      "Index Commit",                                                 \
      "Index Perform Inlining",                                       \
      "Index Clone As Slice",                                         \
      "Index Handle Future",                                          \
      "Index Return Slice Mapped",                                    \
      "Index Return Slice Complete",                                  \
      "Index Return Slice Commit",                                    \
      "Slice Activate",                                               \
      "Slice Deactivate",                                             \
      "Slice Apply Version Info",                                     \
      "Slice Distribute",                                             \
      "Slice Perform Mapping",                                        \
      "Slice Launch",                                                 \
      "Slice Map and Launch",                                         \
      "Slice Pack Task",                                              \
      "Slice Unpack Task",                                            \
      "Slice Clone As Slice",                                         \
      "Slice Handle Future",                                          \
      "Slice Cone as Point",                                          \
      "Slice Enumerate Points",                                       \
      "Slice Mapped",                                                 \
      "Slice Complete",                                               \
      "Slice Commit",                                                 \
      "Realm Spawn Meta",                                             \
      "Realm Spawn Task",                                             \
      "Realm Create Instance",                                        \
      "Realm Issue Copy",                                             \
      "Realm Issue Fill",                                             \
      "Region Tree Logical Analysis",                                 \
      "Region Tree Logical Fence",                                    \
      "Region Tree Versioning Analysis",                              \
      "Region Tree Advance Version Numbers",                          \
      "Region Tree Initialize Context",                               \
      "Region Tree Invalidate Context",                               \
      "Region Tree Premap Only",                                      \
      "Region Tree Physical Register Only",                           \
      "Region Tree Physical Register Users",                          \
      "Region Tree Physical Perform Close",                           \
      "Region Tree Physical Close Context",                           \
      "Region Tree Physical Copy Across",                             \
      "Region Tree Physical Reduce Across",                           \
      "Region Tree Physical Convert Mapping",                         \
      "Region Tree Physical Fill Fields",                             \
      "Region Tree Physical Attach External",                         \
      "Region Tree Physical Detach External",                         \
      "Region Node Register Logical User",                            \
      "Region Node Close Logical Node",                               \
      "Region Node Siphon Logical Children",                          \
      "Region Node Siphon Logical Projection",                        \
      "Region Node Perform Logical Closes",                           \
      "Region Node Find Valid Instance Views",                        \
      "Region Node Find Valid Reduction Views",                       \
      "Region Node Issue Update Copies",                              \
      "Region Node Sort Copy Instances",                              \
      "Region Node Issue Grouped Copies",                             \
      "Region Node Issue Update Reductions",                          \
      "Region Node Premap Region",                                    \
      "Region Node Register Region",                                  \
      "Region Node Close State",                                      \
      "Logical State Record Verison Numbers",                         \
      "Logical State Advance Version Numbers",                        \
      "Physical State Capture State",                                 \
      "Physical State Apply Path Only",                               \
      "Physical State Apply State",                                   \
      "Physical State Make Local",                                    \
      "Materialized View Find Local Preconditions",                   \
      "Materialized View Find Local Copy Preconditions",              \
      "Materialized View Filter Previous Users",                      \
      "Materialized View Filter Current Users",                       \
      "Materialized View Filter Local Users",                         \
      "Reduction View Perform Reduction",                             \
      "Reduction View Perform Deferred Reduction",                    \
      "Reduction View Perform Deferred Reduction Across",             \
      "Reduction View Find Copy Preconditions",                       \
      "Reduction View Find User Preconditions",                       \
      "Reduction View Filter Local Users",                            \
      "Physical Trace Execute",                                       \
      "Physical Trace Precondition Check",                            \
      "Physical Trace Optimize",                                      \
    };

    enum SemanticInfoKind {
      INDEX_SPACE_SEMANTIC,
      INDEX_PARTITION_SEMANTIC,
      FIELD_SPACE_SEMANTIC,
      FIELD_SEMANTIC,
      LOGICAL_REGION_SEMANTIC,
      LOGICAL_PARTITION_SEMANTIC,
      TASK_SEMANTIC,
    };

    // Static locations for where collectives are allocated
    // These are just arbitrary numbers but they should appear
    // with at most one logical static collective kind
    // Ones that have been commented out are free to be reused
    enum CollectiveIndexLocation {
      //COLLECTIVE_LOC_0 = 0, 
      COLLECTIVE_LOC_1 = 1,
      COLLECTIVE_LOC_2 = 2,
      COLLECTIVE_LOC_3 = 3,
      COLLECTIVE_LOC_4 = 4, 
      COLLECTIVE_LOC_5 = 5,
      COLLECTIVE_LOC_6 = 6,
      COLLECTIVE_LOC_7 = 7,
      COLLECTIVE_LOC_8 = 8, 
      COLLECTIVE_LOC_9 = 9,
      COLLECTIVE_LOC_10 = 10,
      COLLECTIVE_LOC_11 = 11, 
      COLLECTIVE_LOC_12 = 12, 
      COLLECTIVE_LOC_13 = 13,
      COLLECTIVE_LOC_14 = 14,
      COLLECTIVE_LOC_15 = 15,
      COLLECTIVE_LOC_16 = 16,
      COLLECTIVE_LOC_17 = 17, 
      COLLECTIVE_LOC_18 = 18, 
      COLLECTIVE_LOC_19 = 19,
      COLLECTIVE_LOC_20 = 20,
      COLLECTIVE_LOC_21 = 21, 
      COLLECTIVE_LOC_22 = 22, 
      COLLECTIVE_LOC_23 = 23,
      COLLECTIVE_LOC_24 = 24,
      COLLECTIVE_LOC_25 = 25,
      COLLECTIVE_LOC_26 = 26,
      COLLECTIVE_LOC_27 = 27, 
      COLLECTIVE_LOC_28 = 28, 
      COLLECTIVE_LOC_29 = 29,
      COLLECTIVE_LOC_30 = 30,
      COLLECTIVE_LOC_31 = 31, 
      COLLECTIVE_LOC_32 = 32,
      COLLECTIVE_LOC_33 = 33,
      COLLECTIVE_LOC_34 = 34,
      COLLECTIVE_LOC_35 = 35,
      COLLECTIVE_LOC_36 = 36,
      COLLECTIVE_LOC_37 = 37, 
      COLLECTIVE_LOC_38 = 38, 
      COLLECTIVE_LOC_39 = 39,
      COLLECTIVE_LOC_40 = 40,
      COLLECTIVE_LOC_41 = 41,
      COLLECTIVE_LOC_42 = 42,
      COLLECTIVE_LOC_43 = 43,
      COLLECTIVE_LOC_44 = 44,
      COLLECTIVE_LOC_45 = 45,
      COLLECTIVE_LOC_46 = 46,
      COLLECTIVE_LOC_47 = 47,
      COLLECTIVE_LOC_48 = 48,
      COLLECTIVE_LOC_49 = 49,
      COLLECTIVE_LOC_50 = 50,
      COLLECTIVE_LOC_51 = 51,
      COLLECTIVE_LOC_52 = 52,
      COLLECTIVE_LOC_53 = 53,
      COLLECTIVE_LOC_54 = 54,
      COLLECTIVE_LOC_55 = 55,
      COLLECTIVE_LOC_56 = 56,
      COLLECTIVE_LOC_57 = 57,
      COLLECTIVE_LOC_58 = 58,
      COLLECTIVE_LOC_59 = 59,
      COLLECTIVE_LOC_60 = 60,
      COLLECTIVE_LOC_61 = 61,
      COLLECTIVE_LOC_62 = 62,
      COLLECTIVE_LOC_63 = 63,
      COLLECTIVE_LOC_64 = 64,
      COLLECTIVE_LOC_65 = 65,
      COLLECTIVE_LOC_66 = 66,
      //COLLECTIVE_LOC_67 = 67,
      //COLLECTIVE_LOC_68 = 68,
      //COLLECTIVE_LOC_69 = 69,
      COLLECTIVE_LOC_70 = 70,
      COLLECTIVE_LOC_71 = 71,
      COLLECTIVE_LOC_72 = 72,
      COLLECTIVE_LOC_73 = 73,
      COLLECTIVE_LOC_74 = 74,
      COLLECTIVE_LOC_75 = 75,
      //COLLECTIVE_LOC_76 = 76,
      COLLECTIVE_LOC_77 = 77,
      COLLECTIVE_LOC_78 = 78,
      COLLECTIVE_LOC_79 = 79,
      COLLECTIVE_LOC_80 = 80,
      COLLECTIVE_LOC_81 = 81,
      COLLECTIVE_LOC_82 = 82,
      COLLECTIVE_LOC_83 = 83,
      COLLECTIVE_LOC_84 = 84,
      COLLECTIVE_LOC_85 = 85,
      COLLECTIVE_LOC_86 = 86,
      COLLECTIVE_LOC_87 = 87,
      COLLECTIVE_LOC_88 = 88,
      COLLECTIVE_LOC_89 = 89,
      COLLECTIVE_LOC_90 = 90,
      COLLECTIVE_LOC_91 = 91,
      COLLECTIVE_LOC_92 = 92,
      COLLECTIVE_LOC_93 = 93,
      COLLECTIVE_LOC_94 = 94,
      COLLECTIVE_LOC_95 = 95,
      COLLECTIVE_LOC_96 = 96,
      COLLECTIVE_LOC_97 = 97,
      COLLECTIVE_LOC_98 = 98,
      COLLECTIVE_LOC_99 = 99,
      COLLECTIVE_LOC_100 = 100,
      COLLECTIVE_LOC_101 = 101,
      COLLECTIVE_LOC_102 = 102,
      COLLECTIVE_LOC_103 = 103,
      COLLECTIVE_LOC_104 = 104,
      COLLECTIVE_LOC_105 = 105,
    };

    // legion_types.h
    class LocalLock;
    class AutoLock;
    class AutoTryLock;
    class LgEvent; // base event type for legion
    class ApEvent; // application event
    class ApUserEvent; // application user event
    class ApBarrier; // application barrier
    class RtEvent; // runtime event
    class RtUserEvent; // runtime user event
    class RtBarrier;

    // legion_utilities.h
    struct RegionUsage; 
    template<typename T> class Fraction;
    template<typename T, unsigned LOG2MAX> class BitPermutation;

    // Forward declarations for runtime level objects
    // runtime.h
    class Collectable;
    class FieldAllocatorImpl;
    class ArgumentMapImpl;
    class FutureImpl;
    class FutureInstance;
    class FutureMapImpl;
    class ReplFutureMapImpl;
    class PhysicalRegionImpl;
    class OutputRegionImpl;
    class ExternalResourcesImpl;
    class PieceIteratorImpl;
    class GrantImpl;
    class PredicateImpl;
    class LegionHandshakeImpl;
    class ProcessorManager;
    class MemoryManager;
    class VirtualChannel;
    class MessageManager;
    class ShutdownManager;
    class TaskImpl;
    class VariantImpl;
    class LayoutConstraints;
    class ProjectionFunction;
    class ShardingFunction;
    class Runtime;
    // A small interface class for handling profiling responses
    struct ProfilingResponseBase;
    class ProfilingResponseHandler {
    public:
      virtual void handle_profiling_response(
                const ProfilingResponseBase *base,
                const Realm::ProfilingResponse &response,
                const void *orig, size_t orig_length) = 0;
    };
    struct ProfilingResponseBase {
    public:
      ProfilingResponseBase(ProfilingResponseHandler *h)
        : handler(h) { }
    public:
      ProfilingResponseHandler *const handler;
    };

    // legion_ops.h
    class Provenance;
    class Operation;
    class MemoizableOp;
    class PredicatedOp;
    class MapOp;
    class CopyOp;
    class IndexCopyOp;
    class PointCopyOp;
    class FenceOp;
    class FrameOp;
    class CreationOp;
    class DeletionOp;
    class InternalOp;
    class CloseOp;
    class MergeCloseOp;
    class PostCloseOp;
    class VirtualCloseOp;
    class RefinementOp;
    class ResetOp;
    class AcquireOp;
    class ReleaseOp;
    class DynamicCollectiveOp;
    class FuturePredOp;
    class NotPredOp;
    class AndPredOp;
    class OrPredOp;
    class MustEpochOp;
    class PendingPartitionOp;
    class DependentPartitionOp;
    class PointDepPartOp;
    class FillOp;
    class IndexFillOp;
    class PointFillOp;
    class DiscardOp;
    class AttachOp;
    class IndexAttachOp;
    class PointAttachOp;
    class DetachOp;
    class IndexDetachOp;
    class PointDetachOp;
    class TimingOp;
    class TunableOp;
    class AllReduceOp;
    class ExternalMappable;
    class RemoteOp;
    class RemoteMapOp;
    class RemoteCopyOp;
    class RemoteCloseOp;
    class RemoteAcquireOp;
    class RemoteReleaseOp;
    class RemoteFillOp;
    class RemotePartitionOp;
    class RemoteReplayOp;
    class RemoteSummaryOp;
    template<typename OP>
    class Memoizable;
    template<typename OP>
    class Predicated;


    // legion_tasks.h
    class ExternalTask;
    class TaskOp;
    class RemoteTaskOp;
    class SingleTask;
    class MultiTask;
    class IndividualTask;
    class PointTask;
    class ShardTask;
    class IndexTask;
    class SliceTask;
    class RemoteTask;

    // legion_context.h
    class TaskContext;
    class InnerContext;;
    class TopLevelContext;
    class ReplicateContext;
    class RemoteContext;
    class LeafContext;

    // legion_trace.h
    class LogicalTrace;
    class TraceCaptureOp;
    class TraceCompleteOp;
    class TraceReplayOp;
    class TraceBeginOp;
    class TraceSummaryOp;
    class PhysicalTrace;
    class TraceViewSet;
    class TraceConditionSet;
    class PhysicalTemplate;
    class ShardedPhysicalTemplate;
    class Instruction;
    class GetTermEvent;
    class ReplayMapping;
    class CreateApUserEvent;
    class TriggerEvent;
    class MergeEvent;
    class AssignFenceCompletion;
    class IssueCopy;
    class IssueFill;
    class IssueAcross;
    class GetOpTermEvent;
    class SetOpSyncEvent;
    class SetEffects;
    class CompleteReplay;
    class AcquireReplay;
    class ReleaseReplay;
    class BarrierArrival;
    class BarrierAdvance;

    // region_tree.h
    class RegionTreeForest;
    class CopyAcrossExecutor;
    class CopyAcrossUnstructured;
    class IndexSpaceExpression;
    class IndexSpaceExprRef;
    class IndexSpaceOperation;
    template<int DIM, typename T> class IndexSpaceOperationT;
    template<int DIM, typename T> class IndexSpaceUnion;
    template<int DIM, typename T> class IndexSpaceIntersection;
    template<int DIM, typename T> class IndexSpaceDifference;
    class ExpressionTrieNode;
    class IndexTreeNode;
    class IndexSpaceNode;
    template<int DIM, typename T> class IndexSpaceNodeT;
    class IndexPartNode;
    template<int DIM, typename T> class IndexPartNodeT;
    class FieldSpaceNode;
    class RegionTreeNode;
    class RegionNode;
    class PartitionNode;
    class ColorSpaceIterator;
    template<int DIM, typename T> class ColorSpaceLinearizationT;
    template<int DIM, typename T, typename RT = void> class KDNode;
    class EqKDTree;
    template<int DIM, typename T> class EqKDTreeT;

    class RegionTreePath;
    class PathTraverser;
    class NodeTraverser;

    class LogicalState;
    class LogicalAnalysis;
    class PhysicalAnalysis;
    class EquivalenceSet;
    class EqSetTracker;
    class VersionManager;
    class VersionInfo;
    class ProjectionNode;
    class ProjectionRegion;
    class ProjectionPartition;
    class RefinementTracker;
    class RegionRefinementTracker;
    class PartitionRefinementTracker;

    class Collectable;
    class Notifiable;
    class ImplicitReferenceTracker;
    class DistributedCollectable;
    class LayoutDescription;
    class InstanceManager; // base class for all instances
    class CopyAcrossHelper;
    class LogicalView; // base class for instance and reduction
    class InstanceKey;
    class InstanceView;
    class CollectableView; // pure virtual class
    class IndividualView;
    class CollectiveView;
    class MaterializedView;
    class ReplicatedView;
    class ReductionView;
    class AllreduceView;
    class DeferredView;
    class FillView;
    class PhiView;
    class MappingRef;
    class InstanceRef;
    class InstanceSet;
    class InnerTaskView;
    class VirtualManager;
    class PhysicalManager;
    class InstanceBuilder;

    class RegionAnalyzer;
    class RegionMapper;

    struct LogicalUser;
    struct PhysicalUser;
    struct LogicalTraceInfo;
    struct PhysicalTraceInfo;
    class TreeCloseImpl;
    class TreeClose;
    struct CloseInfo; 
    struct FieldDataDescriptor;
    struct PendingRemoteExpression;
    class ProjectionSummary;
    class ProjectionInfo;


    // legion_spy.h
    class TreeStateLogger;

    // legion_profiling.h
    class LegionProfiler;
    class LegionProfInstance;

    // mapper_manager.h
    class MappingCallInfo;
    class MapperManager;
    class SerializingManager;
    class ConcurrentManager;
    typedef Mapping::MapperEvent MapperEvent;
    typedef Mapping::ProfilingMeasurementID ProfilingMeasurementID;

    // legion_replication.h
    class ShardedMapping;
    class ReplIndividualTask;
    class ReplIndexTask;
    class ReplMergeCloseOp;
    class ReplVirtualCloseOp;
    class ReplRefinementOp;
    class ReplResetOp;
    class ReplFillOp;
    class ReplIndexFillOp;
    class ReplDiscardOp;
    class ReplCopyOp;
    class ReplIndexCopyOp;
    class ReplDeletionOp;
    class ReplPendingPartitionOp;
    class ReplDependentPartitionOp;
    class ReplMustEpochOp;
    class ReplTimingOp;
    class ReplTunableOp;
    class ReplAllReduceOp;
    class ReplFenceOp;
    class ReplMapOp;
    class ReplAttachOp;
    class ReplIndexAttachOp;
    class ReplDetachOp;
    class ReplIndexDetachOp;
    class ReplAcquireOp;
    class ReplReleaseOp;
    class ReplTraceOp;
    class ReplTraceCaptureOp;
    class ReplTraceCompleteOp;
    class ReplTraceReplayOp;
    class ReplTraceBeginOp;
    class ReplTraceSummaryOp;
    class ShardMapping;
    class CollectiveMapping;
    class ShardManager;
    class ShardCollective;
    class GatherCollective;
    template<bool>
    class AllGatherCollective;
    template<typename T> class BarrierExchangeCollective;
    template<typename T> class ValueBroadcast;
    template<typename T> class AllReduceCollective;
    class CrossProductCollective;
    class ShardingGatherCollective;
    class FieldDescriptorExchange;
    class FieldDescriptorGather;
    class FutureBroadcast;
    class FutureExchange;
    class FutureNameExchange;
    class MustEpochMappingBroadcast;
    class MustEpochMappingExchange;
    class PredicateCollective;
    class UnorderedExchange;
    class ShardRendezvous;
    class ProjectionTreeExchange;
    class TimeoutMatchExchange;

    // Nasty global variable for TLS support of figuring out
    // our context implicitly
    extern thread_local TaskContext *implicit_context;
    // Same thing for the runtime
    extern thread_local Runtime *implicit_runtime;
    // Another nasty global variable for tracking the fast
    // reservations that we are holding
    extern thread_local AutoLock *local_lock_list;
    // One more nasty global variable that we use for tracking
    // the provenance of meta-task operations for profiling
    // purposes, this has no bearing on correctness
    extern thread_local ::legion_unique_id_t implicit_provenance;
    // Use this to track if we're inside of a registration 
    // callback function which we know to be deduplicated
    enum RegistrationCallbackMode {
      NO_REGISTRATION_CALLBACK = 0,
      LOCAL_REGISTRATION_CALLBACK = 1,
      GLOBAL_REGISTRATION_CALLBACK = 2,
    };
    extern thread_local unsigned inside_registration_callback;
    // This data structure tracks references to any live
    // temporary index space expressions that have been
    // handed back by the region tree inside the execution
    // of a meta-task or a runtime API call. It also tracks
    // changes to remote distributed collectable that can be
    // delayed and batched together.
    extern thread_local ImplicitReferenceTracker *implicit_reference_tracker; 
#ifdef DEBUG_LEGION_WAITS
    extern thread_local int meta_task_id;
#endif
#ifdef DEBUG_LEGION_CALLERS
    extern thread_local LgTaskID implicit_task_kind;
    extern thread_local LgTaskID implicit_task_caller;
#endif

    /**
     * \class LgTaskArgs
     * The base class for all Legion Task arguments
     */
    template<typename T>
    struct LgTaskArgs {
    public:
      LgTaskArgs(::legion_unique_id_t uid)
        : provenance(uid),
#ifdef DEBUG_LEGION_CALLERS
          lg_call_id(implicit_task_kind),
#endif
          lg_task_id(T::TASK_ID) { }
    public:
      // In this order for alignment reasons
      const ::legion_unique_id_t provenance;
#ifdef DEBUG_LEGION_CALLERS
      const LgTaskID lg_call_id;
#endif
      const LgTaskID lg_task_id;
    };

#define FRIEND_ALL_RUNTIME_CLASSES                          \
    friend class Legion::Runtime;                           \
    friend class Internal::Runtime;                         \
    friend class Internal::FutureImpl;                      \
    friend class Internal::FutureMapImpl;                   \
    friend class Internal::PhysicalRegionImpl;              \
    friend class Internal::ExternalResourcesImpl;           \
    friend class Internal::TaskImpl;                        \
    friend class Internal::VariantImpl;                     \
    friend class Internal::ProcessorManager;                \
    friend class Internal::MemoryManager;                   \
    friend class Internal::Operation;                       \
    friend class Internal::PredicatedOp;                    \
    friend class Internal::MapOp;                           \
    friend class Internal::CopyOp;                          \
    friend class Internal::IndexCopyOp;                     \
    friend class Internal::PointCopyOp;                     \
    friend class Internal::FenceOp;                         \
    friend class Internal::DynamicCollectiveOp;             \
    friend class Internal::FuturePredOp;                    \
    friend class Internal::CreationOp;                      \
    friend class Internal::DeletionOp;                      \
    friend class Internal::CloseOp;                         \
    friend class Internal::MergeCloseOp;                    \
    friend class Internal::PostCloseOp;                     \
    friend class Internal::VirtualCloseOp;                  \
    friend class Internal::RefinementOp;                    \
    friend class Internal::ResetOp;                         \
    friend class Internal::AcquireOp;                       \
    friend class Internal::ReleaseOp;                       \
    friend class Internal::PredicateImpl;                   \
    friend class Internal::NotPredOp;                       \
    friend class Internal::AndPredOp;                       \
    friend class Internal::OrPredOp;                        \
    friend class Internal::MustEpochOp;                     \
    friend class Internal::PendingPartitionOp;              \
    friend class Internal::DependentPartitionOp;            \
    friend class Internal::PointDepPartOp;                  \
    friend class Internal::FillOp;                          \
    friend class Internal::IndexFillOp;                     \
    friend class Internal::PointFillOp;                     \
    friend class Internal::DiscardOp;                       \
    friend class Internal::AttachOp;                        \
    friend class Internal::IndexAttachOp;                   \
    friend class Internal::ReplIndexAttachOp;               \
    friend class Internal::PointAttachOp;                   \
    friend class Internal::DetachOp;                        \
    friend class Internal::IndexDetachOp;                   \
    friend class Internal::ReplIndexDetachOp;               \
    friend class Internal::PointDetachOp;                   \
    friend class Internal::TimingOp;                        \
    friend class Internal::TunableOp;                       \
    friend class Internal::AllReduceOp;                     \
    friend class Internal::TraceSummaryOp;                  \
    friend class Internal::ExternalMappable;                \
    friend class Internal::ExternalTask;                    \
    friend class Internal::TaskOp;                          \
    friend class Internal::SingleTask;                      \
    friend class Internal::MultiTask;                       \
    friend class Internal::IndividualTask;                  \
    friend class Internal::PointTask;                       \
    friend class Internal::IndexTask;                       \
    friend class Internal::SliceTask;                       \
    friend class Internal::ReplIndividualTask;              \
    friend class Internal::ReplIndexTask;                   \
    friend class Internal::ReplFillOp;                      \
    friend class Internal::ReplIndexFillOp;                 \
    friend class Internal::ReplDiscardOp;                   \
    friend class Internal::ReplCopyOp;                      \
    friend class Internal::ReplIndexCopyOp;                 \
    friend class Internal::ReplDeletionOp;                  \
    friend class Internal::ReplPendingPartitionOp;          \
    friend class Internal::ReplDependentPartitionOp;        \
    friend class Internal::ReplMustEpochOp;                 \
    friend class Internal::ReplMapOp;                       \
    friend class Internal::ReplTimingOp;                    \
    friend class Internal::ReplTunableOp;                   \
    friend class Internal::ReplAllReduceOp;                 \
    friend class Internal::ReplFenceOp;                     \
    friend class Internal::ReplAttachOp;                    \
    friend class Internal::ReplDetachOp;                    \
    friend class Internal::ReplAcquireOp;                   \
    friend class Internal::ReplReleaseOp;                   \
    friend class Internal::MemoizableOp;                    \
    template<typename OP>                                   \
    friend class Internal::Memoizable;                      \
    friend class Internal::ShardManager;                    \
    friend class Internal::RegionTreeForest;                \
    friend class Internal::IndexSpaceNode;                  \
    template<int, typename>                                 \
    friend class Internal::IndexSpaceNodeT;                 \
    friend class Internal::IndexPartNode;                   \
    friend class Internal::FieldSpaceNode;                  \
    friend class Internal::RegionTreeNode;                  \
    friend class Internal::RegionNode;                      \
    friend class Internal::PartitionNode;                   \
    friend class Internal::LogicalView;                     \
    friend class Internal::InstanceView;                    \
    friend class Internal::DeferredView;                    \
    friend class Internal::ReductionView;                   \
    friend class Internal::MaterializedView;                \
    friend class Internal::FillView;                        \
    friend class Internal::LayoutDescription;               \
    friend class Internal::InstanceManager;                 \
    friend class Internal::PhysicalManager;                 \
    friend class Internal::TreeStateLogger;                 \
    friend class Internal::MapperManager;                   \
    friend class Internal::InstanceRef;                     \
    friend class Internal::LegionHandshakeImpl;             \
    friend class Internal::ArgumentMapImpl;                 \
    friend class Internal::FutureMapImpl;                   \
    friend class Internal::ReplFutureMapImpl;               \
    friend class Internal::TaskContext;                     \
    friend class Internal::InnerContext;                    \
    friend class Internal::TopLevelContext;                 \
    friend class Internal::RemoteContext;                   \
    friend class Internal::LeafContext;                     \
    friend class Internal::ReplicateContext;                \
    friend class Internal::InstanceBuilder;                 \
    friend class Internal::FutureNameExchange;              \
    friend class Internal::MustEpochMappingExchange;        \
    friend class Internal::MustEpochMappingBroadcast;       \
    friend class BindingLib::Utility;                       \
    friend class CObjectWrapper;                  

#define LEGION_EXTERN_LOGGER_DECLARATIONS      \
    extern Realm::Logger log_run;              \
    extern Realm::Logger log_task;             \
    extern Realm::Logger log_index;            \
    extern Realm::Logger log_field;            \
    extern Realm::Logger log_region;           \
    extern Realm::Logger log_inst;             \
    extern Realm::Logger log_variant;          \
    extern Realm::Logger log_allocation;       \
    extern Realm::Logger log_migration;        \
    extern Realm::Logger log_prof;             \
    extern Realm::Logger log_garbage;          \
    extern Realm::Logger log_spy;              \
    extern Realm::Logger log_shutdown;         \
    extern Realm::Logger log_tracing;

  }; // Internal namespace

  // Typedefs that are needed everywhere
  typedef Realm::Runtime RealmRuntime;
  typedef Realm::Machine Machine;
  typedef Realm::Memory Memory;
  typedef Realm::Processor Processor;
  typedef Realm::ProcessorGroup ProcessorGroup;
  typedef Realm::CodeDescriptor CodeDescriptor;
  typedef Realm::Reservation Reservation;
  typedef Realm::CompletionQueue CompletionQueue;
  typedef ::legion_reduction_op_id_t ReductionOpID;
  typedef Realm::ReductionOpUntyped ReductionOp;
  typedef ::legion_custom_serdez_id_t CustomSerdezID;
  typedef Realm::CustomSerdezUntyped SerdezOp;
  typedef Realm::Machine::ProcessorMemoryAffinity ProcessorMemoryAffinity;
  typedef Realm::Machine::MemoryMemoryAffinity MemoryMemoryAffinity;
  typedef Realm::DynamicTemplates::TagType TypeTag;
  typedef Realm::Logger Logger;
  typedef ::legion_coord_t coord_t;
  typedef std::map<CustomSerdezID, 
                   const Realm::CustomSerdezUntyped *> SerdezOpTable;
  typedef std::map<Realm::ReductionOpID, 
                   Realm::ReductionOpUntyped *> ReductionOpTable;
  typedef void (*SerdezInitFnptr)(const ReductionOp*, void *&, size_t&);
  typedef void (*SerdezFoldFnptr)(const ReductionOp*, void *&, 
                                  size_t&, const void*);
  typedef std::map<Realm::ReductionOpID, SerdezRedopFns> SerdezRedopTable;
  typedef ::legion_projection_type_t HandleType;
  typedef ::legion_address_space_t AddressSpace;
  typedef ::legion_task_priority_t TaskPriority;
  typedef ::legion_task_priority_t RealmPriority;
  typedef ::legion_garbage_collection_priority_t GCPriority;
  typedef ::legion_color_t Color;
  typedef ::legion_field_id_t FieldID;
  typedef ::legion_trace_id_t TraceID;
  typedef ::legion_mapper_id_t MapperID;
  typedef ::legion_context_id_t ContextID;
  typedef ::legion_instance_id_t InstanceID;
  typedef ::legion_index_space_id_t IndexSpaceID;
  typedef ::legion_index_partition_id_t IndexPartitionID;
  typedef ::legion_index_tree_id_t IndexTreeID;
  typedef ::legion_field_space_id_t FieldSpaceID;
  typedef ::legion_generation_id_t GenerationID;
  typedef ::legion_type_handle TypeHandle;
  typedef ::legion_projection_id_t ProjectionID;
  typedef ::legion_sharding_id_t ShardingID;
  typedef ::legion_region_tree_id_t RegionTreeID;
  typedef ::legion_distributed_id_t DistributedID;
  typedef ::legion_address_space_t AddressSpaceID;
  typedef ::legion_tunable_id_t TunableID;
  typedef ::legion_local_variable_id_t LocalVariableID;
  typedef ::legion_mapping_tag_id_t MappingTagID;
  typedef ::legion_semantic_tag_t SemanticTag;
  typedef ::legion_variant_id_t VariantID;
  typedef ::legion_code_descriptor_id_t CodeDescriptorID;
  typedef ::legion_unique_id_t UniqueID;
  typedef ::legion_version_id_t VersionID;
  typedef ::legion_projection_epoch_id_t ProjectionEpochID;
  typedef ::legion_task_id_t TaskID;
  typedef ::legion_layout_constraint_id_t LayoutConstraintID;
  typedef ::legion_shard_id_t ShardID;
  typedef ::legion_internal_color_t LegionColor;
  typedef void (*RegistrationCallbackFnptr)(Machine machine, 
                Runtime *rt, const std::set<Processor> &local_procs);
  typedef void (*RegistrationWithArgsCallbackFnptr)(
                const RegistrationCallbackArgs &args);
  typedef LogicalRegion (*RegionProjectionFnptr)(LogicalRegion parent, 
      const DomainPoint&, Runtime *rt);
  typedef LogicalRegion (*PartitionProjectionFnptr)(LogicalPartition parent, 
      const DomainPoint&, Runtime *rt);
  typedef bool (*PredicateFnptr)(const void*, size_t, 
      const std::vector<Future> futures);
  typedef void (*RealmFnptr)(const void*,size_t,
                             const void*,size_t,Processor);
  // Magical typedefs 
  // (don't forget to update ones in old HighLevel namespace in legion.inl)
  typedef Internal::TaskContext* Context;
  // Anothing magical typedef
  namespace Mapping {
    typedef Internal::MappingCallInfo* MapperContext;
    typedef Internal::InstanceManager* PhysicalInstanceImpl;
    typedef Internal::CollectiveView*  CollectiveViewImpl;
    // This type import is experimental to facilitate coordination and
    // synchronization between different mappers and may be revoked later
    // as we develop new abstractions for mappers to interact
    typedef Internal::LocalLock LocalLock;
  };

  namespace Internal { 
    // The invalid color
    const LegionColor INVALID_COLOR = LLONG_MAX;
    // This is only needed internally
    typedef Realm::RegionInstance PhysicalInstance;
    typedef Realm::CopySrcDstField CopySrcDstField;
    typedef unsigned long long CollectiveID;
    typedef unsigned long long IndexSpaceExprID;
    struct ContextCoordinate;
    typedef ContextCoordinate TraceLocalID;
    typedef std::vector<ContextCoordinate> TaskTreeCoordinates;
    // Helper for encoding templates
    struct NT_TemplateHelper : 
      public Realm::DynamicTemplates::ListProduct2<Realm::DIMCOUNTS, 
                                                   Realm::DIMTYPES> {
    typedef Realm::DynamicTemplates::ListProduct2<Realm::DIMCOUNTS, 
                                                  Realm::DIMTYPES> SUPER;
    public:
      template<int N, typename T> __CUDA_HD__
      static inline constexpr TypeTag encode_tag(void) {
#if __cplusplus >= 201402L
        constexpr TypeTag type =
          SUPER::template encode_tag<Realm::DynamicTemplates::Int<N>, T>();
        static_assert(type != 0, "All types should be non-zero for Legion");
        return type;
#else
        return SUPER::template encode_tag<Realm::DynamicTemplates::Int<N>, T>();
#endif
      }
      template<int N, typename T>
      static inline void check_type(const TypeTag t) {
#ifdef DEBUG_LEGION
#ifndef NDEBUG
        const TypeTag t1 = encode_tag<N,T>();
#endif
        assert(t1 == t);
#endif
      }
      struct DimHelper {
      public:
        template<typename N, typename T>
        static inline void demux(int *result) { *result = N::N; }
      };
      static inline int get_dim(const TypeTag t) {
        int result = 0;
        SUPER::demux<DimHelper>(t, &result);
        return result; 
      }
    };
    // Pull some of the mapper types into the internal space
    typedef Mapping::Mapper Mapper;
    typedef Mapping::PhysicalInstance MappingInstance;
    typedef Mapping::CollectiveView MappingCollective;
    // A little bit of logic here to figure out the 
    // kind of bit mask to use for FieldMask

// The folowing macros are used in the FieldMask instantiation of BitMask
// If you change one you probably have to change the others too
#define LEGION_FIELD_MASK_FIELD_TYPE          uint64_t 
#define LEGION_FIELD_MASK_FIELD_SHIFT         6
#define LEGION_FIELD_MASK_FIELD_MASK          0x3F
#define LEGION_FIELD_MASK_FIELD_ALL_ONES      0xFFFFFFFFFFFFFFFF

#if defined(__AVX__)
#if (LEGION_MAX_FIELDS > 256)
    typedef AVXTLBitMask<LEGION_MAX_FIELDS> FieldMask;
#elif (LEGION_MAX_FIELDS > 128)
    typedef AVXBitMask<LEGION_MAX_FIELDS> FieldMask;
#elif (LEGION_MAX_FIELDS > 64)
    typedef SSEBitMask<LEGION_MAX_FIELDS> FieldMask;
#else
    typedef BitMask<LEGION_FIELD_MASK_FIELD_TYPE,LEGION_MAX_FIELDS,
                    LEGION_FIELD_MASK_FIELD_SHIFT,
                    LEGION_FIELD_MASK_FIELD_MASK> FieldMask;
#endif
#elif defined(__SSE2__)
#if (LEGION_MAX_FIELDS > 128)
    typedef SSETLBitMask<LEGION_MAX_FIELDS> FieldMask;
#elif (LEGION_MAX_FIELDS > 64)
    typedef SSEBitMask<LEGION_MAX_FIELDS> FieldMask;
#else
    typedef BitMask<LEGION_FIELD_MASK_FIELD_TYPE,LEGION_MAX_FIELDS,
                    LEGION_FIELD_MASK_FIELD_SHIFT,
                    LEGION_FIELD_MASK_FIELD_MASK> FieldMask;
#endif
#elif defined(__ALTIVEC__)
#if (LEGION_MAX_FIELDS > 128)
    typedef PPCTLBitMask<LEGION_MAX_FIELDS> FieldMask;
#elif (LEGION_MAX_FIELDS > 64)
    typedef PPCBitMask<LEGION_MAX_FIELDS> FieldMask;
#else
    typedef BitMask<LEGION_FIELD_MASK_FIELD_TYPE,LEGION_MAX_FIELDS,
                    LEGION_FIELD_MASK_FIELD_SHIFT,
                    LEGION_FIELD_MASK_FIELD_MASK> FieldMask;
#endif
#elif defined(__ARM_NEON)
#if (LEGION_MAX_FIELDS > 128)
    typedef NeonTLBitMask<LEGION_MAX_FIELDS> FieldMask;
#elif (LEGION_MAX_FIELDS > 64)
    typedef NeonBitMask<LEGION_MAX_FIELDS> FieldMask;
#else
    typedef BitMask<LEGION_FIELD_MASK_FIELD_TYPE,LEGION_MAX_FIELDS,
                    LEGION_FIELD_MASK_FIELD_SHIFT,
                    LEGION_FIELD_MASK_FIELD_MASK> FieldMask;
#endif
#else
#if (LEGION_MAX_FIELDS > 64)
    typedef TLBitMask<LEGION_FIELD_MASK_FIELD_TYPE,LEGION_MAX_FIELDS,
                      LEGION_FIELD_MASK_FIELD_SHIFT,
                      LEGION_FIELD_MASK_FIELD_MASK> FieldMask;
#else
    typedef BitMask<LEGION_FIELD_MASK_FIELD_TYPE,LEGION_MAX_FIELDS,
                    LEGION_FIELD_MASK_FIELD_SHIFT,
                    LEGION_FIELD_MASK_FIELD_MASK> FieldMask;
#endif
#endif
    typedef BitPermutation<FieldMask,LEGION_FIELD_LOG2> FieldPermutation;
    typedef Fraction<unsigned long> InstFrac;
#undef LEGION_FIELD_MASK_FIELD_SHIFT
#undef LEGION_FIELD_MASK_FIELD_MASK

    // Similar logic as field masks for node masks

// The following macros are used in the NodeMask instantiation of BitMask
// If you change one you probably have to change the others too
#define LEGION_NODE_MASK_NODE_TYPE           uint64_t
#define LEGION_NODE_MASK_NODE_SHIFT          6
#define LEGION_NODE_MASK_NODE_MASK           0x3F
#define LEGION_NODE_MASK_NODE_ALL_ONES       0xFFFFFFFFFFFFFFFF

#if defined(__AVX__)
#if (LEGION_MAX_NUM_NODES > 256)
    typedef AVXTLBitMask<LEGION_MAX_NUM_NODES> NodeMask;
#elif (LEGION_MAX_NUM_NODES > 128)
    typedef AVXBitMask<LEGION_MAX_NUM_NODES> NodeMask;
#elif (LEGION_MAX_NUM_NODES > 64)
    typedef SSEBitMask<LEGION_MAX_NUM_NODES> NodeMask;
#else
    typedef BitMask<LEGION_NODE_MASK_NODE_TYPE,LEGION_MAX_NUM_NODES,
                    LEGION_NODE_MASK_NODE_SHIFT,
                    LEGION_NODE_MASK_NODE_MASK> NodeMask;
#endif
#elif defined(__SSE2__)
#if (LEGION_MAX_NUM_NODES > 128)
    typedef SSETLBitMask<LEGION_MAX_NUM_NODES> NodeMask;
#elif (LEGION_MAX_NUM_NODES > 64)
    typedef SSEBitMask<LEGION_MAX_NUM_NODES> NodeMask;
#else
    typedef BitMask<LEGION_NODE_MASK_NODE_TYPE,LEGION_MAX_NUM_NODES,
                    LEGION_NODE_MASK_NODE_SHIFT,
                    LEGION_NODE_MASK_NODE_MASK> NodeMask;
#endif
#elif defined(__ALTIVEC__)
#if (LEGION_MAX_NUM_NODES > 128)
    typedef PPCTLBitMask<LEGION_MAX_NUM_NODES> NodeMask;
#elif (LEGION_MAX_NUM_NODES > 64)
    typedef PPCBitMask<LEGION_MAX_NUM_NODES> NodeMask;
#else
    typedef BitMask<LEGION_NODE_MASK_NODE_TYPE,LEGION_MAX_NUM_NODES,
                    LEGION_NODE_MASK_NODE_SHIFT,
                    LEGION_NODE_MASK_NODE_MASK> NodeMask;
#endif
#elif defined(__ARM_NEON)
#if (LEGION_MAX_NUM_NODES > 128)
    typedef NeonTLBitMask<LEGION_MAX_NUM_NODES> NodeMask;
#elif (LEGION_MAX_NUM_NODES > 64)
    typedef NeonBitMask<LEGION_MAX_NUM_NODES> NodeMask;
#else
    typedef BitMask<LEGION_NODE_MASK_NODE_TYPE,LEGION_MAX_NUM_NODES,
                    LEGION_NODE_MASK_NODE_SHIFT,
                    LEGION_NODE_MASK_NODE_MASK> NodeMask;
#endif
#else
#if (LEGION_MAX_NUM_NODES > 64)
    typedef TLBitMask<LEGION_NODE_MASK_NODE_TYPE,LEGION_MAX_NUM_NODES,
                      LEGION_NODE_MASK_NODE_SHIFT,
                      LEGION_NODE_MASK_NODE_MASK> NodeMask;
#else
    typedef BitMask<LEGION_NODE_MASK_NODE_TYPE,LEGION_MAX_NUM_NODES,
                    LEGION_NODE_MASK_NODE_SHIFT,
                    LEGION_NODE_MASK_NODE_MASK> NodeMask;
#endif
#endif
    typedef CompoundBitMask<NodeMask,1/*bloat*/,true/*bidir*/> NodeSet;

#undef LEGION_NODE_MASK_NODE_SHIFT
#undef LEGION_NODE_MASK_NODE_MASK

// The following macros are used in the ProcessorMask instantiation of BitMask
// If you change one you probably have to change the others too
#define LEGION_PROC_MASK_PROC_TYPE           uint64_t
#define LEGION_PROC_MASK_PROC_SHIFT          6
#define LEGION_PROC_MASK_PROC_MASK           0x3F
#define LEGION_PROC_MASK_PROC_ALL_ONES       0xFFFFFFFFFFFFFFFF

#if defined(__AVX__)
#if (LEGION_MAX_NUM_PROCS > 256)
    typedef AVXTLBitMask<LEGION_MAX_NUM_PROCS> ProcessorMask;
#elif (LEGION_MAX_NUM_PROCS > 128)
    typedef AVXBitMask<LEGION_MAX_NUM_PROCS> ProcessorMask;
#elif (LEGION_MAX_NUM_PROCS > 64)
    typedef SSEBitMask<LEGION_MAX_NUM_PROCS> ProcessorMask;
#else
    typedef BitMask<LEGION_PROC_MASK_PROC_TYPE,LEGION_MAX_NUM_PROCS,
                    LEGION_PROC_MASK_PROC_SHIFT,
                    LEGION_PROC_MASK_PROC_MASK> ProcessorMask;
#endif
#elif defined(__SSE2__)
#if (LEGION_MAX_NUM_PROCS > 128)
    typedef SSETLBitMask<LEGION_MAX_NUM_PROCS> ProcessorMask;
#elif (LEGION_MAX_NUM_PROCS > 64)
    typedef SSEBitMask<LEGION_MAX_NUM_PROCS> ProcessorMask;
#else
    typedef BitMask<LEGION_PROC_MASK_PROC_TYPE,LEGION_MAX_NUM_PROCS,
                    LEGION_PROC_MASK_PROC_SHIFT,
                    LEGION_PROC_MASK_PROC_MASK> ProcessorMask;
#endif
#elif defined(__ALTIVEC__)
#if (LEGION_MAX_NUM_PROCS > 128)
    typedef PPCTLBitMask<LEGION_MAX_NUM_PROCS> ProcessorMask;
#elif (LEGION_MAX_NUM_PROCS > 64)
    typedef PPCBitMask<LEGION_MAX_NUM_PROCS> ProcessorMask;
#else
    typedef BitMask<LEGION_PROC_MASK_PROC_TYPE,LEGION_MAX_NUM_PROCS,
                    LEGION_PROC_MASK_PROC_SHIFT,
                    LEGION_PROC_MASK_PROC_MASK> ProcessorMask;
#endif
#elif defined(__ARM_NEON)
#if (LEGION_MAX_NUM_PROCS > 128)
    typedef NeonTLBitMask<LEGION_MAX_NUM_PROCS> ProcessorMask;
#elif (LEGION_MAX_NUM_PROCS > 64)
    typedef NeonBitMask<LEGION_MAX_NUM_PROCS> ProcessorMask;
#else
    typedef BitMask<LEGION_PROC_MASK_PROC_TYPE,LEGION_MAX_NUM_PROCS,
                    LEGION_PROC_MASK_PROC_SHIFT,
                    LEGION_PROC_MASK_PROC_MASK> ProcessorMask;
#endif
#else
#if (LEGION_MAX_NUM_PROCS > 64)
    typedef TLBitMask<LEGION_PROC_MASK_PROC_TYPE,LEGION_MAX_NUM_PROCS,
                      LEGION_PROC_MASK_PROC_SHIFT,
                      LEGION_PROC_MASK_PROC_MASK> ProcessorMask;
#else
    typedef BitMask<LEGION_PROC_MASK_PROC_TYPE,LEGION_MAX_NUM_PROCS,
                    LEGION_PROC_MASK_PROC_SHIFT,
                    LEGION_PROC_MASK_PROC_MASK> ProcessorMask;
#endif
#endif

#undef PROC_SHIFT
#undef PROC_MASK 

    // Legion derived event types
    class LgEvent : public Realm::Event {
    public:
      static const LgEvent NO_LG_EVENT;
    public:
      LgEvent(void) noexcept { id = 0; }
      LgEvent(const LgEvent &rhs) = default;
      explicit LgEvent(const Realm::Event e) { id = e.id; }
    public:
      inline LgEvent& operator=(const LgEvent &rhs) = default;
    public:
      // Override the wait method so we can have our own implementation
      inline void wait(void) const;
      inline void wait_faultaware(bool &poisoned, bool from_application) const;
    protected:
      void begin_context_wait(Context ctx, bool from_application) const;
      void end_context_wait(Context ctx, bool from_application) const;
    };

    class PredEvent : public LgEvent {
    public:
      static const PredEvent NO_PRED_EVENT;
    public:
      PredEvent(void) noexcept : LgEvent() { } 
      PredEvent(const PredEvent &rhs) = default;
      explicit PredEvent(const Realm::Event &e) : LgEvent(e) { }
    public:
      inline PredEvent& operator=(const PredEvent &rhs) = default;
    };

    class PredUserEvent : public PredEvent {
    public:
      static const PredUserEvent NO_PRED_USER_EVENT;
    public:
      PredUserEvent(void) noexcept : PredEvent() { }
      PredUserEvent(const PredUserEvent &rhs) = default;
      explicit PredUserEvent(const Realm::UserEvent &e) : PredEvent(e) { }
    public:
      inline PredUserEvent& operator=(const PredUserEvent &rhs) = default;
      inline operator Realm::UserEvent() const
        { Realm::UserEvent e; e.id = id; return e; }
    };

    class ApEvent : public LgEvent {
    public:
      static const ApEvent NO_AP_EVENT;
    public:
      ApEvent(void) noexcept : LgEvent() { }
      ApEvent(const ApEvent &rhs) = default;
      explicit ApEvent(const Realm::Event &e) : LgEvent(e) { }
      explicit ApEvent(const PredEvent &e) { id = e.id; }
    public:
      inline ApEvent& operator=(const ApEvent &rhs) = default;
      inline bool has_triggered_faultignorant(void) const
        { bool poisoned = false; 
          return has_triggered_faultaware(poisoned); }
      inline void wait_faultaware(bool &poisoned) const
        { return LgEvent::wait_faultaware(poisoned, true/*application*/); }
      inline void wait_faultignorant(void) const
        { bool poisoned = false; 
          LgEvent::wait_faultaware(poisoned, true/*application*/); }
    private:
      // Make these private because we always want to be conscious of faults
      // when testing or waiting on application events
      inline bool has_triggered(void) const { return LgEvent::has_triggered(); }
      inline void wait(void) const { LgEvent::wait(); }
    };

    class ApUserEvent : public ApEvent {
    public:
      static const ApUserEvent NO_AP_USER_EVENT;
    public:
      ApUserEvent(void) noexcept : ApEvent() { }
      ApUserEvent(const ApUserEvent &rhs) = default;
      explicit ApUserEvent(const Realm::UserEvent &e) : ApEvent(e) { }
    public:
      inline ApUserEvent& operator=(const ApUserEvent &rhs) = default;
      inline operator Realm::UserEvent() const
        { Realm::UserEvent e; e.id = id; return e; }
    };

    class ApBarrier : public ApEvent {
    public:
      static const ApBarrier NO_AP_BARRIER;
    public:
      ApBarrier(void) noexcept : ApEvent(), timestamp(0) { }
      ApBarrier(const ApBarrier &rhs) = default; 
      explicit ApBarrier(const Realm::Barrier &b) 
        : ApEvent(b), timestamp(b.timestamp) { }
    public:
      inline ApBarrier& operator=(const ApBarrier &rhs) = default;
      inline operator Realm::Barrier() const
        { Realm::Barrier b; b.id = id; 
          b.timestamp = timestamp; return b; }
    public:
      inline bool get_result(void *value, size_t value_size) const
        { Realm::Barrier b; b.id = id;
          b.timestamp = timestamp; return b.get_result(value, value_size); }
      inline void destroy_barrier(void)
        { Realm::Barrier b; b.id = id;
          b.timestamp = timestamp; b.destroy_barrier(); }
    public:
      Realm::Barrier::timestamp_t timestamp;
    };

    class RtEvent : public LgEvent {
    public:
      static const RtEvent NO_RT_EVENT;
    public:
      RtEvent(void) noexcept : LgEvent() { }
      RtEvent(const RtEvent &rhs) = default;
      explicit RtEvent(const Realm::Event &e) : LgEvent(e) { }
      explicit RtEvent(const PredEvent &e) { id = e.id; }
    public:
      inline RtEvent& operator=(const RtEvent &rhs) = default;
    };

    class RtUserEvent : public RtEvent {
    public:
      static const RtUserEvent NO_RT_USER_EVENT;
    public:
      RtUserEvent(void) noexcept : RtEvent() { }
      RtUserEvent(const RtUserEvent &rhs) = default;
      explicit RtUserEvent(const Realm::UserEvent &e) : RtEvent(e) { }
    public:
      inline RtUserEvent& operator=(const RtUserEvent &rhs) = default;
      inline operator Realm::UserEvent() const
        { Realm::UserEvent e; e.id = id; return e; }
    };

    class RtBarrier : public RtEvent {
    public:
      static const RtBarrier NO_RT_BARRIER;
    public:
      RtBarrier(void) noexcept : RtEvent(), timestamp(0) { }
      RtBarrier(const RtBarrier &rhs) = default;
      explicit RtBarrier(const Realm::Barrier &b)
        : RtEvent(b), timestamp(b.timestamp) { }
    public:
      inline RtBarrier& operator=(const RtBarrier &rhs) = default;
      inline operator Realm::Barrier() const
        { Realm::Barrier b; b.id = id; 
          b.timestamp = timestamp; return b; } 
    public:
      inline bool get_result(void *value, size_t value_size) const
        { Realm::Barrier b; b.id = id;
          b.timestamp = timestamp; return b.get_result(value, value_size); }
      inline RtBarrier get_previous_phase(void)
        { Realm::Barrier b; b.id = id;
          return RtBarrier(b.get_previous_phase()); }
      inline void destroy_barrier(void)
        { Realm::Barrier b; b.id = id;
          b.timestamp = timestamp; b.destroy_barrier(); }
    public:
      Realm::Barrier::timestamp_t timestamp;
    }; 

    // Local lock for accelerating lock taking
    class LocalLock {
    public:
      inline LocalLock(void) { } 
    public:
      inline LocalLock(const LocalLock &rhs)
      {
        // should never be called
        assert(false);
      }
      inline ~LocalLock(void) { }
    public:
      inline LocalLock& operator=(const LocalLock &rhs)
      {
        // should never be called
        assert(false);
        return *this;
      }
    private:
      // These are only accessible via AutoLock
      friend class AutoLock;
      friend class AutoTryLock;
      friend class Mapping::AutoLock;
      inline RtEvent lock(void)   { return RtEvent(wrlock()); }
      inline RtEvent wrlock(void) { return RtEvent(reservation.wrlock()); }
      inline RtEvent rdlock(void) { return RtEvent(reservation.rdlock()); }
      inline bool trylock(void) { return reservation.trylock(); }
      inline bool trywrlock(void) { return reservation.trywrlock(); }
      inline bool tryrdlock(void) { return reservation.tryrdlock(); }
      inline void unlock(void) { reservation.unlock(); }
    private:
      inline void advise_sleep_entry(Realm::UserEvent guard)
        { reservation.advise_sleep_entry(guard); }
      inline void advise_sleep_exit(void)
        { reservation.advise_sleep_exit(); }
    protected:
      Realm::FastReservation reservation;
    };

    /////////////////////////////////////////////////////////////
    // AutoLock 
    /////////////////////////////////////////////////////////////
    // An auto locking class for taking a lock and releasing it when
    // the object goes out of scope
    class AutoLock { 
    public:
      inline AutoLock(LocalLock &r, int mode = 0, bool excl = true)
        : local_lock(r), previous(Internal::local_lock_list), 
          exclusive(excl), held(true)
      {
#ifdef DEBUG_REENTRANT_LOCKS
        if (previous != NULL)
          previous->check_for_reentrant_locks(&local_lock);
#endif
        if (exclusive)
        {
          RtEvent ready = local_lock.wrlock();
          while (ready.exists())
          {
            ready.wait();
            ready = local_lock.wrlock();
          }
        }
        else
        {
          RtEvent ready = local_lock.rdlock();
          while (ready.exists())
          {
            ready.wait();
            ready = local_lock.rdlock();
          }
        }
        Internal::local_lock_list = this;
      }
    protected:
      // Helper constructor for AutoTryLock and Mapping::AutoLock
      inline AutoLock(int mode, bool excl, LocalLock &r)
        : local_lock(r), previous(Internal::local_lock_list), 
          exclusive(excl), held(false)
      {
#ifdef DEBUG_REENTRANT_LOCKS
        if (previous != NULL)
          previous->check_for_reentrant_locks(&local_lock);
#endif
      }
    public:
      AutoLock(AutoLock &&rhs) = delete;
      AutoLock(const AutoLock &rhs) = delete;
      inline ~AutoLock(void)
      {
        if (held)
        {
#ifdef DEBUG_LEGION
          assert(Internal::local_lock_list == this);
#endif
          local_lock.unlock();
          Internal::local_lock_list = previous;
        }
        else
          assert(Internal::local_lock_list == previous);
      }
    public:
      AutoLock& operator=(AutoLock &&rhs) = delete;
      AutoLock& operator=(const AutoLock &rhs) = delete;
    public:
      inline void release(void) 
      { 
#ifdef DEBUG_LEGION
        assert(held);
        assert(Internal::local_lock_list == this);
#endif
        local_lock.unlock(); 
        Internal::local_lock_list = previous;
        held = false; 
      }
      inline void reacquire(void)
      {
#ifdef DEBUG_LEGION
        assert(!held);
        assert(Internal::local_lock_list == previous);
#endif
#ifdef DEBUG_REENTRANT_LOCKS
        if (previous != NULL)
          previous->check_for_reentrant_locks(&local_lock);
#endif
        if (exclusive)
        {
          RtEvent ready = local_lock.wrlock();
          while (ready.exists())
          {
            ready.wait();
            ready = local_lock.wrlock();
          }
        }
        else
        {
          RtEvent ready = local_lock.rdlock();
          while (ready.exists())
          {
            ready.wait();
            ready = local_lock.rdlock();
          }
        }
        Internal::local_lock_list = this;
        held = true;
      }
    public:
      inline void advise_sleep_entry(Realm::UserEvent guard) const
      {
        if (held)
          local_lock.advise_sleep_entry(guard);
        if (previous != NULL)
          previous->advise_sleep_entry(guard);
      }
      inline void advise_sleep_exit(void) const
      {
        if (held)
          local_lock.advise_sleep_exit();
        if (previous != NULL)
          previous->advise_sleep_exit();
      }
#ifdef DEBUG_REENTRANT_LOCKS
      inline void check_for_reentrant_locks(LocalLock *to_acquire) const
      {
        assert(to_acquire != &local_lock);
        if (previous != NULL)
          previous->check_for_reentrant_locks(to_acquire);
      }
#endif
    protected:
      LocalLock &local_lock;
      AutoLock *const previous;
      const bool exclusive;
      bool held;
    };

    // AutoTryLock is an extension of AutoLock that supports try lock
    class AutoTryLock : public AutoLock {
    public:
      inline AutoTryLock(LocalLock &r, int mode = 0, bool excl = true)
        : AutoLock(mode, excl, r) 
      {
        if (exclusive)
          ready = local_lock.wrlock();
        else
          ready = local_lock.rdlock();
        held = !ready.exists();
        if (held)
          Internal::local_lock_list = this;
      }
      AutoTryLock(const AutoTryLock &rhs) = delete;
    public:
      AutoTryLock& operator=(const AutoTryLock &rhs) = delete;
    public:
      // Allow an easy test for whether we got the lock or not
      inline bool has_lock(void) const { return held; }
      inline RtEvent try_next(void) const { return ready; }
    protected:
      RtEvent ready;
    };
    
    // Special method that we need here for waiting on events

    //--------------------------------------------------------------------------
    inline void LgEvent::wait(void) const
    //--------------------------------------------------------------------------
    {
#ifdef DEBUG_LEGION_WAITS
      const int local_meta_task_id = Internal::meta_task_id;
      const long long start = Realm::Clock::current_time_in_microseconds();
#endif
      // Save the context locally
      Internal::TaskContext *local_ctx = Internal::implicit_context; 
      // Save the task provenance information
      UniqueID local_provenance = Internal::implicit_provenance;
#ifdef DEBUG_LEGION_CALLERS
      LgTaskID local_kind = Internal::implicit_task_kind;
      LgTaskID local_caller = Internal::implicit_task_caller;
#endif
      // Save whether we are in a registration callback
      unsigned local_callback = Internal::inside_registration_callback;
      // Save the reference tracker that we have
      ImplicitReferenceTracker *local_tracker = implicit_reference_tracker;
      Internal::implicit_reference_tracker = NULL;
      // Check to see if we have any local locks to notify
      if (Internal::local_lock_list != NULL)
      {
        // Make a copy of the local locks here
        AutoLock *local_lock_list_copy = Internal::local_lock_list;
        // Set this back to NULL until we are done waiting
        Internal::local_lock_list = NULL;
        // Make a user event and notify all the thread locks
        const Realm::UserEvent done = Realm::UserEvent::create_user_event();
        local_lock_list_copy->advise_sleep_entry(done);
        if (local_ctx != NULL)
          begin_context_wait(local_ctx, false/*from application*/); 
        // Now we can do the wait
        if (!Processor::get_executing_processor().exists())
          Realm::Event::external_wait();
        else
          Realm::Event::wait();
        if (local_ctx != NULL)
          end_context_wait(local_ctx, false/*from application*/);
        // When we wake up, notify that we are done and exited the wait
        local_lock_list_copy->advise_sleep_exit();
        // Trigger the user-event
        done.trigger();
        // Restore our local lock list
        Internal::local_lock_list = local_lock_list_copy; 
      }
      else // Just do the normal wait
      {
        if (local_ctx != NULL)
          begin_context_wait(local_ctx, false/*from application*/);
        if (!Processor::get_executing_processor().exists())
          Realm::Event::external_wait();
        else
          Realm::Event::wait();
        if (local_ctx != NULL)
          end_context_wait(local_ctx, false/*from application*/);
      }
      // Write the context back
      Internal::implicit_context = local_ctx;
      // Write the provenance information back
      Internal::implicit_provenance = local_provenance;
#ifdef DEBUG_LEGION_CALLERS
      Internal::implicit_task_kind = local_kind;
      Internal::implicit_task_caller = local_caller;
#endif
      // Write the registration callback information back
      Internal::inside_registration_callback = local_callback;
#ifdef DEBUG_LEGION
      assert(Internal::implicit_reference_tracker == NULL);
#endif
      // Write the local reference tracker back
      Internal::implicit_reference_tracker = local_tracker;
#ifdef DEBUG_LEGION_WAITS
      Internal::meta_task_id = local_meta_task_id;
      const long long stop = Realm::Clock::current_time_in_microseconds();
      if (((stop - start) >= LIMIT) && (local_meta_task_id == BAD_TASK_ID))
        assert(false);
#endif
    }

    //--------------------------------------------------------------------------
    inline void LgEvent::wait_faultaware(bool &poisoned, bool from_app) const
    //--------------------------------------------------------------------------
    {
#ifdef DEBUG_LEGION_WAITS
      const int local_meta_task_id = Internal::meta_task_id;
      const long long start = Realm::Clock::current_time_in_microseconds();
#endif
      // Save the context locally
      Internal::TaskContext *local_ctx = Internal::implicit_context; 
      // Save the task provenance information
      UniqueID local_provenance = Internal::implicit_provenance;
#ifdef DEBUG_LEGION_CALLERS
      LgTaskID local_kind = Internal::implicit_task_kind;
      LgTaskID local_caller = Internal::implicit_task_caller;
#endif
      // Save whether we are in a registration callback
      unsigned local_callback = Internal::inside_registration_callback;
      // Save the reference tracker that we have
      ImplicitReferenceTracker *local_tracker = implicit_reference_tracker;
      Internal::implicit_reference_tracker = NULL;
      // Check to see if we have any local locks to notify
      if (Internal::local_lock_list != NULL)
      {
        // Make a copy of the local locks here
        AutoLock *local_lock_list_copy = Internal::local_lock_list;
        // Set this back to NULL until we are done waiting
        Internal::local_lock_list = NULL;
        // Make a user event and notify all the thread locks
        const Realm::UserEvent done = Realm::UserEvent::create_user_event();
        local_lock_list_copy->advise_sleep_entry(done);
        if (local_ctx != NULL)
          begin_context_wait(local_ctx, from_app);
        // Now we can do the wait
        if (!Processor::get_executing_processor().exists())
          Realm::Event::external_wait_faultaware(poisoned);
        else
          Realm::Event::wait_faultaware(poisoned);
        if (local_ctx != NULL)
          end_context_wait(local_ctx, from_app);
        // When we wake up, notify that we are done and exited the wait
        local_lock_list_copy->advise_sleep_exit();
        // Trigger the user-event
        done.trigger();
        // Restore our local lock list
        Internal::local_lock_list = local_lock_list_copy; 
      }
      else // Just do the normal wait
      {
        if (local_ctx != NULL)
          begin_context_wait(local_ctx, from_app);
        if (!Processor::get_executing_processor().exists())
          Realm::Event::external_wait_faultaware(poisoned);
        else
          Realm::Event::wait_faultaware(poisoned);
        if (local_ctx != NULL)
          end_context_wait(local_ctx, from_app);
      }
      // Write the context back
      Internal::implicit_context = local_ctx;
      // Write the provenance information back
      Internal::implicit_provenance = local_provenance;
#ifdef DEBUG_LEGION_CALLERS
      Internal::implicit_task_kind = local_kind;
      Internal::implicit_task_caller = local_caller;
#endif
      // Write the registration callback information back
      Internal::inside_registration_callback = local_callback;
#ifdef DEBUG_LEGION
      assert(Internal::implicit_reference_tracker == NULL);
#endif
      // Write the local reference tracker back
      Internal::implicit_reference_tracker = local_tracker;
#ifdef DEBUG_LEGION_WAITS
      Internal::meta_task_id = local_meta_task_id;
      const long long stop = Realm::Clock::current_time_in_microseconds();
      if (((stop - start) >= LIMIT) && (local_meta_task_id == BAD_TASK_ID))
        assert(false);
#endif
    }

  }; // namespace Internal 
  
  // A class for preventing serialization of Legion objects
  // which cannot be serialized
  template<typename T>
  class Unserializable {
  public:
    inline size_t legion_buffer_size(void);
    inline size_t legion_serialize(void *buffer);
    inline size_t legion_deserialize(const void *buffer);
  };

}; // Legion namespace

// now that we have things like LgEvent defined, we can include accessor.h to
// pick up ptr_t, which is used for compatibility-mode Coloring and friends
#include "legion/accessor.h"

namespace Legion {
  typedef LegionRuntime::Accessor::ByteOffset ByteOffset;

  typedef std::map<Color,ColoredPoints<ptr_t> > Coloring;
  typedef std::map<Color,Domain> DomainColoring;
  typedef std::map<Color,std::set<Domain> > MultiDomainColoring;
  typedef std::map<DomainPoint,ColoredPoints<ptr_t> > PointColoring;
  typedef std::map<DomainPoint,Domain> DomainPointColoring;
  typedef std::map<DomainPoint,std::set<Domain> > MultiDomainPointColoring;
};

#endif // __LEGION_TYPES_H__
