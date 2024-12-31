/* Copyright 2023 Stanford University
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

// NOP, but helps with IDEs
#include "simple_blas.h"

template <typename T>
inline BlasArrayRef<T>::BlasArrayRef(LogicalRegion _region,
                                     LogicalPartition _logical_partition,
				     FieldID _fid /*= DEFAULT_FID*/)
  : region(_region)
  , logical_partition(_logical_partition)
  , fid(_fid)
{}

template <typename T>
inline BlasArrayRef<T>::BlasArrayRef(const BlasArrayRef<T>& copy_from)
  : region(copy_from.region)
  , logical_partition(copy_from.logical_partition)
  , fid(copy_from.fid)
{}

template <typename T>
inline BlasArrayRef<T>::~BlasArrayRef(void)
{}

template <typename T>
inline /*static*/ BlasArrayRef<T> BlasArrayRef<T>::create(Runtime *runtime,
                                                          Context ctx,
                                                          IndexSpace is,
                                                          IndexSpace cs,
                                                          FieldID fid /*= DEFAULT_FID*/)
{
  FieldSpace fs = runtime->create_field_space(ctx);
  {
    FieldAllocator fa = runtime->create_field_allocator(ctx, fs);
    fa.allocate_field(sizeof(T), fid);
  }

  LogicalRegion region = runtime->create_logical_region(ctx, is, fs);
  IndexPartition ipartition = runtime->create_equal_partition(ctx, is, cs);
  LogicalPartition lpartition = runtime->get_logical_partition(ctx, region, ipartition);

  return BlasArrayRef<T>(region, lpartition, fid);
}

template <typename T>
inline void BlasArrayRef<T>::destroy(Runtime *runtime, Context ctx)
{
  // do not destroy index space - it's not owned by us
  FieldSpace fs = region.get_field_space();
  runtime->destroy_logical_region(ctx, region);
  runtime->destroy_field_space(ctx, fs);
}

template <typename T>
inline void BlasArrayRef<T>::fill(Runtime *runtime, Context ctx, T fill_val)
{
  runtime->fill_field(ctx, region, region, fid, &fill_val, sizeof(T));
}

template <typename T>
template <typename LT>
inline void BlasArrayRef<T>::add_requirement(LT& launcher, PrivilegeMode mode,
                                             CoherenceProperty prop /*= EXCLUSIVE*/,
                                             bool is_index_launcher /*= false*/) const
{
  if (is_index_launcher) {
    launcher.add_region_requirement(RegionRequirement(logical_partition, 0, mode, prop, region)
                                    .add_field(fid));
  } else {
    launcher.add_region_requirement(RegionRequirement(region, mode, prop, region)
                                    .add_field(fid));
  }
}

template <typename T>
inline void axpy(Runtime *runtime, Context ctx,
                 T alpha, const BlasArrayRef<T>& x, BlasArrayRef<T> y,
                 IndexSpace cs)
{
  ArgumentMap arg_map;
  IndexLauncher launcher(blas_impl_s.axpy_task_id, cs,
			TaskArgument(&alpha, sizeof(T)), arg_map);
//   TaskLauncher launcher(blas_impl_s.axpy_task_id
//    			TaskArgument(&alpha, sizeof(T)));
  x.add_requirement(launcher, READ_ONLY, EXCLUSIVE, true);
  y.add_requirement(launcher, READ_WRITE, EXCLUSIVE, true);
  runtime->execute_index_space(ctx, launcher);
}

template <typename T>
inline T dot(Runtime *runtime, Context ctx,
	     const BlasArrayRef<T>& x, BlasArrayRef<T> y)
{
  TaskLauncher launcher(blas_impl_s.dot_task_id,
			TaskArgument(0, 0));
  x.add_requirement(launcher, READ_ONLY);
  y.add_requirement(launcher, READ_ONLY);
  Future f = runtime->execute_task(ctx, launcher);
  return f.get_result<T>();
}

template <typename T>
inline void BlasTaskImplementations<T>::preregister_tasks(void)
{
  {
    axpy_task_id = Runtime::generate_static_task_id();
    TaskVariantRegistrar tvr(axpy_task_id);
#ifdef REALM_USE_OPENMP
    tvr.add_constraint(ProcessorConstraint(Processor::OMP_PROC));
#else
    tvr.add_constraint(ProcessorConstraint(Processor::LOC_PROC));
#endif
    Runtime::preregister_task_variant<BlasTaskImplementations<T>::axpy_task_cpu>(tvr, "axpy (cpu)");
  }

  {
    dot_task_id = Runtime::generate_static_task_id();
    TaskVariantRegistrar tvr(dot_task_id);
#ifdef REALM_USE_OPENMP
    tvr.add_constraint(ProcessorConstraint(Processor::OMP_PROC));
#else
    tvr.add_constraint(ProcessorConstraint(Processor::LOC_PROC));
#endif
    Runtime::preregister_task_variant<T, &BlasTaskImplementations<T>::dot_task_cpu>(tvr, "dot (cpu)");
  }
}


