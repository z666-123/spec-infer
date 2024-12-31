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

#ifndef __LEGION_MAPPING_H__
#define __LEGION_MAPPING_H__

#include "legion/legion_types.h"
#include "legion/legion_constraint.h"
#include "legion.h"
#include "realm/profiling.h"

#include <iostream>
#include <type_traits>

namespace Legion {
  namespace Mapping { 

    /**
     * \class PhysicalInstance
     * The PhysicalInstance class provides an interface for
     * garnering information about physical instances 
     * throughout the mapping interface. Mappers can discover
     * information about physical instances such as their
     * location, layout, and validity of data. Mappers can
     * make copies of these objects and store them permanently
     * in their state, but must be prepared that the validity
     * of field data can change under such circumstances.
     * The instance itself can actually be garbage collected.
     * Methods are provided for detecting such cases.
     */
    class PhysicalInstance {
    public:
      PhysicalInstance(void);
      PhysicalInstance(PhysicalInstance &&rhs);
      PhysicalInstance(const PhysicalInstance &rhs);
      ~PhysicalInstance(void);
    public:
      PhysicalInstance& operator=(PhysicalInstance &&rhs);
      PhysicalInstance& operator=(const PhysicalInstance &rhs); 
    public:
      bool operator<(const PhysicalInstance &rhs) const;
      bool operator==(const PhysicalInstance &rhs) const;
      bool operator!=(const PhysicalInstance &rhs) const;
    public:
      // Get the location of this physical instance
      Memory get_location(void) const;
      unsigned long get_instance_id(void) const;
      size_t get_instance_size(void) const;
      Domain get_instance_domain(void) const;
      // Adds all fields that exist in instance to 'fields', unless
      //  instance is virtual
      void get_fields(std::set<FieldID> &fields) const;
    public:
      FieldSpace get_field_space(void) const;
      RegionTreeID get_tree_id(void) const;
      LayoutConstraintID get_layout_id(void) const;
    public:
      // See if our instance still exists or if it has been
      // garbage collected, this is just a sample, using the
      // acquire methods provided by the mapper_rt interface
      // can prevent it from being collected during the
      // lifetime of a mapper call.
      bool exists(bool strong_test = false) const;
      bool is_normal_instance(void) const;
      bool is_virtual_instance(void) const;
      bool is_reduction_instance(void) const;
      bool is_external_instance(void) const;
    public:
      bool has_field(FieldID fid) const;
      void has_fields(std::map<FieldID,bool> &fids) const;
      void remove_space_fields(std::set<FieldID> &fids) const;
    public:
      // Use these to specify the fields for which this instance
      // should be used. It is optional to specify this and is only
      // necessary to disambiguate which fields should be used when
      // multiple selected instances have the same field(s).
      void add_use_field(FieldID fid);
      void add_use_fields(const std::set<FieldID> &fids);
    public:
      // Check to see if a whole set of constraints are satisfied
      bool entails(const LayoutConstraintSet &constraint_set,
                   const LayoutConstraint **failed_constraint = NULL) const;
    public:
      static PhysicalInstance get_virtual_instance(void);
    private:
      friend class CollectiveView;
      FRIEND_ALL_RUNTIME_CLASSES
      // Only the runtime can make an instance like this
      PhysicalInstance(PhysicalInstanceImpl impl);
    private:
      PhysicalInstanceImpl impl;
      friend std::ostream& operator<<(std::ostream& os,
				      const PhysicalInstance& p);
    };

    /**
     * \class CollectiveView
     * A collective view is simply a group of physical instances 
     * that the runtime knows all have the same data replicated
     * across the different copies. Collective views only show
     * up when the mapper is asked to pick a source instances 
     * from a collective group.
     */
    class CollectiveView {
    public:
      CollectiveView(void);
      CollectiveView(CollectiveView &&rhs);
      CollectiveView(const CollectiveView &rhs);
      ~CollectiveView(void);
    public:
      CollectiveView& operator=(CollectiveView &&rhs);
      CollectiveView& operator=(const CollectiveView&rhs); 
    public:
      bool operator<(const CollectiveView &rhs) const;
      bool operator==(const CollectiveView &rhs) const;
      bool operator!=(const CollectiveView &rhs) const;
    public:
      void find_instances_in_memory(Memory memory,
                                    std::vector<PhysicalInstance> &insts) const;
      void find_instances_nearest_memory(Memory memory,
                                    std::vector<PhysicalInstance> &insts,
                                    bool bandwidth = true) const;
    private:
      FRIEND_ALL_RUNTIME_CLASSES
      // Only the runtime can make an instance like this
      CollectiveView(CollectiveViewImpl impl);
    private:
      CollectiveViewImpl impl;
      friend std::ostream& operator<<(std::ostream& os,
				      const CollectiveView& p);
    };

    /**
     * \class MapperEvent
     * A mapper event is a mechanism through which mappers
     * are allowed to preempt a mapper call until a later
     * time when the mapper is ready to resume execution 
     */
    class MapperEvent {
    public:
      MapperEvent(void)
        : impl(Internal::RtUserEvent()) { }
      FRIEND_ALL_RUNTIME_CLASSES
    public:
      inline bool exists(void) const { return impl.exists(); }
      inline bool operator==(const MapperEvent &rhs) const 
        { return (impl.id == rhs.impl.id); }
      inline bool operator<(const MapperEvent &rhs) const
        { return (impl.id < rhs.impl.id); }
    private:
      Internal::RtUserEvent impl;
    };

    namespace ProfilingMeasurements {
      // import all the Realm measurements into this namespace too
      using namespace Realm::ProfilingMeasurements;

      struct RuntimeOverhead {
	static const ProfilingMeasurementID ID = PMID_RUNTIME_OVERHEAD;
        RuntimeOverhead(void);
	// application, runtime, wait times all reported in nanoseconds
	long long application_time;  // time spent in application code
	long long runtime_time;      // time spent in runtime code
	long long wait_time;         // time spent waiting
      };
    };

    /**
     * \class ProfilingRequest
     * This definition shadows the Realm version, since it's the job of the
     * Legion runtime to handle the actual callback part (and to divert any
     * measurement requests not known to Realm)
     */
    class ProfilingRequest {
    public:
      ProfilingRequest(void);
      ~ProfilingRequest(void);

      template <typename T>
      inline ProfilingRequest &add_measurement(void);

      inline bool empty(void) const;

    protected:
      FRIEND_ALL_RUNTIME_CLASSES
      void populate_realm_profiling_request(Realm::ProfilingRequest& req);

      std::set<ProfilingMeasurementID> requested_measurements;
    };

    /**
     * \class ProfilingResponse
     * Similarly, the profiling response wraps around the Realm version so
     * that it can handle non-Realm measurements
     */
    class ProfilingResponse {
    public:
      // default constructor used because this appears in the
      //  {...}ProfilingInfo structs below
      ProfilingResponse(void);
      ~ProfilingResponse(void);

      // even if a measurement was requested, it may not have been performed -
      //  use this to check
      template <typename T>
      inline bool has_measurement(void) const;

      // extracts a measurement (if available), returning a dynamically
      //  allocated result - caller should delete it when done
      template <typename T>
      inline T *get_measurement(void) const;

      template <typename T>
      inline bool get_measurement(T& result) const;

    protected:
      FRIEND_ALL_RUNTIME_CLASSES
      void attach_realm_profiling_response(
          const Realm::ProfilingResponse& resp);
      void attach_overhead(
          ProfilingMeasurements::RuntimeOverhead *overhead);

      const Realm::ProfilingResponse *realm_resp;
      ProfilingMeasurements::RuntimeOverhead *overhead;
    };

    /**
     * \struct TaskGeneratorArguments
     * This structure defines the arguments that will be passed to a 
     * task generator variant from a call to find_or_create_variant
     * if the no variant could be found. The task generator function 
     * will then be expected to generate one or more variants and 
     * register them with the runtime. The first variant registered
     * will be the one that the runtime will use to satisfy the
     * mapper request.
     */
    struct TaskGeneratorArguments {
    public:
      TaskID                            task_id;
      MapperID                          mapper_id;
      ExecutionConstraintSet            execution_constraints;
      TaskLayoutConstraintSet           layout_constraints;
    };

    /**
     * \class Mapper
     * This class is a pure virtual class that defines the mapper interface.
     * Every mapper must be derived from this class and implement all of
     * the virtual methods declared in this class.
     */
    class Mapper {
    public:
      Mapper(MapperRuntime *rt);
      virtual ~Mapper(void);
    public:
      MapperRuntime *const runtime;
    public:
      /**
       ** ----------------------------------------------------------------------
       *  Get Mapper Name 
       * ----------------------------------------------------------------------
       *  Specify a name that the runtime can use for referring to this
       *  mapper. This will primarily be used for providing helpful
       *  error messages so semantically meaningful names are encouraged.
       *  This mapper call must be immutable as it may be made before the
       *  synchronization model has been chosen.
       */
      virtual const char* get_mapper_name(void) const = 0;
    public:
      /**
       * ----------------------------------------------------------------------
       *  Get Mapper Synchronization Model 
       * ----------------------------------------------------------------------
       * Specify the mapper synchronization model. The concurrent mapper model 
       * will alternatively allow mapper calls to be performed at the same time 
       * and will rely on the mapper to lock itself to protect access to shared 
       * data. If the mapper is locked when performing a utility call, it may
       * be automatically unlocked and locked around the utility call. The 
       * serialized model will guarantee that all mapper calls are performed 
       * atomically with respect to each other unless they perform a utility 
       * call when the mapper has indicated that it is safe to permit 
       * re-entrant mapper call(s) in the process of performing the utility 
       * call. The reentrant version of the serialized mapper model will 
       * default to allowing reentrant calls to the mapper context. The 
       * non-reentrant version will default to not allowing reentrant calls.
       */
      enum MapperSyncModel {
        CONCURRENT_MAPPER_MODEL,
        SERIALIZED_REENTRANT_MAPPER_MODEL,
        SERIALIZED_NON_REENTRANT_MAPPER_MODEL,
      };
      virtual MapperSyncModel get_mapper_sync_model(void) const = 0;
    public:
      /**
       * ----------------------------------------------------------------------
       *  Request Valid Instances
       * ----------------------------------------------------------------------
       * Indicate whether the runtime should populate the valid instances as
       * inputs for mapping operations. This will control the setting for all
       * operations that are not tasks. For tasks, this will just set the 
       * initial value of 'valid_instances' in the select_task_options struct
       * as we give mappers more control over needing valid inputs for tasks
       * than generic operations at the moment. We provide a default 
       * implementation of this method because older versions of the runtime
       * would always fill in this data structure, however, we now allow
       * mappers to disable this in order to reduce the cost of physical
       * analysis done for mapping operations.
       */
      virtual bool request_valid_instances(void) const { return true; }
    public: // Task mapping calls
      /**
       * ----------------------------------------------------------------------
       *  Select Task Options
       * ----------------------------------------------------------------------
       * This mapper call happens immediately after the task is launched
       * and before any other stages of the pipeline. This gives the mapper
       * control over the execution of this task before the runtime puts it
       * in the task pipeline. Below are the fields of the TaskOptions
       * struct and their semantics.
       *
       * target_proc default:local processor
       *     This field will only be obeyed by single task launches. It 
       *     sets the initial processor where the task will be sent after
       *     dependence analysis if the task is to be eagerly evaluated.
       *     Index space tasks will invoke slice_domain to determine where
       *     its components should be sent.
       *
       * inline_task default:false
       *     Specify whether this task should be inlined directly into the
       *     parent task using the parent task's regions. If the regions
       *     are not already mapped, they will be re-mapped and the task
       *     will be executed on the local processor. The mapper should
       *     select an alternative call to the select_inline_variant call
       *     to select the task variant to be used.
       *
       * spawn_task default:false
       *     This field is inspired by Cilk and has equivalent semantics.
       *     If a task is spawned, then it becomes eligible for stealing,
       *     otherwise it will traverse the task pipeline as directed by
       *     the mapper. The one deviation from Cilk stealing is that 
       *     stealing in Legion is managed by the mappers instead of
       *     implicitly by the Legion runtime.
       *
       * map_locally default:false
       *     Tasks have the option of either being mapped on 
       *     the processor on which they were created or being mapped
       *     on their ultimate destination processor.  Mapping on the
       *     local processor where the task was created can be
       *     more efficient in some cases since it requires less
       *     meta-data movement by the runtime, but can also be
       *     subject to having an incomplete view of the destination
       *     memories during the mapping process.  In general a task
       *     should only be mapped locally if it is a leaf task as
       *     the runtime will need to move the meta-data for a task
       *     anyway if it is going to launch sub-tasks.  Note that
       *     deciding to map a task locally disqualifies that task
       *     from being stolen as it will have already been mapped
       *     once it enters the ready queue.
       *
       * valid_instance default:result of request_valid_instances
       *     When calls to map_task are performed, it's often the 
       *     case that the mapper will want to know the currently valid
       *     instances are for that region. There is some overhead to
       *     doing this and the mapper may want to avoid this overhead
       *     in cases where it knows it won't need the information such
       *     as when it is going to virtually map all the regions for 
       *     an inner task. By setting this flag to false the mapper
       *     can opt-out of receiving the valid instance information
       *     for a task.
       *
       * replicate default:false
       *     Enable replication of the individual tasks for this
       *     operation. This is useful for performing redundant
       *     computation to avoid communication. There are 
       *     requirements on the properties of replicated tasks
       *     and how they are mapped. Replicated tasks are not
       *     allowed to have reduction-only privileges. Furthermore
       *     the mapper must map any regions with write privileges
       *     for different copies of the task to different instances.
       *
       * parent_priority default:current
       *     If the mapper for the parent task permits child
       *     operations to mutate the priority of the parent task
       *     then the mapper can use this field to alter the 
       *     priority of the parent task
       *
       * check_collective_regions:empty
       *     For index space tasks, provide the indexes of any region 
       *     requirements that the runtime should check for collective
       *     mappings between the point tasks.
       */
      struct TaskOptions {
        Processor                              initial_proc; // = current
        bool                                   inline_task;  // = false
        bool                                   stealable;    // = false
        bool                                   map_locally;  // = false
        bool                                   valid_instances; // = true
        bool                                   memoize;  // = false
        bool                                   replicate;    // = false
        TaskPriority                           parent_priority; // = current
        std::set<unsigned>                     check_collective_regions;
      };
      //------------------------------------------------------------------------
      virtual void select_task_options(MapperContext          ctx,
                                       const Task&            task,
                                             TaskOptions&     output) = 0;
      //------------------------------------------------------------------------

      /**
       * ----------------------------------------------------------------------
       *  Premap Task (should really be called map_index_task) 
       * ----------------------------------------------------------------------
       * This mapper call is only invoked for index space task launches. It
       * will invoked if at least one of the following two conditions occur:
       * 1. The task is performing a reduction of its point task futures down
       *    to a single future value as an output, in which case the mapper
       *    needs to select one or more locations for the futures to go.
       * 2. (No longer applies) 
       *
       * In the case of (1), the mapper can optionally choose to fill in 
       * the 'reduction_futures' vector with one or more memories in which 
       * to create a copy of the reduced future output. If multiple such
       * destinations are specified, the runtime will construct a broadcast
       * tree to make the copies efficiently. We allo the 'reduction_instances'
       * data structure to be left empty for backwards compatibility. In this
       * case the runtime will create a single copy of the future in the 
       * local system memory.
       */
      struct PremapTaskInput {
        LEGION_DEPRECATED("Premapping regions is no longer supported")
        std::map<unsigned,std::vector<PhysicalInstance> >  valid_instances;
        PremapTaskInput(void);
        ~PremapTaskInput(void);
      };
      struct PremapTaskOutput {
        Processor                                          new_target_proc;
        std::vector<Memory>                                reduction_futures;
        LEGION_DEPRECATED("Premapping regions is no longer supported")
        std::map<unsigned,std::vector<PhysicalInstance> >  premapped_instances;
        LEGION_DEPRECATED("Premapping regions is no longer supported")
        std::map<unsigned,std::vector<PhysicalInstance> >  premapped_sources;
        LEGION_DEPRECATED("Premapping regions is no longer supported")
        ProfilingRequest                                   copy_prof_requests;
        LEGION_DEPRECATED("Premapping regions is no longer supported")
        TaskPriority                                       profiling_priority;
        PremapTaskOutput(void);
        ~PremapTaskOutput(void);
      };
      //------------------------------------------------------------------------
      virtual void premap_task(MapperContext            ctx,
                               const Task&              task, 
                               const PremapTaskInput&   input,
                               PremapTaskOutput&        output) = 0; 
      //------------------------------------------------------------------------

      /**
       * ----------------------------------------------------------------------
       *  Slice Domain 
       * ----------------------------------------------------------------------
       * Instead of needing to map an index space of tasks as a single
       * domain, Legion allows index space of tasks to be decomposed
       * into smaller sets of tasks that are mapped in parallel on
       * different processors. To achieve this, the domain of the
       * index space task launch must be sliced into subsets of points
       * and distributed to the different processors which will actually
       * run the tasks. Decomposing arbitrary domains in a way that
       * matches the target architecture is clearly a mapping decision.
       * Slicing the domain can be done recursively to match the 
       * hierarchical nature of modern machines. By setting the
       * 'recurse' field on a DomainSlice struct to true, the runtime
       * will invoke slice_domain again on the destination node.
       * It is acceptable to return a single slice consisting of the
       * entire domain, but this will guarantee that all points in 
       * an index space will map on the same node. The mapper can 
       * request that the runtime check the correctness of the slicing
       * (e.g. each point is in exactly one slice) dynamically by setting
       * the 'verify_correctness' flag. Note that verification can be
       * expensive and should only be used in testing or rare cases.
       */
      struct TaskSlice {
      public:
        TaskSlice(void) : domain_is(IndexSpace::NO_SPACE), 
          domain(Domain::NO_DOMAIN), proc(Processor::NO_PROC), 
          recurse(false), stealable(false) { }
        TaskSlice(const Domain &d, Processor p, bool r, bool s)
          : domain_is(IndexSpace::NO_SPACE), domain(d), 
            proc(p), recurse(r), stealable(s) { }
        TaskSlice(IndexSpace is, Processor p, bool r, bool s)
          : domain_is(is), domain(Domain::NO_DOMAIN),
            proc(p), recurse(r), stealable(s) { }
      public:
        IndexSpace                              domain_is;
        Domain                                  domain;
        Processor                               proc;
        bool                                    recurse;
        bool                                    stealable;
      };
      struct SliceTaskInput {
        IndexSpace                             domain_is;
        Domain                                 domain;
        IndexSpace                             sharding_is;
      };
      struct SliceTaskOutput {
        std::vector<TaskSlice>                 slices;
        bool                                   verify_correctness; // = false
      };
      //------------------------------------------------------------------------
      virtual void slice_task(MapperContext            ctx,
                              const Task&              task, 
                              const SliceTaskInput&    input,
                                    SliceTaskOutput&   output) = 0;
      //------------------------------------------------------------------------

      /**
       * ----------------------------------------------------------------------
       *  Map Task 
       * ----------------------------------------------------------------------
       * The map task call is performed on every task which is eagerly
       * (as opposed to lazily) executed and has all its input already
       * eagerly executed. The input to map_task consists of the names
       * of any valid instances that the runtime knows about for each
       * of the individual region requirements stored in 'valid_instances'
       * (if the user requested them by setting 'valid_instances' to 'true'
       * in the select_task_options mapper call), and the indexes of any
       * regions which were premapped in 'premapped_regions'.
       *
       * The mapper must first select a set of 'chosen_instances' to use
       * for each region requirement of the task. Multiple instances can
       * be chosen for each region requirement (hence the vector of vectors)
       * but the runtime will use the first instance that has space for each
       * field in the vector of instances for all the fields in the region
       * requirement. For read-only region requirements, the mapper can 
       * optionally request that the runtime not track the instances used
       * for read-only region requirements with the 'untracked_valid_regions'.
       * This will ensure that read-only instances are not considered a 
       * long-term valid copy of the data and make them immediately eligible
       * for garbage collection after the task is done mapping. Only the
       * indexes of read-only region requirements should be specified.
       *
       * In addition to mapping regions for the task, the mapper can also
       * specify a memory to use for each of the futures of the task. The
       * entries in this vector will be zipped with the vector of futures
       * in the 'Task' object to determine which memory to map each future.
       *
       * The mapper must also select a set of 'target_procs'
       * that specifies the target processor(s) on which the task can run.
       * If a single processor is chosen then the task is guaranteed to 
       * run on that processor. If multiple processors are specified, 
       * the runtime will run the task on the first procoessor that becomes
       * available. All of the processors must be on the same node and of 
       * the same kind for now. 
       *
       * The mapper must further select a task variant to use to execute
       * the task and specify its VariantID in 'chosen_variant'. This variant
       * must have execution constraints consistent with all the 'target_procs'.
       * All of the instances specified by 'chosen_instances' must be in 
       * memories visible to all the target processors or the variant must speicfy
       * 'no_access' specialized constraints for such region requirements.
       * The mapper can specify a priority for the task with the 'task_priority'
       * field. This will allow the task to be re-ordered ahead of lower
       * priority tasks and behind higher priority tasks by the runtime
       * as it's being dynamically scheduled. Negative priorities are lower
       * and positive priorities are higher. The 'copy_fill_priority' field
       * can control the probabilities of any copies and fills performed on 
       * behalf of the task. 
       *
       * The mapper can request profiling information about this
       * task as part of its execution. The mapper can specify a task
       * profiling request set in 'task_prof_requests' for profiling
       * statistics about the execution of the task. The mapper can
       * also ask for profiling information for the copies generated
       * as part of the mapping of the task through the 
       * 'copy_prof_requests' field. The 'profiling_priority' field
       * indicates with which priority the profiling results should
       * be send back to the mapper.
       *
       * Finally, the mapper can requrest a postmap_task mapper call be
       * performed to make additional copies of any output regions of the
       * task for resilience purposes by setting the 'postmap_task' flag
       * to true.
       */
      struct MapTaskInput {
        std::vector<std::vector<PhysicalInstance> > valid_instances;
        std::vector<std::vector<CollectiveView> >   valid_collectives;
        std::vector<unsigned>                       premapped_regions;
        // These only apply when mapping a replicated task
        DomainPoint                                 shard;
        Domain                                      shard_domain;
        Processor                                   shard_processor;
        VariantID                                   shard_variant;
      };
      struct MapTaskOutput {
        std::vector<std::vector<PhysicalInstance> > chosen_instances; 
        std::vector<std::vector<PhysicalInstance> > source_instances;
        std::vector<Memory>                         output_targets;
        std::vector<LayoutConstraintSet>            output_constraints;
        std::set<unsigned>                          untracked_valid_regions;
        std::vector<Memory>                         future_locations;
        std::vector<Processor>                      target_procs;
        VariantID                                   chosen_variant; // = 0 
        TaskPriority                                task_priority;  // = 0
        RealmPriority                               copy_fill_priority;
        RealmPriority                               profiling_priority;
        ProfilingRequest                            task_prof_requests;
        ProfilingRequest                            copy_prof_requests;
        bool                                        postmap_task; // = false
      };
      //------------------------------------------------------------------------
      virtual void map_task(MapperContext            ctx,
                            const Task&              task,
                            const MapTaskInput&      input,
                                  MapTaskOutput&     output) = 0;
      //------------------------------------------------------------------------

      /**
       * ----------------------------------------------------------------------
       *  Replicate Task
       * ----------------------------------------------------------------------
       * This mapper call is invoked if the 'replicate' parameter was set in
       * select_task_options. It provides the mapper the options to replicate
       * the execution of this task on multiple different processors. All the
       * copies of the task must use the same variant which must be set as
       * supporting replication. If the variant is a non-leaf variant then the
       * execution will be control-replicated.
       *
       * Note that if the task has any region requirements with write or
       * reduction privileges then it will be incumbent upon the mapper to
       * ensure that each of the different copies of the task are mapped to
       * different physical instances. This invariant will be verified by the
       * runtime if safe mapping is enabled.
       *
       * The mapper can optionally give names to the shards by filling in the
       * 'shard_points' vector with a set of unique points, all which must be
       * of the same dimension. The 'shard_points' vector must either be empty
       * or be of the same size as the 'task_mappings'. The mapper can also 
       * provide an optional 'shard_domain' value to describe the set of points.
       * If this is provided the runtime does not introspect it other than to
       * check that its dimensionality matches that of the points. This value
       * is then passed as the 'shard_domain' argument to all invocations of a
       * sharding functor for operations launched by these shards.
       */
      struct ReplicateTaskInput {
        // Nothing here for now
      };
      struct ReplicateTaskOutput {
        VariantID                                     chosen_variant;
        std::vector<Processor>                        target_processors;
        // The following outputs are optional
        std::vector<DomainPoint>                      shard_points;
        Domain                                        shard_domain;
      };
      //------------------------------------------------------------------------
      virtual void replicate_task(MapperContext               ctx,
                                  const Task&                 task,
                                  const ReplicateTaskInput&   input,
                                        ReplicateTaskOutput&  output) = 0;
      //------------------------------------------------------------------------

      // This is here for backwards compatibility
      // The mapper call it was used by no longer exists
      // It was replaced by replicate_task
      struct MapReplicateTaskOutput {
        std::vector<MapTaskOutput>                      task_mappings;
        std::vector<Processor>                          control_replication_map;
        std::vector<DomainPoint>                        shard_points;
        Domain                                          shard_domain;
      };

      /**
       * ----------------------------------------------------------------------
       *  Select Task Variant 
       * ----------------------------------------------------------------------
       * This mapper call will only invoked if a task selected to be inlined.
       * If there is only one choice for the task variant the runtime will 
       * not invoke this method. However, if there are multiple valid variants
       * for this task given the processor and parent task physical regions,
       * then this call will be invoked to select the correct variant.
       */
      struct SelectVariantInput {
        Processor                                       processor;
        std::vector<std::vector<PhysicalInstance> >     chosen_instances;
      };
      struct SelectVariantOutput {
        VariantID                                       chosen_variant;
      };
      //------------------------------------------------------------------------
      virtual void select_task_variant(MapperContext                ctx,
                                       const Task&                  task,
                                       const SelectVariantInput&    input,
                                             SelectVariantOutput&   output) = 0;
      //------------------------------------------------------------------------

      /**
       * ----------------------------------------------------------------------
       *  Postmap Task 
       * ----------------------------------------------------------------------
       * This call will only be invoked if the postmap_task field was set
       * in the 'select_task_options' call. The postmap task call gives the
       * mapper the option to create additional copies of the output in 
       * different memories. The mapper is told about the mapped regions for
       * each of the different region requirements for the task in 
       * 'mapped_regions', as well of any currently valid physical instances
       * for those regions in the set of 'valid_instances' for each region
       * requirement. The mapper can then specify one or more new instances
       * to update with the output from the task for each region requirement.
       * Unlike map_task where the chosen_instances are filtered so that only
       * the first instance which has space for a given field is updated, each
       * instances specified in 'chosen_instances' will be updated for any 
       * fields of the original region requirement for which they have 
       * sufficient space. 
       */
      struct PostMapInput {
        std::vector<std::vector<PhysicalInstance> >     mapped_regions;
        std::vector<std::vector<PhysicalInstance> >     valid_instances;
        std::vector<std::vector<CollectiveView> >       valid_collectives;
      };
      struct PostMapOutput {
        std::vector<std::vector<PhysicalInstance> >     chosen_instances;
        std::vector<std::vector<PhysicalInstance> >     source_instances;
      };
      //------------------------------------------------------------------------
      virtual void postmap_task(MapperContext            ctx,
                                const Task&              task,
                                const PostMapInput&      input,
                                      PostMapOutput&     output) = 0;
      //------------------------------------------------------------------------

      /**
       * ----------------------------------------------------------------------
       *  Select Task Sources 
       * ----------------------------------------------------------------------
       * The rank copy sources mapper call allows for the mapper to 
       * select a ranking of potential source physical instances when
       * making a copy to a new physical instance. The mapper is given
       * the 'target_instance' and the set of 'source_instances' and
       * asked to provide the 'chosen_ranking' of the physical instances.
       * The runtime will issue copies from unranking instances in an
       * undefined order until all fields have valid data. The
       * 'region_req_index' field indicates the index of the region
       * requirement for which this copy is being requested.
       */
      struct SelectTaskSrcInput {
        PhysicalInstance                        target;
        std::vector<PhysicalInstance>           source_instances;
        std::vector<CollectiveView>             collective_views;
        unsigned                                region_req_index;
      };
      struct SelectTaskSrcOutput {
        std::deque<PhysicalInstance>            chosen_ranking;
      };
      //------------------------------------------------------------------------
      virtual void select_task_sources(MapperContext              ctx,
                                       const Task&                task,
                                       const SelectTaskSrcInput&  input,
                                             SelectTaskSrcOutput& output) = 0;
      //------------------------------------------------------------------------

      // These are here for backwards compatibility
      // The mapper call these were used by no longer exists
      struct CreateTaskTemporaryInput {
        unsigned                                region_requirement_index; 
        PhysicalInstance                        destination_instance;
      };
      struct CreateTaskTemporaryOutput {
        PhysicalInstance                        temporary_instance; 
      };

      // Keep this struct around for backwards compatibility 
      struct SpeculativeOutput {
        bool                                    speculate;
        bool                                    speculative_value;
        bool                                    speculate_mapping_only;
      };

      /**
       * ----------------------------------------------------------------------
       *  Report Profiling
       * ----------------------------------------------------------------------
       * This mapper call will report the profiling information
       * requested either for the task execution and/or any copy
       * operations that were issued on behalf of mapping the task.
       * If the 'task_response' field is set to true this is the
       * profiling callback for the task itself, otherwise it is a
       * callback for one of the copies for the task'. If it is a
       * response for a copy the 'region_requirement_index' will say
       * for which region requirement the copy was issued. The runtime
       * will also report the number of 'total_reports' to expect. There
       * will always be at least one of these if the number of copy or
       * task profiling requests is not empty.
       */
      struct TaskProfilingInfo {
	ProfilingResponse                       profiling_responses;
        unsigned                                region_requirement_index;
        unsigned                                total_reports;
        bool                                    task_response;
        bool                                    fill_response;
      };
      //------------------------------------------------------------------------
      virtual void report_profiling(MapperContext            ctx,
                                    const Task&              task,
                                    const TaskProfilingInfo& input)  = 0;
      //------------------------------------------------------------------------
      
      /**
       * ----------------------------------------------------------------------
       *  Select Sharding Functor 
       * ----------------------------------------------------------------------
       * This mapper call is invoked whenever the enclosing parent
       * task for the task being launched has been control replicated
       * and it's up to the mapper for this task to pick a sharding
       * functor to determine which shard will own the point(s) of the
       * task. The mapper must return the same sharding functor for all
       * copies of the task. The runtime will verify this in debug mode
       * but not in release mode. In the case of sharding index space
       * tasks, the mapper can also specify whether the resulting slice
       * should be recursively sliced or not using 'slice_recurse'.
       */
      struct SelectShardingFunctorInput {
        std::vector<Processor>                  shard_mapping;
      };
      struct SelectShardingFunctorOutput {
        ShardingID                              chosen_functor;
        bool                                    slice_recurse;
      };
      //------------------------------------------------------------------------
      virtual void select_sharding_functor(MapperContext            ctx,
                                 const Task&                        task,
                                 const SelectShardingFunctorInput&  input,
                                       SelectShardingFunctorOutput& output) = 0;
      //------------------------------------------------------------------------
    public: // Inline mapping
      /**
       * ----------------------------------------------------------------------
       *  Map Inline 
       * ----------------------------------------------------------------------
       * The map inline mapper call is responsible for handling the mapping
       * of an inline mapping operation to a specific physical region. The
       * mapper is given a set of valid physical instances in the 
       * 'valid_instances' field. The mapper must then specify a set of chosen
       * instances to use for the inline mapping operation in 
       * 'chosen_instances'. Multiple instances can be selected for different
       * fields but the runtime will use the first instance that it finds that
       * in the vector that has space for each field. If this is a read-only 
       * inline mapping, the mapper can request that the runtime not track the 
       * validity of the instance(s) used for the inline mapping by setting 
       * 'track_valid_region' to 'false'. The 'copy_fill_priority' field will
       * control the priorities of any copies or fills needed for the mapping.
       *
       * The mapper can also request profiling information for any copies 
       * issued by filling in the 'profiling_requests' set. The mapper can 
       * control the priority with which this profiling information is 
       * returned to the mapper with 'profiling priority'.
       */
      struct MapInlineInput {
        std::vector<PhysicalInstance>           valid_instances; 
        std::vector<CollectiveView>             valid_collectives;
      };
      struct MapInlineOutput {
        std::vector<PhysicalInstance>           chosen_instances;
        std::vector<PhysicalInstance>           source_instances;
        RealmPriority                           copy_fill_priority;
        ProfilingRequest                        profiling_requests;
        RealmPriority                           profiling_priority;
        bool                                    track_valid_region; /*=true*/
      };
      //------------------------------------------------------------------------
      virtual void map_inline(MapperContext              ctx,
                              const InlineMapping&       inline_op,
                              const MapInlineInput&      input,
                                    MapInlineOutput&     output) = 0;
      //------------------------------------------------------------------------

      /**
       * ----------------------------------------------------------------------
       *  Select Inline Sources 
       * ----------------------------------------------------------------------
       * The rank copy sources mapper call allows for the mapper to select a
       * ranking for source physical instances when generating copies for an
       * inline mapping. The mapper is given the target physical instance in
       * the 'target' field and the set of possible source instances in 
       * 'source_instances'. The mapper speciefies a ranking of physical 
       * instances for copies to be issued from until all the fields contain 
       * valid data. The runtime will also issue copies from any instances not 
       * placed in the ranking in an unspecified order.
       */
      struct SelectInlineSrcInput {
        PhysicalInstance                        target;
        std::vector<PhysicalInstance>           source_instances;
        std::vector<CollectiveView>             collective_views;
      };
      struct SelectInlineSrcOutput {
        std::deque<PhysicalInstance>            chosen_ranking;
      };
      //------------------------------------------------------------------------
      virtual void select_inline_sources(MapperContext              ctx,
                                       const InlineMapping&         inline_op,
                                       const SelectInlineSrcInput&  input,
                                             SelectInlineSrcOutput& output) = 0;
      //------------------------------------------------------------------------
      
      // These are here for backwards compatibility
      // The mapper call these were used by no longer exists
      struct CreateInlineTemporaryInput {
        PhysicalInstance                        destination_instance;
      };
      struct CreateInlineTemporaryOutput {
        PhysicalInstance                        temporary_instance;
      };

      // No speculation for inline mappings

      /**
       * ----------------------------------------------------------------------
       *  Report Profiling 
       * ----------------------------------------------------------------------
       * If the mapper requested profiling information on the copies
       * generated during an inline mapping operations then this mapper
       * call will be invoked to inform the mapper of the result. You are
       * always guaranteed to get at least one of these calls if the number
       * of profiling requests is not empty even if there are no copies or
       * fills performed in order to report the 'total_reports' to be expected. 
       * If the number of 'total_reports' is zero then the that means that
       * no copies or fills were generated and this is in the only response
       * that should be expected and it contains no other valid information.
       */
      struct InlineProfilingInfo {
	ProfilingResponse                       profiling_responses;
        unsigned                                total_reports;
        bool                                    fill_response;
      };
      //------------------------------------------------------------------------
      virtual void report_profiling(MapperContext               ctx,
                                    const InlineMapping&        inline_op,
                                    const InlineProfilingInfo&  input)  = 0;
      //------------------------------------------------------------------------
    public: // Region-to-region copies
      /**
       * ----------------------------------------------------------------------
       *  Map Copy 
       * ----------------------------------------------------------------------
       * When an application requests an explicit region-to-region copy, this
       * mapper call is invoked to map both the source and destination 
       * instances for the copy. The mapper is provided with a set of valid
       * instances to be used for both the source and destination region
       * requirements in the 'src_instances' and 'dst_instances' fields.
       * The mapper then picks the chosen instances for the source and
       * destination region requirements and puts them in the corresponding
       * vectors of the output structure. The mapper can specify multiple
       * instances for different fields. For each field the runtime will select
       * the instance that first has space for that field that it finds in
       * the vector of instances. For source region requirements the mapper
       * can optionally select to use a virtual mapping if the copy is not
       * a reduction copy. If the copy is a gather or a scatter copy then 
       * the mapper must also create instances for the source and/or destination
       * indirection region requirements as well. The mapper can specify the
       * priority of any copies or fills required for executing this copy
       * operation using the 'copy_fill_priority' field.
       *
       * The mapper can optionally choose not to have the runtime track any
       * of the instances made for the copy as valid for the source or 
       * indirection region requirements by specifying indexes of the valid
       * region requirements in 'untracked_valid_srcs', 
       * 'untracked_valid_ind_srcs', or 'untracked_valid_ind_dsts' respectively.
       *
       * The mapper can request profiling feedback on any copies performed by
       * this copy operation by filling in the 'profiling_requests' data 
       * structure with the kind of measurements desired. The priority
       * with which this information is sent back to the mapper can be 
       * set with 'profiling_priority'.
       *
       * The mapper can say whether or not the runtime should compute preimages
       * for any indirection fields in the copy operation. This will incur an
       * additional latency in the copy operation, but can reduce the number
       * of instances that must be investigated for performing the indirect
       * copies which can improve overall performance and scalability. The
       * default is not to compute the preimages.
       */
      struct MapCopyInput {
        std::vector<std::vector<PhysicalInstance> >   src_instances;
        std::vector<std::vector<PhysicalInstance> >   dst_instances;
        std::vector<std::vector<PhysicalInstance> >   src_indirect_instances;
        std::vector<std::vector<PhysicalInstance> >   dst_indirect_instances;
        std::vector<std::vector<CollectiveView> >     src_collectives;
        std::vector<std::vector<CollectiveView> >     dst_collectives;
        std::vector<std::vector<CollectiveView> >     src_indirect_collectives;
        std::vector<std::vector<CollectiveView> >     dst_indirect_collectives;
      };
      struct MapCopyOutput {
        std::vector<std::vector<PhysicalInstance> >   src_instances;
        std::vector<std::vector<PhysicalInstance> >   dst_instances;
        std::vector<PhysicalInstance>                 src_indirect_instances;
        std::vector<PhysicalInstance>                 dst_indirect_instances;
        std::vector<std::vector<PhysicalInstance> >   src_source_instances;
        std::vector<std::vector<PhysicalInstance> >   dst_source_instances;
        std::vector<std::vector<PhysicalInstance> >   src_indirect_source_instances;
        std::vector<std::vector<PhysicalInstance> >   dst_indirect_source_instances;

        std::set<unsigned>                            untracked_valid_srcs;
        std::set<unsigned>                            untracked_valid_ind_srcs;
        std::set<unsigned>                            untracked_valid_ind_dsts;
        ProfilingRequest                              profiling_requests;
        RealmPriority                                 profiling_priority;
        RealmPriority                                 copy_fill_priority;
        bool                                          compute_preimages;
      };
      //------------------------------------------------------------------------
      virtual void map_copy(MapperContext            ctx,
                            const Copy&              copy,
                            const MapCopyInput&      input,
                                  MapCopyOutput&     output) = 0;
      //------------------------------------------------------------------------

      /**
       * ----------------------------------------------------------------------
       *  Select Copy Sources 
       * ----------------------------------------------------------------------
       * The select copy sources mapper call allows the mapper to select a
       * ranking of physical instances to use when updating the fields for
       * a target physical instance. The physical instance is specified in 
       * the 'target' field and the set of source physical instances are
       * in the 'source_instances'. The 'is_src' and 'region_req_index'
       * say which region requirement the copy is being issued. The mapper
       * can specify an optional ranking in the 'chosen_ranking' field.
       * The runtime will issue copies from the chosen ranking until all
       * the fields in the target are made valid. Any instances not put
       * in the chosen ranking will be considered by the runtime in an 
       * undefined order for updating valid fields.
       */
      struct SelectCopySrcInput {
        PhysicalInstance                              target;
        std::vector<PhysicalInstance>                 source_instances;
        std::vector<CollectiveView>                   collective_views;
        bool                                          is_src;
        bool                                          is_dst;
        bool                                          is_src_indirect;
        bool                                          is_dst_indirect;
        unsigned                                      region_req_index;
      };
      struct SelectCopySrcOutput {
        std::deque<PhysicalInstance>                  chosen_ranking;
      };
      //------------------------------------------------------------------------
      virtual void select_copy_sources(MapperContext                ctx,
                                       const Copy&                  copy,
                                       const SelectCopySrcInput&    input,
                                             SelectCopySrcOutput&   output) = 0;
      //------------------------------------------------------------------------
      
      // These are here for backwards compatibility
      // The mapper call these were used by no longer exists
      struct CreateCopyTemporaryInput {
        unsigned                                region_requirement_index; 
        bool                                    src_requirement;
        PhysicalInstance                        destination_instance;
      };
      struct CreateCopyTemporaryOutput {
        PhysicalInstance                        temporary_instance;
      };

      /**
       * ----------------------------------------------------------------------
       *  Report Profiling 
       * ----------------------------------------------------------------------
       * If the mapper requested profiling information for an explicit
       * copy operation then this call will return the profiling information.
       * The 'src_index' and 'dst_index' fields will report which region
       * requiremnts were responsible for generating the copy. The 
       * 'fill_response' field says whether this is a fill operation or a
       * copy operation. You are always guaranteed to get at least one of 
       * these calls if the number of profiling requests is non-zero even 
       * if there are no copies or fills performed in order to report the 
       * 'total_reports' to be expected. If the number of 'total_reports' 
       * is zero then the that means that no copies or fills were generated 
       * and this is in the only response that should be expected and it 
       * contains no other valid information. 
       */
      struct CopyProfilingInfo {
	ProfilingResponse                       profiling_responses;
        unsigned                                src_index;
        unsigned                                dst_index;
        unsigned                                total_reports;
        bool                                    fill_response;
      };
      //------------------------------------------------------------------------
      virtual void report_profiling(MapperContext            ctx,
                                    const Copy&              copy,
                                    const CopyProfilingInfo& input)  = 0;
      //------------------------------------------------------------------------

      /**
       * ----------------------------------------------------------------------
       *  Select Sharding Functor 
       * ----------------------------------------------------------------------
       * This mapper call is invoked whenever the enclosing parent
       * task for the copy being launched has been control replicated
       * and it's up to the mapper for this copy to pick a sharding
       * functor to determine which shard will own the point(s) of the
       * copy. The mapper must return the same sharding functor for all
       * instances of the copy. The runtime will verify this in debug mode
       * but not in release mode.
       */
      //------------------------------------------------------------------------
      virtual void select_sharding_functor(MapperContext            ctx,
                                 const Copy&                        copy,
                                 const SelectShardingFunctorInput&  input,
                                       SelectShardingFunctorOutput& output) = 0;
      //------------------------------------------------------------------------
    public: // Close operations
      // These are here for backwards compatibility
      // The mapper call these were used by no longer exists
      struct MapCloseInput {
        std::vector<PhysicalInstance>               valid_instances;
      };
      struct MapCloseOutput {
        std::vector<PhysicalInstance>               chosen_instances;
        ProfilingRequest                            profiling_requests;
        RealmPriority                               profiling_priority;
      };

      /**
       * ----------------------------------------------------------------------
       *  Select Close Sources 
       * ----------------------------------------------------------------------
       * The rank copy sources mapper call will be invoked whenever multiple
       * physical instances can serve as the source for a copy aimed at the
       * 'target' physical instance. The possible source instances are named
       * in 'source_instances' and the mapper can specify a ranking in 
       * 'chosen_ranking'. Any instances not explicitly listed in the order
       * will be used by the runtime in an undefined order.
       */
      struct SelectCloseSrcInput {
        PhysicalInstance                            target;
        std::vector<PhysicalInstance>               source_instances;
        std::vector<CollectiveView>                 collective_views;
      };
      struct SelectCloseSrcOutput {
        std::deque<PhysicalInstance>                chosen_ranking;
      };
      //------------------------------------------------------------------------
      virtual void select_close_sources(MapperContext              ctx,
                                        const Close&               close,
                                        const SelectCloseSrcInput&  input,
                                              SelectCloseSrcOutput& output) = 0;
      //------------------------------------------------------------------------

      // These are here for backwards compatibility
      // The mapper call these were used by no longer exists
      struct CreateCloseTemporaryInput {
        PhysicalInstance                        destination_instance;
      };
      struct CreateCloseTemporaryOutput {
        PhysicalInstance                        temporary_instance;
      };

      // No speculation for close operations

      /**
       * ----------------------------------------------------------------------
       *  Report Profiling 
       * ----------------------------------------------------------------------
       * If the mapper requested profiling information this close operation
       * then this call will return the profiling data back to the mapper
       * for all the copy operations issued by the close operation. The
       * 'fill_response' field indicates whether this response is for a
       * fill operation. You are always guaranteed to get at least one of 
       * these calls if the number of profiling requests is not empty even 
       * if there are no copies of fills performed in order to report the 
       * 'total_reports' to be expected. If the number of 'total_reports' 
       * is zero then the that means that no copies or fills were generated 
       * and this is in the only response that should be expected and it 
       * contains no other valid information. 
       */
      struct CloseProfilingInfo {
	ProfilingResponse                       profiling_responses;
        unsigned                                total_reports;
        bool                                    fill_response;
      };
      //------------------------------------------------------------------------
      virtual void report_profiling(MapperContext             ctx,
                                    const Close&              close,
                                    const CloseProfilingInfo& input)  = 0;
      //------------------------------------------------------------------------

      /**
       * ----------------------------------------------------------------------
       *  Select Sharding Functor 
       * ----------------------------------------------------------------------
       * This mapper call is invoked whenever the enclosing parent
       * task for the close being launched has been control replicated
       * and it's up to the mapper for this task to pick a sharding
       * functor to determine which shard will own the point(s) of the
       * close. The mapper must return the same sharding functor for all
       * instances of the close. The runtime will verify this in debug mode
       * but not in release mode.
       */
      //------------------------------------------------------------------------
      virtual void select_sharding_functor(MapperContext            ctx,
                                 const Close&                       close,
                                 const SelectShardingFunctorInput&  input,
                                       SelectShardingFunctorOutput& output) = 0;
      //------------------------------------------------------------------------
    public: // Acquire operations
      /**
       * ----------------------------------------------------------------------
       *  Map Acquire 
       * ----------------------------------------------------------------------
       * Acquire operations do not actually need to be mapped since they
       * are explicitly tied to a physical region when they are launched.
       * Therefore the only information needed from the mapper is whether
       * it would like to request any profiling information.
       */
      struct MapAcquireInput {
        // Nothing
      };
      struct MapAcquireOutput {
        ProfilingRequest                            profiling_requests;
        RealmPriority                               profiling_priority;
        RealmPriority                               copy_fill_priority;
      };
      //------------------------------------------------------------------------
      virtual void map_acquire(MapperContext               ctx,
                               const Acquire&              acquire,
                               const MapAcquireInput&      input,
                                     MapAcquireOutput&     output) = 0;
      //------------------------------------------------------------------------

      /**
       * ----------------------------------------------------------------------
       *  Report Profiling
       * ----------------------------------------------------------------------
       * If the mapper requested profiling information on this acquire
       * operation, then this call will be invoked with the associated
       * profiling data. You are always guaranteed to get at least one of 
       * these calls if the number of profiling requests is not empty even 
       * if there are no copies or fills performed in order to report the 
       * 'total_reports' to be expected. If the number of 'total_reports' 
       * is zero then the that means that no copies or fills were generated 
       * and this is in the only response that should be expected and it 
       * contains no other valid information.  
       */
      struct AcquireProfilingInfo {
	ProfilingResponse                       profiling_responses;
        unsigned                                total_reports;
        bool                                    fill_response;
      };
      //------------------------------------------------------------------------
      virtual void report_profiling(MapperContext               ctx,
                                    const Acquire&              acquire,
                                    const AcquireProfilingInfo& input) = 0;
      //------------------------------------------------------------------------

      /**
       * ----------------------------------------------------------------------
       *  Select Sharding Functor 
       * ----------------------------------------------------------------------
       * This mapper call is invoked whenever the enclosing parent
       * task for the acquire being launched has been control replicated
       * and it's up to the mapper for this task to pick a sharding
       * functor to determine which shard will own the point(s) of the
       * acquire . The mapper must return the same sharding functor for all
       * instances of the acquire. The runtime will verify this in debug mode
       * but not in release mode.
       */
      //------------------------------------------------------------------------
      virtual void select_sharding_functor(MapperContext            ctx,
                                 const Acquire&                     acquire,
                                 const SelectShardingFunctorInput&  input,
                                       SelectShardingFunctorOutput& output) = 0;
      //------------------------------------------------------------------------
    public: // Release operations 
      /**
       * ----------------------------------------------------------------------
       *  Map Release 
       * ----------------------------------------------------------------------
       * Release operations don't actually have any mapping to perform since
       * they are explicitly associated with a physical instance when they
       * are launched by the application. Thereforefore the only output
       * currently neecessary is whether the mapper would like profiling
       * information for this release operation. The mapper can control
       * the priority of any copies or fills needed for flushing data back
       * to the restricted instances using the 'copy_fill_priority' field.
       */
      struct MapReleaseInput {
        // Nothing
      };
      struct MapReleaseOutput {
        std::vector<PhysicalInstance>               source_instances;
        ProfilingRequest                            profiling_requests;
        RealmPriority                               profiling_priority;
        RealmPriority                               copy_fill_priority;
      };
      //------------------------------------------------------------------------
      virtual void map_release(MapperContext               ctx,
                               const Release&              release,
                               const MapReleaseInput&      input,
                                     MapReleaseOutput&     output) = 0;
      //------------------------------------------------------------------------

      /**
       * ----------------------------------------------------------------------
       *  Select Release Sources 
       * ----------------------------------------------------------------------
       * The select release sources call allows mappers to specify a 
       * 'chosen_ranking' for different 'source_instances' of a region when 
       * copying to a 'target' phsyical instance. The mapper can rank any or 
       * all of the source instances and any instances which are not ranked 
       * will be copied from in an unspecified order by the runtime until all 
       * the necessary fields in the target contain valid data.
       */
      struct SelectReleaseSrcInput {
        PhysicalInstance                        target;
        std::vector<PhysicalInstance>           source_instances;
        std::vector<CollectiveView>             collective_views;
      };
      struct SelectReleaseSrcOutput {
        std::deque<PhysicalInstance>            chosen_ranking;
      };
      //------------------------------------------------------------------------
      virtual void select_release_sources(MapperContext             ctx,
                                     const Release&                 release,
                                     const SelectReleaseSrcInput&   input,
                                           SelectReleaseSrcOutput&  output) = 0;
      //------------------------------------------------------------------------
      
      // These are here for backwards compatibility
      // The mapper call these were used by no longer exists
      struct CreateReleaseTemporaryInput {
        PhysicalInstance                        destination_instance;
      };
      struct CreateReleaseTemporaryOutput {
        PhysicalInstance                        temporary_instance;
      };

      /**
       * ----------------------------------------------------------------------
       *  Report Profiling
       * ----------------------------------------------------------------------
       * If the mapper requested profiling data for the release operation
       * then this call will be invoked to report the profiling results
       * back to the mapper. You are always guaranteed to get at least one of 
       * these calls if the number of profiling requests is not empty even if 
       * there are no copies or fills performed in order to report the 
       * 'total_reports' to be expected. If the number of 'total_reports' is 
       * zero then the that means that no copies or fills were generated and 
       * this is in the only response that should be expected and it contains 
       * no other valid information. 
       */
      struct ReleaseProfilingInfo {
	ProfilingResponse                       profiling_responses;
        unsigned                                total_reports;
        bool                                    fill_response;
      };
      //------------------------------------------------------------------------
      virtual void report_profiling(MapperContext               ctx,
                                    const Release&              release,
                                    const ReleaseProfilingInfo& input)  = 0;
      //------------------------------------------------------------------------

      /**
       * ----------------------------------------------------------------------
       *  Select Sharding Functor 
       * ----------------------------------------------------------------------
       * This mapper call is invoked whenever the enclosing parent
       * task for the release being launched has been control replicated
       * and it's up to the mapper for this task to pick a sharding
       * functor to determine which shard will own the point(s) of the
       * release. The mapper must return the same sharding functor for all
       * instances of the release. The runtime will verify this in debug mode
       * but not in release mode.
       */
      //------------------------------------------------------------------------
      virtual void select_sharding_functor(MapperContext            ctx,
                                 const Release&                     release,
                                 const SelectShardingFunctorInput&  input,
                                       SelectShardingFunctorOutput& output) = 0;
      //------------------------------------------------------------------------
    public: // Partition Operations
      /**
       * ----------------------------------------------------------------------
       *  Select Partition Projection
       * ----------------------------------------------------------------------
       * Partition operations are usually done with respect to a given
       * logical region. However, for performance reasons the data for
       * a logical region might be spread across many subregions from a
       * previous operation (e.g. in the case of create_partition_by_field
       * where a previous index space launch filled in the field containing
       * the colors). In these cases , the mapper may want to specify that
       * the mapping for the projection operation should not be done with
       * respect to the region being partitioning, but for each of the
       * subregions of a complete partition of the logical region. This
       * mapper call permits the mapper to decide whether to make the 
       * partition operation an 'index' operation over the color space
       * of a complete partition, or whether it should just remain a
       * 'single' operation that maps the logical region directly.
       * If the mapper picks a complete partition to return for 
       * 'chosen_partition' then the partition will become an 'index'
       * operation, but if it return a NO_PART, then the partition
       * operation will remain a 'single' operation.
       */
      struct SelectPartitionProjectionInput {
        std::vector<LogicalPartition>           open_complete_partitions;
      };
      struct SelectPartitionProjectionOutput {
        LogicalPartition                        chosen_partition;
      };
      //------------------------------------------------------------------------
      virtual void select_partition_projection(MapperContext        ctx,
                          const Partition&                          partition,
                          const SelectPartitionProjectionInput&     input,
                                SelectPartitionProjectionOutput&    output) = 0;
      //------------------------------------------------------------------------

      /**
       * ----------------------------------------------------------------------
       *  Map Projection 
       * ----------------------------------------------------------------------
       * The map partition mapper call is responsible for handling the mapping
       * of a dependent partitioning operation to a specific physical region. 
       * The mapper is given a set of valid physical instances in the 
       * 'valid_instances' field. The mapper must then specify a set of chosen
       * instances to use for the inline mapping operation in 
       * 'chosen_instances'. Multiple instances can be selected for different
       * fields but the runtime will use the first instance that it finds that
       * in the vector that has space for each field. Since all dependent
       * partitioning operations have read-only privileges on their input
       * regions, the mapper can request that the runtime not track the 
       * validity of the instance(s) used for the dependent parititoning
       * operation by setting 'track_valid_region' to 'false'. The 
       * 'copy_fill_priority' field specifies the priorities of any copy
       * or fills needed to bring the 'chosen_instances' up to date.
       *
       * The mapper can also request profiling information for any copies 
       * issued by filling in the 'profiling_requests' set. The mapper can 
       * control the priority with which this profiling information is 
       * returned to the mapper with 'profiling priority'.
       */
      struct MapPartitionInput {
        std::vector<PhysicalInstance>           valid_instances; 
        std::vector<CollectiveView>             valid_collectives;
      };
      struct MapPartitionOutput {
        std::vector<PhysicalInstance>           chosen_instances;
        std::vector<PhysicalInstance>           source_instances;
        ProfilingRequest                        profiling_requests;
        RealmPriority                           profiling_priority;
        RealmPriority                           copy_fill_priority;
        bool                                    track_valid_region; /*=true*/
      };
      //------------------------------------------------------------------------
      virtual void map_partition(MapperContext              ctx,
                                 const Partition&           partition,
                                 const MapPartitionInput&   input,
                                       MapPartitionOutput&  output) = 0;
      //------------------------------------------------------------------------

      /**
       * ----------------------------------------------------------------------
       *  Select Partition Sources 
       * ----------------------------------------------------------------------
       * The select partition sources mapper call allows the mapper to select a
       * ranking for source physical instances when generating copies for an
       * partition operation. The mapper is given the target physical instance 
       * in the 'target' field and the set of possible source instances in 
       * 'source_instances'. The mapper speciefies a ranking of physical 
       * instances for copies to be issued from until all the fields contain 
       * valid data. The runtime will also issue copies from any instances not 
       * placed in the ranking in an unspecified order.
       */
      struct SelectPartitionSrcInput {
        PhysicalInstance                        target;
        std::vector<PhysicalInstance>           source_instances;
        std::vector<CollectiveView>             collective_views;
      };
      struct SelectPartitionSrcOutput {
        std::deque<PhysicalInstance>            chosen_ranking;
      };
      //------------------------------------------------------------------------
      virtual void select_partition_sources(MapperContext           ctx,
                                    const Partition&                partition,
                                    const SelectPartitionSrcInput&  input,
                                          SelectPartitionSrcOutput& output) = 0;
      //------------------------------------------------------------------------

      // These are here for backwards compatibility
      // The mapper call these were used by no longer exists
      struct CreatePartitionTemporaryInput {
        PhysicalInstance                        destination_instance;
      };
      struct CreatePartitionTemporaryOutput {
        PhysicalInstance                        temporary_instance;
      };

      // No speculation for dependent partition operations

      /**
       * ----------------------------------------------------------------------
       *  Report Profiling 
       * ----------------------------------------------------------------------
       * If the mapper requested profiling information on the copies
       * generated during a dependent partition operation then this mapper
       * call will be invoked to inform the mapper of the result. You are
       * always guaranteed to get at least one of these calls if the number
       * of profiling requests is not empty even if there are no copies or
       * fills performed in order to report the 'total_reports' to be expected. 
       * If the number of 'total_reports' is zero then the that means that
       * no copies or fills were generated and this is in the only response
       * that should be expected and it contains no other valid information.
       */
      struct PartitionProfilingInfo {
	ProfilingResponse                       profiling_responses;
        unsigned                                total_reports;
        bool                                    fill_response;
      };
      //------------------------------------------------------------------------
      virtual void report_profiling(MapperContext                    ctx,
                                    const Partition&                 partition,
                                    const PartitionProfilingInfo&    input) = 0;
      //------------------------------------------------------------------------

      /**
       * ----------------------------------------------------------------------
       *  Select Sharding Functor 
       * ----------------------------------------------------------------------
       * This mapper call is invoked whenever the enclosing parent
       * task for the partition being launched has been control replicated
       * and it's up to the mapper for this task to pick a sharding
       * functor to determine which shard will own the point(s) of the
       * partition. The mapper must return the same sharding functor for all
       * instances of the partition. The runtime will verify this in debug mode
       * but not in release mode.
       */
      //------------------------------------------------------------------------
      virtual void select_sharding_functor(MapperContext            ctx,
                                 const Partition&                   partition,
                                 const SelectShardingFunctorInput&  input,
                                       SelectShardingFunctorOutput& output) = 0;
      //------------------------------------------------------------------------
    public: // Fill Operations
      /**
       * ----------------------------------------------------------------------
       *  Select Sharding Functor 
       * ----------------------------------------------------------------------
       * This mapper call is invoked whenever the enclosing parent
       * task for the fill being launched has been control replicated
       * and it's up to the mapper for this task to pick a sharding
       * functor to determine which shard will own the points of the
       * fill. The mapper must return the same sharding functor for all
       * instances of the fill. The runtime will verify this in debug mode
       * but not in release mode.
       */
      //------------------------------------------------------------------------
      virtual void select_sharding_functor(MapperContext            ctx,
                                 const Fill&                        fill,
                                 const SelectShardingFunctorInput&  input,
                                       SelectShardingFunctorOutput& output) = 0;
      //------------------------------------------------------------------------
    public: // Future Map Reductions
      /**
       * ----------------------------------------------------------------------
       *  Map Future Map Reduction
       * This mapper call is invoked to map the output futures of a request
       * to reduce a future map down to a single future value. The runtime
       * provides the mapping tag that was passed into the runtime at the
       * dispatch site. The mapper should return a set of memories for where
       * to place instances of the future as output. If there are multiple
       * copies the runtime will broadcast out the results in the order in
       * which they are specified. Note that this mapper call is not a pure
       * virtual function because we allow the output to be empty for
       * backwards compatibility. If the destination memories are empty
       * then the runtime will map one copy in the local system memory.
       *
       * In the case that the all-reduce is being performed using a reduction
       * operator with serdez functions, then the mapper can also specify an
       * upper bound on the amount of memory required for the findl output 
       * instance of the fully reduced future which will improve performance.
       * Not specifying an upper bound will not impact correctness.
       * ----------------------------------------------------------------------
       */
      struct FutureMapReductionInput {
        MappingTagID                            tag;
      };
      struct FutureMapReductionOutput {
        std::vector<Memory>                     destination_memories;
        size_t                                  serdez_upper_bound; // =SIZE_MAX
      };
      //------------------------------------------------------------------------
      virtual void map_future_map_reduction(MapperContext            ctx,
                                     const FutureMapReductionInput&  input,
                                           FutureMapReductionOutput& output) { }
      //------------------------------------------------------------------------

    public: // Single Task Context 
      /**
       * ----------------------------------------------------------------------
       *  Configure Context 
       * ----------------------------------------------------------------------
       * The configure_context mapping call is performed once for every 
       * non-leaf task before it starts running. It allows the mapper 
       * control over important aspects of the task's execution. First,
       * the mapper can control how far the task runs ahead before it
       * starts stalling due to resource constraints. The mapper can 
       * specify either a maximum number of outstanding sub operations
       * by specifying 'max_window_size' or if the task issues frame
       * operations (see 'complete_frame') it can set the maximum
       * number of outstanding frames with 'max_outstanding_frames.'
       * For the task-based run ahead measure, the mapper can also
       * apply a hysteresis factor by setting 'hysteresis_percentage'
       * to reduce jitter. The hysteresis factor specifies what percentage
       * of 'max_window_size' tasks have to finish executing before 
       * execution can begin again after a stall.
       *
       * The mapper can also control how many outstanding sub-tasks need
       * to be mapped before the mapping process is considered to be far
       * enough ahead that it can be halted for this context by setting
       * the 'min_tasks_to_schedule' parameter.
       *
       * The mapper can control the granularity of Legion meta-tasks
       * for this context with the 'meta_task_vector_width' parameter
       * which control how many meta-tasks get batched together for 
       * certain stages of the execution pipeline. This is useful to 
       * avoid the overheads of Realm tasks which often do not deal
       * with very small meta-tasks (e.g. those that take 20us or less).
       *
       * The 'max_templates_per_trace' parameter specifies the maximum
       * number of templates that should be kept around per captured 
       * physical trace before discarding them. The runtime will use a
       * least-recently used policy when discarding templates.
       *
       * The 'mutable_priority' parameter allows the mapper to specify
       * whether child operations launched in this context are permitted
       * to alter the priority of parent task. See the 'update_parent_priority'
       * field of the 'select_task_options' mapper call. If this is set 
       * to false then the child mappers cannot change the priority of
       * the parent task.
       */
      struct ContextConfigOutput {
        unsigned                                max_window_size; // = 1024
        unsigned                                hysteresis_percentage; // = 25
        unsigned                                max_outstanding_frames; // = 2
        unsigned                                min_tasks_to_schedule; // = 64
        unsigned                                min_frames_to_schedule; // = 0 
        unsigned                                meta_task_vector_width; // = 16
        unsigned                                max_templates_per_trace; // = 16
        bool                                    mutable_priority; // = false
      };
      //------------------------------------------------------------------------
      virtual void configure_context(MapperContext               ctx,
                                     const Task&                 task,
                                           ContextConfigOutput&  output) = 0;
      //------------------------------------------------------------------------

      /**
       * ----------------------------------------------------------------------
       *  Select Tunable Variable 
       * ----------------------------------------------------------------------
       * The select_tunable_value mapper call allows mappers to control
       * decisions about tunable values for a given task execution. The
       * mapper is told of the tunable ID and presented with the mapping
       * tag for the operation. It then must then allocate a buffer and 
       * put the result in the buffer. Alternatively, it can also tell the
       * runtime that it does not own the result by setting the take_ownership
       * flag to false indicating that the runtime should make its own copy
       * of the resulting buffer. If the resulting future expects the 
       * future to be packed, it is the responsibility of the mapper 
       * to pack it. The utility method 'pack_tunable' will allocate
       * the buffer and do any necessary packing for an arbitrary type.
       */
      struct SelectTunableInput {
        TunableID                               tunable_id;
        MappingTagID                            mapping_tag;
        std::vector<Future>                     futures;
        const void*                             args;
        size_t                                  size;
      };
      struct SelectTunableOutput {
        void*                                   value;
        size_t                                  size;
        bool                                    take_ownership; // = true 
      };
      //------------------------------------------------------------------------
      virtual void select_tunable_value(MapperContext               ctx,
                                        const Task&                 task,
                                        const SelectTunableInput&   input,
                                              SelectTunableOutput&  output) = 0;
      //------------------------------------------------------------------------
    public: // Mapping collections of operations 
      /**
       * ----------------------------------------------------------------------
       *  Select Sharding Functor 
       * ----------------------------------------------------------------------
       * This mapper call is invoked whenever the enclosing parent
       * task for the must epoch operation being launched has been 
       * control replicated and it's up to the mapper for this must epoch
       * operation to pick a sharding functor to determine which shard will 
       * own the point(s) of the must epoch operation . The mapper must return 
       * the same sharding functor for all instances of the must epoch 
       * operation. The runtime will verify this in debug mode
       * but not in release mode. For this mapper call the mapper must 
       * also choose whether to perform the map_must_epoch call as a collective
       * operation or not. If it chooses to perform it as a collective then we 
       * will do one map_must_epoch call on each shard with the constraints 
       * that apply to the points owned by the shard. The default is not to
       * perform the map must epoch call as a collective operation.
       */
      struct MustEpochShardingFunctorOutput :
              public SelectShardingFunctorOutput {
        bool                                    collective_map_must_epoch_call;
      };
      //------------------------------------------------------------------------
      virtual void select_sharding_functor(MapperContext            ctx,
                              const MustEpoch&                      epoch,
                              const SelectShardingFunctorInput&     input,
                                    MustEpochShardingFunctorOutput& output) = 0;
      //------------------------------------------------------------------------

      /**
       * ----------------------------------------------------------------------
       *  Map Must Epoch 
       * ----------------------------------------------------------------------
       * The map_must_epoch mapper call is invoked for mapping groups of
       * tasks which are required to execute concurrently, thereby allowing
       * them to optionally synchronize with each other. Each of the tasks in
       * the 'tasks' vector must be mapped with their resulting mapping being
       * specified in the corresponding location in the 'task_mapping' field.
       * The mapper is provided with the usual inputs for each task in the 
       * 'task_inputs' vector. As part of the mapping process, the mapper 
       * must abide by the mapping constraints specified in the 'constraints' 
       * field which says which logical regions in different tasks must be 
       * mapped to the same physical instance. The mapper is also given 
       * the mapping tag passed at the callsite in 'mapping_tag'.
       *
       * A special case of map_must_epoch is when it is called as a collective
       * mapping call for a must epoch launch performed inside of a control
       * replicated parent task. This behavior is controlled by the result
       * of select_sharding_functor for the must epoch operation (see above).
       * In this case map_must_epoch will only be given 'tasks' owned by its
       * shard and 'constraints' that apply to those 'tasks'. The mapper must
       * still pick 'task_processors' and these processor must be unique with
       * respect to any chosen for other 'tasks' by other mappers. The runime
       * will check this property in debug mode. For constraints, the mapper
       * may also pick optional 'constraint_mappings' for its constraints or
       * rely on another mapper to pick them (it's up to the mapper to 
       * determine which mapper instance picks thems). The mapper can then
       * specify a 'weight' for each constraint mapping. The runtime will 
       * do a collective reduction across all the 'constraint_mappings' taking
       * the mappings with the highest weights and the lowest shard ID when
       * the weights are the same.
       */
      struct MappingConstraint {
        std::vector<const Task*>                    constrained_tasks;
        std::vector<unsigned>                       requirement_indexes;
        // tasks.size() == requirement_indexes.size()
      };
      struct MapMustEpochInput {
        std::vector<const Task*>                    tasks;
        std::vector<MappingConstraint>              constraints;
        MappingTagID                                mapping_tag;
        // For collective map_must_epoch only
        std::vector<Processor>                      shard_mapping;
        ShardID                                     local_shard;
      };
      struct MapMustEpochOutput {
        std::vector<Processor>                      task_processors;
        std::vector<std::vector<PhysicalInstance> > constraint_mappings;
        // For collective map_must_epoch only
        std::vector<int>                            weights;
      };
      //------------------------------------------------------------------------
      virtual void map_must_epoch(MapperContext                 ctx,
                                  const MapMustEpochInput&      input,
                                        MapMustEpochOutput&     output) = 0;
      //------------------------------------------------------------------------

      struct MapDataflowGraphInput {
#if 0
        std::vector<const Task*>                nodes;
        std::vector<DataflowEdge>               edges;
        std::vector<Callsite>                   callsites;
#endif
      };
      struct MapDataflowGraphOutput {
          
      };
      //------------------------------------------------------------------------
      virtual void map_dataflow_graph(MapperContext                 ctx,
                                      const MapDataflowGraphInput&  input,
                                            MapDataflowGraphOutput& output) = 0;
      //------------------------------------------------------------------------

    public: // Memoizing physical analyses of operations
      /**
       * ----------------------------------------------------------------------
       *  Memoize Operation
       * ----------------------------------------------------------------------
       * The memoize_operation mapper call asks the mapper to decide if the
       * physical analysis of the operation should be memoized. Operations
       * that are not being logically traced cannot be memoized.
       *
       */
      struct MemoizeInput {
        TraceID trace_id;
      };
      struct MemoizeOutput {
        bool memoize;
      };
      //------------------------------------------------------------------------
      virtual void memoize_operation(MapperContext        ctx,
                                     const Mappable&      mappable,
                                     const MemoizeInput&  input,
                                           MemoizeOutput& output) = 0;
      //------------------------------------------------------------------------

    public: // Mapping control 
      /**
       * ----------------------------------------------------------------------
       *  Select Tasks to Map 
       * ----------------------------------------------------------------------
       * Legion gives the mapper control over when application tasks are
       * mapped, so application tasks can be kept available for stealing
       * or dynamically sent to another node. The select_tasks_to_map
       * mapper call presents the mapper for this processor with a list of
       * tasks that are ready to map in the 'ready_tasks' list. For any
       * of the tasks in this list, the mapper can either decide to map
       * the task by placing it in the 'map_tasks' set, or send it to 
       * another processor by placing it in the 'relocate_tasks' map 
       * along with the target processor for the task. Finally, the
       * mapper can also choose to leave the task on the ready queue
       * by doing nothing. If the mapper chooses not to do anything
       * for any of the tasks in the ready queue then it must give
       * the runtime a mapper event to use for deferring any future 
       * calls to select_tasks_to_map. No more calls will be made to
       * select_tasks_to_map until this mapper event is triggered by
       * the mapper in another mapper call or the state of the 
       * ready_queue changes (e.g. new tasks are added). Failure to
       * provide a mapper event will result in an error.
       */
      struct SelectMappingInput {
        std::list<const Task*>                  ready_tasks;
      };
      struct SelectMappingOutput {
        std::set<const Task*>                   map_tasks;
        std::map<const Task*,Processor>         relocate_tasks;
        MapperEvent                             deferral_event;
      };
      //------------------------------------------------------------------------
      virtual void select_tasks_to_map(MapperContext                ctx,
                                       const SelectMappingInput&    input,
                                             SelectMappingOutput&   output) = 0;
      //------------------------------------------------------------------------
    public: // Stealing
      /**
       * ----------------------------------------------------------------------
       *  Select Steal Targets
       * ----------------------------------------------------------------------
       * Control over stealing in Legion is explicitly given to the mappers.
       * The select_steal_targets mapper call is invoked whenever the 
       * select_tasks_to_map call is made for a mapper and asks the mapper
       * if it would like to attempt to steal from any other processors in
       * the machine. The mapper is provided with a list of 'blacklist'
       * processors which are disallowed because of previous stealing 
       * failures (the runtime automatically manages this blacklist and
       * remove processors when it receives notification that they have
       * additional work available for stealing). The mapper can put
       * any set of processors in the potential 'targets' and steal requests
       * will be sent. Note that any targets also contained in the blacklist
       * will be ignored.
       */
      struct SelectStealingInput {
        std::set<Processor>                     blacklist;
      };
      struct SelectStealingOutput {
        std::set<Processor>                     targets;
      };
      //------------------------------------------------------------------------
      virtual void select_steal_targets(MapperContext               ctx,
                                        const SelectStealingInput&  input,
                                              SelectStealingOutput& output) = 0;
      //------------------------------------------------------------------------

      /**
       * ----------------------------------------------------------------------
       *  Permit Steal Request 
       * ----------------------------------------------------------------------
       * Steal requests are also reported to mappers using the 
       * 'permit_steal_request' mapper call. This gives mappers the option
       * of deciding which tasks are stolen and which are kept on the 
       * local node. Mappers are told which processor originated the steal
       * request in the 'thief_proc' field along with a list of tasks which
       * are eligible for stealing in 'stealable_tasks' (note all these
       * tasks must have had 'spawn' set to true either in select_task_options
       * or slice_domain). The mapper can then specify the tasks that are
       * permitted to be stolen (if any) by place them in the stolen tasks
       * data structure.
       */
      struct StealRequestInput {
        Processor                               thief_proc;
        std::vector<const Task*>                stealable_tasks;
      };
      struct StealRequestOutput {
        std::set<const Task*>                   stolen_tasks;
      };
      //------------------------------------------------------------------------
      virtual void permit_steal_request(MapperContext               ctx,
                                        const StealRequestInput&    input,
                                              StealRequestOutput&   output) = 0;
      //------------------------------------------------------------------------
    public: // Handling
      /**
       * ----------------------------------------------------------------------
       *  Handle Message 
       * ----------------------------------------------------------------------
       * The handle_message call is invoked as the result of a message being
       * delivered from another processor. The 'sender' field indicates the
       * processor from which the message originated. The message is stored 
       * in a buffer pointed to by 'message' and contains 'size' bytes. The
       * mapper must make a copy of the buffer if it wants it to remain
       * persistent. The 'broadcast' field indicates whether this message
       * is the result of a broadcast or whether it is a single message
       * send directly to this mapper.
       */
      struct MapperMessage {
        Processor                               sender;
        unsigned                                kind;
        const void*                             message;
        size_t                                  size;
        bool                                    broadcast;
      };
      //------------------------------------------------------------------------
      virtual void handle_message(MapperContext                 ctx,
                                  const MapperMessage&          message) = 0;
      //------------------------------------------------------------------------

      /**
       * ----------------------------------------------------------------------
       *  Handle Task Result 
       * ----------------------------------------------------------------------
       * The handle_task_result call is made after the mapper has requested
       * an external computation be run by calling 'launch_mapper_task'. This
       * calls gives the 'mapper_event' that says which task result is being
       * returned. The result is passed in a buffer call 'result' of 
       * 'result_size' bytes. The mapper must make a copy of this buffer if
       * it wants the data to remain persistent.
       */
      struct MapperTaskResult {
        MapperEvent                             mapper_event;
        const void*                             result;
        size_t                                  result_size;
      };
      //------------------------------------------------------------------------
      virtual void handle_task_result(MapperContext           ctx,
                                      const MapperTaskResult&       result) = 0;
      //------------------------------------------------------------------------
    };

    /**
     * \class MapperRuntime
     * This class defines the set of calls that a mapper can perform as part
     * of its execution. All the calls must be given a MapperContext which 
     * comes from the enclosing mapper call context in which the runtime 
     * method is being invoked.
     */
    class MapperRuntime {
    protected:
      // These runtime objects will be created by Legion
      friend class Internal::Runtime;
      MapperRuntime(void);
      ~MapperRuntime(void);
    public:
      //------------------------------------------------------------------------
      // Methods for managing access to mapper state in the concurrent model
      // These calls are illegal in the serialized mapper model 
      //------------------------------------------------------------------------
      bool is_locked(MapperContext ctx) const;
      void lock_mapper(MapperContext ctx, bool read_only = false) const;
      void unlock_mapper(MapperContext ctx) const;
    public:
      //------------------------------------------------------------------------
      // Methods for managing the re-entrant state in the serialized model
      // These calls are illegal in the concurrent mapper model
      //------------------------------------------------------------------------
      bool is_reentrant(MapperContext ctx) const;
      void enable_reentrant(MapperContext ctx) const;
      void disable_reentrant(MapperContext ctx) const;
    public:
      //------------------------------------------------------------------------
      // Methods for updating mappable data 
      // The mapper is responsible for atomicity of these calls 
      // (usually through the choice of mapper synchronization model) 
      //------------------------------------------------------------------------
      void update_mappable_tag(MapperContext ctx, const Mappable &mappable, 
                               MappingTagID new_tag) const;
      // Runtime will make a copy of the data passed into this method
      void update_mappable_data(MapperContext ctx, const Mappable &mappable,
                                const void *mapper_data, 
                                size_t mapper_data_size) const;
    public:
      //------------------------------------------------------------------------
      // Methods for communicating with other mappers of the same kind
      //------------------------------------------------------------------------
      void send_message(MapperContext ctx, Processor target,const void *message,
                        size_t message_size, unsigned message_kind = 0) const;
      void broadcast(MapperContext ctx, const void *message, 
           size_t message_size, unsigned message_kind = 0, int radix = 4) const;
    public:
      //------------------------------------------------------------------------
      // Methods for packing and unpacking physical instances
      //------------------------------------------------------------------------
      void pack_physical_instance(MapperContext ctx, Serializer &rez,
                                  PhysicalInstance instance) const;
      void unpack_physical_instance(MapperContext ctx, Deserializer &derez,
                                    PhysicalInstance &instance) const;
    public:
      //------------------------------------------------------------------------
      // Methods for managing the execution of mapper tasks 
      //------------------------------------------------------------------------
      //MapperEvent launch_mapper_task(MapperContext ctx, 
      //                               Processor::TaskFuncID tid,
      //                               const UntypedBuffer &arg) const;

      //void defer_mapper_call(MapperContext ctx, 
      //                                 MapperEvent event) const;

      //MapperEvent merge_mapper_events(MapperContext ctx,
      //                              const std::set<MapperEvent> &events)const;
    public:
      //------------------------------------------------------------------------
      // Methods for managing mapper events 
      //------------------------------------------------------------------------
      MapperEvent create_mapper_event(MapperContext ctx) const;
      bool has_mapper_event_triggered(MapperContext ctx,
                                                MapperEvent event) const;
      void trigger_mapper_event(MapperContext ctx, 
                                          MapperEvent event) const;
      void wait_on_mapper_event(MapperContext ctx,
                                          MapperEvent event) const;
    public:
      //------------------------------------------------------------------------
      // Methods for managing constraint information
      //------------------------------------------------------------------------
      const ExecutionConstraintSet& find_execution_constraints(
                       MapperContext ctx, TaskID task_id, VariantID vid) const;
      const TaskLayoutConstraintSet& find_task_layout_constraints(
                       MapperContext ctx, TaskID task_id, VariantID vid) const;
      const LayoutConstraintSet& find_layout_constraints(
                               MapperContext ctx, LayoutConstraintID id) const;
      LayoutConstraintID register_layout(MapperContext ctx, 
                          const LayoutConstraintSet &layout_constraints,
                          FieldSpace handle = FieldSpace::NO_SPACE) const;
      void release_layout(MapperContext ctx, 
                                    LayoutConstraintID layout_id) const;
      bool do_constraints_conflict(MapperContext ctx,
                     LayoutConstraintID set1, LayoutConstraintID set2,
                     const LayoutConstraint **conflict_constraint = NULL) const;
      bool do_constraints_entail(MapperContext ctx,
                   LayoutConstraintID source, LayoutConstraintID target,
                   const LayoutConstraint **failed_constraint = NULL) const;
    public:
      //------------------------------------------------------------------------
      // Methods for manipulating variants 
      //------------------------------------------------------------------------
      void find_valid_variants(MapperContext ctx, TaskID task_id, 
                               std::vector<VariantID> &valid_variants,
                               Processor::Kind kind = Processor::NO_KIND) const;
      void find_generator_variants(MapperContext ctx, TaskID task_id,
                  std::vector<std::pair<TaskID,VariantID> > &generator_variants,
                  Processor::Kind kind = Processor::NO_KIND) const;
      VariantID find_or_create_variant(MapperContext ctx, TaskID task_id,
                             const ExecutionConstraintSet &execution_constrains,
                             const TaskLayoutConstraintSet &layout_constraints,
                             TaskID generator_tid, VariantID generator_vid, 
                             Processor generator_processor, bool &created) const;
      const char* find_task_variant_name(MapperContext ctx,
                                         TaskID task_id, VariantID vid) const;
      bool is_leaf_variant(MapperContext ctx, TaskID task_id,
                                      VariantID variant_id) const;
      bool is_inner_variant(MapperContext ctx, TaskID task_id,
                                      VariantID variant_id)const;
      bool is_idempotent_variant(MapperContext ctx, TaskID task_id,
                                      VariantID variant_id) const;
      bool is_replicable_variant(MapperContext ctx, TaskID task_id,
                                      VariantID variant_id) const;     
    public:
      //------------------------------------------------------------------------
      // Methods for registering variants 
      //------------------------------------------------------------------------
      template<typename T,
        T (*TASK_PTR)(const Task*, const std::vector<PhysicalRegion>&,
                      Context, Runtime*)>
      VariantID register_task_variant(MapperContext ctx,
                                      const TaskVariantRegistrar &registrar);
      template<typename T, typename UDT,
        T (*TASK_PTR)(const Task*, const std::vector<PhysicalRegion>&,
                      Context, Runtime*, const UDT&)>
      VariantID register_task_variant(MapperContext ctx,
                                      const TaskVariantRegistrar &registrar,
                                      const UDT &user_data);
      template<
        void (*TASK_PTR)(const Task*, const std::vector<PhysicalRegion>&,
                         Context, Runtime*)>
      VariantID register_task_variant(MapperContext ctx,
                                      const TaskVariantRegistrar &registrar);
      template<typename UDT,
        void (*TASK_PTR)(const Task*, const std::vector<PhysicalRegion>&,
                         Context, Runtime*, const UDT&)>
      VariantID register_task_variant(MapperContext ctx,
                                      const TaskVariantRegistrar &registrar,
                                      const UDT &user_data);
      VariantID register_task_variant(MapperContext ctx, 
                                      const TaskVariantRegistrar &registrar,
				      const CodeDescriptor &codedesc,
				      const void *user_data = NULL,
				      size_t user_len = 0,
                                      size_t return_type_size =
                                              LEGION_MAX_RETURN_SIZE,
                                      bool has_return_type = false);
    public:
      //------------------------------------------------------------------------
      // Methods for accelerating mapping decisions
      //------------------------------------------------------------------------
      // Filter variants based on the chosen instances
      void filter_variants(MapperContext ctx, const Task &task,
             const std::vector<std::vector<PhysicalInstance> > &chosen_intances,
                           std::vector<VariantID>              &variants);
      // Filter instances based on a chosen variant
      void filter_instances(MapperContext ctx, const Task &task,
                                      VariantID chosen_variant, 
                        std::vector<std::vector<PhysicalInstance> > &instances,
                               std::vector<std::set<FieldID> > &missing_fields);
      // Filter a specific set of instances for one region requirement
      void filter_instances(MapperContext ctx, const Task &task,
                                      unsigned index, VariantID chosen_variant,
                                      std::vector<PhysicalInstance> &instances,
                                      std::set<FieldID> &missing_fields);
    public:
      //------------------------------------------------------------------------
      // Methods for managing physical instances 
      //------------------------------------------------------------------------
      bool create_physical_instance(
                                   MapperContext ctx, Memory target_memory,
                                   const LayoutConstraintSet &constraints, 
                                   const std::vector<LogicalRegion> &regions,
                                   PhysicalInstance &result, bool acquire=true,
                                   GCPriority priority = 0,
                                   bool tight_region_bounds = false,
                                   size_t *footprint = NULL,
                                   const LayoutConstraint **unsat = NULL) const;
      bool create_physical_instance(
                                   MapperContext ctx, Memory target_memory,
                                   LayoutConstraintID layout_id,
                                   const std::vector<LogicalRegion> &regions,
                                   PhysicalInstance &result, bool acquire=true,
                                   GCPriority priority = 0,
                                   bool tight_region_bounds = false,
                                   size_t *footprint = NULL,
                                   const LayoutConstraint **unsat = NULL) const;
      bool find_or_create_physical_instance(
                                   MapperContext ctx, Memory target_memory,
                                   const LayoutConstraintSet &constraints, 
                                   const std::vector<LogicalRegion> &regions,
                                   PhysicalInstance &result, bool &created, 
                                   bool acquire = true,GCPriority priority = 0,
                                   bool tight_region_bounds = false,
                                   size_t *footprint = NULL,
                                   const LayoutConstraint **unsat = NULL) const;
      bool find_or_create_physical_instance(
                                   MapperContext ctx, Memory target_memory,
                                   LayoutConstraintID layout_id,
                                   const std::vector<LogicalRegion> &regions,
                                   PhysicalInstance &result, bool &created, 
                                   bool acquire = true,GCPriority priority = 0,
                                   bool tight_region_bounds = false,
                                   size_t *footprint = NULL,
                                   const LayoutConstraint **unsat = NULL) const;
      bool find_physical_instance(
                                   MapperContext ctx, Memory target_memory,
                                   const LayoutConstraintSet &constraints,
                                   const std::vector<LogicalRegion> &regions,
                                   PhysicalInstance &result, bool acquire =true,
                                   bool tight_region_bounds = false) const;
      bool find_physical_instance(
                                   MapperContext ctx, Memory target_memory,
                                   LayoutConstraintID layout_id,
                                   const std::vector<LogicalRegion> &regions,
                                   PhysicalInstance &result, bool acquire =true,
                                   bool tight_region_bounds = false) const;
      void find_physical_instances(
                                   MapperContext ctx, Memory target_memory,
                                   const LayoutConstraintSet &constraints,
                                   const std::vector<LogicalRegion> &regions,
                                   std::vector<PhysicalInstance> &results, 
                                   bool acquire = false,
                                   bool tight_region_bounds = false) const;
      void find_physical_instances(
                                   MapperContext ctx, Memory target_memory,
                                   LayoutConstraintID layout_id,
                                   const std::vector<LogicalRegion> &regions,
                                   std::vector<PhysicalInstance> &results, 
                                   bool acquire = false,
                                   bool tight_region_bounds = false) const;
      void set_garbage_collection_priority(MapperContext ctx, 
                const PhysicalInstance &instance, GCPriority priority) const;
      // These methods will atomically check to make sure that these instances
      // are still valid and then add an implicit reference to them to ensure
      // that they aren't collected before this mapping call completes. They
      // don't need to be called as part of mapping an instance, but they are
      // highly recommended to ensure correctness. Acquiring instances and
      // then not using them is also acceptable as the runtime will implicitly
      // release the references after the call. Instances can also be released
      // as might be expected if a mapper opts to attempt to map a different
      // instance, but this is an optional performance improvement.
      bool acquire_instance(MapperContext ctx, 
                                      const PhysicalInstance &instance) const;
      bool acquire_instances(MapperContext ctx,
                             const std::vector<PhysicalInstance> &insts) const;
      bool acquire_and_filter_instances(MapperContext ctx,
                                std::vector<PhysicalInstance> &instances,
                                bool filter_acquired_instance = false) const;
      bool acquire_instances(MapperContext ctx,
            const std::vector<std::vector<PhysicalInstance> > &instances) const;
      bool acquire_and_filter_instances(MapperContext ctx,
                  std::vector<std::vector<PhysicalInstance> > &instances,
                  bool filter_acquired_instances = false) const;
      void release_instance(MapperContext ctx, 
                                        const PhysicalInstance &instance) const;
      void release_instances(MapperContext ctx,
                          const std::vector<PhysicalInstance> &instances) const;
      void release_instances(MapperContext ctx,
            const std::vector<std::vector<PhysicalInstance> > &instances) const;
    public:
      // Futures can also be acquired to ensure that they are available in
      // particular memories prior to running a task.
      bool acquire_future(MapperContext ctx, const Future &f, Memory mem) const;
    public:
      //------------------------------------------------------------------------
      // Methods for creating index spaces which mappers need to do
      // in order to be able to properly slice index space operations
      //------------------------------------------------------------------------
      IndexSpace create_index_space(MapperContext ctx, 
                                    const Domain &bounds,
                                    TypeTag type_tag = 0,
                                    const char *provenance = NULL) const;
      // Template version
      template<int DIM, typename COORD_T>
      IndexSpaceT<DIM,COORD_T> create_index_space(MapperContext ctx,
                                           Rect<DIM,COORD_T> bounds,
                                           const char *provenance = NULL) const;

      IndexSpace create_index_space(MapperContext ctx, 
                                    const std::vector<DomainPoint> &points,
                                    const char *provenance = NULL) const;
      // Template version
      template<int DIM, typename COORD_T>
      IndexSpaceT<DIM,COORD_T> create_index_space(MapperContext ctx,
                    const std::vector<Point<DIM,COORD_T> > &points,
                    const char *provenance = NULL) const;

      IndexSpace create_index_space(MapperContext ctx,
                                    const std::vector<Domain> &rects,
                                    const char *provenance = NULL) const;
      // Template version
      template<int DIM, typename COORD_T>
      IndexSpaceT<DIM,COORD_T> create_index_space(MapperContext ctx,
                      const std::vector<Rect<DIM,COORD_T> > &rects,
                      const char *provenance = NULL) const;

      IndexSpace union_index_spaces(MapperContext ctx,
                      const std::vector<IndexSpace> &sources,
                      const char *provenance = NULL) const;
      // Template version
      template<int DIM, typename COORD_T>
      IndexSpaceT<DIM,COORD_T> union_index_spaces(MapperContext ctx,
                const std::vector<IndexSpaceT<DIM,COORD_T> > &sources,
                const char *provenance = NULL) const;

      IndexSpace intersect_index_spaces(MapperContext ctx,
                      const std::vector<IndexSpace> &sources,
                      const char *provenance = NULL) const;
      // Template version
      template<int DIM, typename COORD_T>
      IndexSpaceT<DIM,COORD_T> intersect_index_spaces(MapperContext ctx,
                const std::vector<IndexSpaceT<DIM,COORD_T> > &sources,
                const char *provenance = NULL) const;

      IndexSpace subtract_index_spaces(MapperContext ctx,
                        IndexSpace left, IndexSpace right,
                        const char *provenance = NULL) const;
      // Template version
      template<int DIM, typename COORD_T>
      IndexSpaceT<DIM,COORD_T> subtract_index_spaces(MapperContext ctx,
          IndexSpaceT<DIM,COORD_T> left, IndexSpaceT<DIM,COORD_T> right,
          const char *provenance = NULL) const;
    public:
      //------------------------------------------------------------------------
      // Convenience methods for introspecting index spaces
      //------------------------------------------------------------------------
      bool is_index_space_empty(MapperContext ctx, IndexSpace handle) const;
      template<int DIM, typename COORD_T>
      bool is_index_space_empty(MapperContext ctx,
                                IndexSpaceT<DIM,COORD_T> handle) const;

      bool index_spaces_overlap(MapperContext ctx,
                                IndexSpace one, IndexSpace two) const;
      template<int DIM, typename COORD_T>
      bool index_spaces_overlap(MapperContext ctx, IndexSpaceT<DIM,COORD_T> one,
                                IndexSpaceT<DIM,COORD_T> two) const;

      bool index_space_dominates(MapperContext ctx,
                                 IndexSpace test, IndexSpace dominator) const;
      template<int DIM, typename COORD_T>
      bool index_space_dominates(MapperContext ctx, 
                                 IndexSpaceT<DIM,COORD_T> test,
                                 IndexSpaceT<DIM,COORD_T> dominator) const;
    public:
      //------------------------------------------------------------------------
      // Methods for introspecting index space trees 
      // For documentation see methods of the same name in Runtime
      //------------------------------------------------------------------------
      bool has_index_partition(MapperContext ctx,
                               IndexSpace parent, Color c) const;

      IndexPartition get_index_partition(MapperContext ctx,
                                         IndexSpace parent, Color color) const;

      IndexSpace get_index_subspace(MapperContext ctx, 
                                    IndexPartition p, Color c) const;
      IndexSpace get_index_subspace(MapperContext ctx, 
                   IndexPartition p, const DomainPoint &color) const;

      bool has_multiple_domains(MapperContext ctx, 
                                          IndexSpace handle) const;

      Domain get_index_space_domain(MapperContext ctx, 
                                              IndexSpace handle) const;

      void get_index_space_domains(MapperContext ctx, 
                         IndexSpace handle, std::vector<Domain> &domains) const;

      Domain get_index_partition_color_space(MapperContext ctx,
                                                       IndexPartition p) const;
      
      IndexSpace get_index_partition_color_space_name(MapperContext ctx,
                                                      IndexPartition p) const;

      void get_index_space_partition_colors(MapperContext ctx, 
                                  IndexSpace sp, std::set<Color> &colors) const;

      bool is_index_partition_disjoint(MapperContext ctx, 
                                       IndexPartition p) const;

      bool is_index_partition_complete(MapperContext ctx,
                                       IndexPartition p) const;

      Color get_index_space_color(MapperContext ctx, 
                                            IndexSpace handle) const;

      DomainPoint get_index_space_color_point(MapperContext ctx,
                                              IndexSpace handle) const;

      Color get_index_partition_color(MapperContext ctx, 
                                                IndexPartition handle) const;

      IndexSpace get_parent_index_space(MapperContext ctx,
                                                  IndexPartition handle) const;

      bool has_parent_index_partition(MapperContext ctx, 
                                                IndexSpace handle) const;
      
      IndexPartition get_parent_index_partition(MapperContext ctx,
                                                IndexSpace handle) const;

      unsigned get_index_space_depth(MapperContext ctx,
                                     IndexSpace handle) const;

      unsigned get_index_partition_depth(MapperContext ctx,
                                         IndexPartition handle) const;
    public:
      //------------------------------------------------------------------------
      // Methods for introspecting field spaces 
      // For documentation see methods of the same name in Runtime
      //------------------------------------------------------------------------
      size_t get_field_size(MapperContext ctx, 
                            FieldSpace handle, FieldID fid) const;

      void get_field_space_fields(MapperContext ctx, 
           FieldSpace handle, std::vector<FieldID> &fields) const;
      void get_field_space_fields(MapperContext ctx,
           FieldSpace handle, std::set<FieldID> &fields) const;
    public:
      //------------------------------------------------------------------------
      // Methods for introspecting logical region trees
      //------------------------------------------------------------------------
      LogicalPartition get_logical_partition(MapperContext ctx, 
                             LogicalRegion parent, IndexPartition handle) const;

      LogicalPartition get_logical_partition_by_color(MapperContext ctx, 
                                       LogicalRegion parent, Color color) const;
      LogicalPartition get_logical_partition_by_color(MapperContext ctx, 
                          LogicalRegion parent, const DomainPoint &color) const;

      LogicalPartition get_logical_partition_by_tree(
                    MapperContext ctx, IndexPartition handle, 
                    FieldSpace fspace, RegionTreeID tid) const;

      LogicalRegion get_logical_subregion(MapperContext ctx, 
                              LogicalPartition parent, IndexSpace handle) const;

      LogicalRegion get_logical_subregion_by_color(MapperContext ctx,
                                    LogicalPartition parent, Color color) const;
      LogicalRegion get_logical_subregion_by_color(MapperContext ctx,
                       LogicalPartition parent, const DomainPoint &color) const;
      
      LogicalRegion get_logical_subregion_by_tree(MapperContext ctx,
                  IndexSpace handle, FieldSpace fspace, RegionTreeID tid) const;

      Color get_logical_region_color(MapperContext ctx, 
                                     LogicalRegion handle) const;

      DomainPoint get_logical_region_color_point(MapperContext ctx,
                                                 LogicalRegion handle) const;

      Color get_logical_partition_color(MapperContext ctx,
                                                LogicalPartition handle) const;

      LogicalRegion get_parent_logical_region(MapperContext ctx,
                                                LogicalPartition handle) const;

      bool has_parent_logical_partition(MapperContext ctx, 
                                                  LogicalRegion handle) const;

      LogicalPartition get_parent_logical_partition(MapperContext ctx,
                                                    LogicalRegion handle) const;
    public:
      //------------------------------------------------------------------------
      // Methods for getting access to semantic info
      //------------------------------------------------------------------------
      bool retrieve_semantic_information(MapperContext ctx, 
          TaskID task_id, SemanticTag tag, const void *&result, size_t &size, 
          bool can_fail = false, bool wait_until_ready = false);

      bool retrieve_semantic_information(MapperContext ctx, 
          IndexSpace handle, SemanticTag tag, const void *&result, size_t &size,
          bool can_fail = false, bool wait_until_ready = false);

      bool retrieve_semantic_information(MapperContext ctx,
          IndexPartition handle, SemanticTag tag, const void *&result, 
          size_t &size, bool can_fail = false, bool wait_until_ready = false);

      bool retrieve_semantic_information(MapperContext ctx,
          FieldSpace handle, SemanticTag tag, const void *&result, size_t &size,
          bool can_fail = false, bool wait_until_ready = false);

      bool retrieve_semantic_information(MapperContext ctx, 
          FieldSpace handle, FieldID fid, SemanticTag tag, const void *&result, 
          size_t &size, bool can_fail = false, bool wait_until_ready = false);

      bool retrieve_semantic_information(MapperContext ctx,
          LogicalRegion handle, SemanticTag tag, const void *&result, 
          size_t &size, bool can_fail = false, bool wait_until_ready = false);

      bool retrieve_semantic_information(MapperContext ctx,
          LogicalPartition handle, SemanticTag tag, const void *&result, 
          size_t &size, bool can_fail = false, bool wait_until_ready = false);

      void retrieve_name(MapperContext ctx, TaskID task_id,
                                   const char *&result);

      void retrieve_name(MapperContext ctx, IndexSpace handle,
                                   const char *&result);

      void retrieve_name(MapperContext ctx, IndexPartition handle,
                                   const char *&result);
      
      void retrieve_name(MapperContext ctx, FieldSpace handle,
                                   const char *&result);

      void retrieve_name(MapperContext ctx, FieldSpace handle, 
                                   FieldID fid, const char *&result);

      void retrieve_name(MapperContext ctx, LogicalRegion handle,
                                   const char *&result);

      void retrieve_name(MapperContext ctx, LogicalPartition handle,
                                   const char *&result);
    public:
      //------------------------------------------------------------------------
      // Methods for MPI interoperability
      //------------------------------------------------------------------------
      bool is_MPI_interop_configured(MapperContext ctx);
      const std::map<int/*rank*/,AddressSpace>& 
                                    find_forward_MPI_mapping(MapperContext ctx); 

      const std::map<AddressSpace,int/*rank*/>&
                                    find_reverse_MPI_mapping(MapperContext ctx);
      int find_local_MPI_rank(MapperContext ctx);
    public:
      //------------------------------------------------------------------------
      // Support for packing tunable values
      //------------------------------------------------------------------------
      template<typename T>
      void pack_tunable(const T &result, Mapper::SelectTunableOutput &output)
      {
#if !defined(__GNUC__) || (__GNUC__ >= 5)
        static_assert(std::is_trivially_copyable<T>::value,
                      "tunable type must be trivially copyable");
#endif  // !defined(__GNUC__) || (__GNUC__ >= 5)
        void *output_result = malloc(sizeof(T));
        memcpy(output_result, &result, sizeof(T));
        output.value = output_result;
        output.size = sizeof(T);
      }
    }; 

    /**
     * \class AutoLock
     * This class allows mappers to use their own fast reservation
     * synchronization primitives instead of relying on the mapper
     * synchronization model to perform all the synchronization.
     * (This is still an experimental feature and subject to change)
     */
    class AutoLock : public Internal::AutoLock {
    public:
      AutoLock(MapperContext ctx, LocalLock &r, int mode = 0, bool excl = true);
      AutoLock(AutoLock &&rhs) = delete;
      AutoLock(const AutoLock &rhs) = delete;
      ~AutoLock(void) { };
    public:
      AutoLock& operator=(AutoLock &&rhs) = delete;
      AutoLock& operator=(const AutoLock &rhs) = delete;
    public:
      void reacquire(void);
    protected:
      const MapperContext ctx;
    };

  }; // namespace Mapping
}; // namespace Legion

#include "legion/legion_mapping.inl"

#endif // __LEGION_MAPPING_H__

