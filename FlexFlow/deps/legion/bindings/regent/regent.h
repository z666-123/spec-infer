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

#ifndef __REGENT_H__
#define __REGENT_H__

#include "legion.h"

#ifdef REALM_USE_CUDART_HIJACK
#define REGENT_USE_HIJACK 1
#else
#define REGENT_USE_HIJACK 0
#endif

#ifdef __cplusplus
extern "C" {
#endif

void register_array_reduction_plus_float(
    legion_reduction_op_id_t redop_id, unsigned array_size, bool permit_duplicates);
void register_array_reduction_plus_double(
    legion_reduction_op_id_t redop_id, unsigned array_size, bool permit_duplicates);
void register_array_reduction_plus_int32(
    legion_reduction_op_id_t redop_id, unsigned array_size, bool permit_duplicates);
void register_array_reduction_plus_int64(
    legion_reduction_op_id_t redop_id, unsigned array_size, bool permit_duplicates);
void register_array_reduction_plus_uint32(
    legion_reduction_op_id_t redop_id, unsigned array_size, bool permit_duplicates);
void register_array_reduction_plus_uint64(
    legion_reduction_op_id_t redop_id, unsigned array_size, bool permit_duplicates);

void register_array_reduction_minus_float(
    legion_reduction_op_id_t redop_id, unsigned array_size, bool permit_duplicates);
void register_array_reduction_minus_double(
    legion_reduction_op_id_t redop_id, unsigned array_size, bool permit_duplicates);
void register_array_reduction_minus_int32(
    legion_reduction_op_id_t redop_id, unsigned array_size, bool permit_duplicates);
void register_array_reduction_minus_int64(
    legion_reduction_op_id_t redop_id, unsigned array_size, bool permit_duplicates);
void register_array_reduction_minus_uint32(
    legion_reduction_op_id_t redop_id, unsigned array_size, bool permit_duplicates);
void register_array_reduction_minus_uint64(
    legion_reduction_op_id_t redop_id, unsigned array_size, bool permit_duplicates);

void register_array_reduction_times_float(
    legion_reduction_op_id_t redop_id, unsigned array_size, bool permit_duplicates);
void register_array_reduction_times_double(
    legion_reduction_op_id_t redop_id, unsigned array_size, bool permit_duplicates);
void register_array_reduction_times_int32(
    legion_reduction_op_id_t redop_id, unsigned array_size, bool permit_duplicates);
void register_array_reduction_times_int64(
    legion_reduction_op_id_t redop_id, unsigned array_size, bool permit_duplicates);
void register_array_reduction_times_uint32(
    legion_reduction_op_id_t redop_id, unsigned array_size, bool permit_duplicates);
void register_array_reduction_times_uint64(
    legion_reduction_op_id_t redop_id, unsigned array_size, bool permit_duplicates);

void register_array_reduction_divide_float(
    legion_reduction_op_id_t redop_id, unsigned array_size, bool permit_duplicates);
void register_array_reduction_divide_double(
    legion_reduction_op_id_t redop_id, unsigned array_size, bool permit_duplicates);
void register_array_reduction_divide_int32(
    legion_reduction_op_id_t redop_id, unsigned array_size, bool permit_duplicates);
void register_array_reduction_divide_int64(
    legion_reduction_op_id_t redop_id, unsigned array_size, bool permit_duplicates);
void register_array_reduction_divide_uint32(
    legion_reduction_op_id_t redop_id, unsigned array_size, bool permit_duplicates);
void register_array_reduction_divide_uint64(
    legion_reduction_op_id_t redop_id, unsigned array_size, bool permit_duplicates);

void register_array_reduction_max_float(
    legion_reduction_op_id_t redop_id, unsigned array_size, bool permit_duplicates);
void register_array_reduction_max_double(
    legion_reduction_op_id_t redop_id, unsigned array_size, bool permit_duplicates);
void register_array_reduction_max_int32(
    legion_reduction_op_id_t redop_id, unsigned array_size, bool permit_duplicates);
void register_array_reduction_max_int64(
    legion_reduction_op_id_t redop_id, unsigned array_size, bool permit_duplicates);
void register_array_reduction_max_uint32(
    legion_reduction_op_id_t redop_id, unsigned array_size, bool permit_duplicates);
void register_array_reduction_max_uint64(
    legion_reduction_op_id_t redop_id, unsigned array_size, bool permit_duplicates);

void register_array_reduction_min_float(
    legion_reduction_op_id_t redop_id, unsigned array_size, bool permit_duplicates);
void register_array_reduction_min_double(
    legion_reduction_op_id_t redop_id, unsigned array_size, bool permit_duplicates);
void register_array_reduction_min_int32(
    legion_reduction_op_id_t redop_id, unsigned array_size, bool permit_duplicates);
void register_array_reduction_min_int64(
    legion_reduction_op_id_t redop_id, unsigned array_size, bool permit_duplicates);
void register_array_reduction_min_uint32(
    legion_reduction_op_id_t redop_id, unsigned array_size, bool permit_duplicates);
void register_array_reduction_min_uint64(
    legion_reduction_op_id_t redop_id, unsigned array_size, bool permit_duplicates);

#define DECLARE_C_REDUCTION(NAME)                                     \
  void NAME_float(legion_accessor_array_1d_t accessor,                \
                  legion_ptr_t ptr, float value);                     \
  void NAME_float_point_1d(legion_accessor_array_1d_t accessor,       \
                           legion_point_1d_t, float value);           \
  void NAME_float_point_2d(legion_accessor_array_2d_t accessor,       \
                           legion_point_2d_t, float value);           \
  void NAME_float_point_3d(legion_accessor_array_3d_t accessor,       \
                           legion_point_3d_t, float value);           \
  void NAME_double(legion_accessor_array_1d_t accessor,               \
                   legion_ptr_t ptr, double value);                   \
  void NAME_double_point_1d(legion_accessor_array_1d_t accessor,      \
                            legion_point_1d_t, double value);         \
  void NAME_double_point_2d(legion_accessor_array_2d_t accessor,      \
                           legion_point_2d_t, double value);          \
  void NAME_double_point_3d(legion_accessor_array_3d_t accessor,      \
                           legion_point_3d_t, double value);          \
  void NAME_int32(legion_accessor_array_1d_t accessor,                \
                  legion_ptr_t ptr, int value);                       \
  void NAME_int32_point_1d(legion_accessor_array_1d_t accessor,       \
                            legion_point_1d_t, int value);            \
  void NAME_int32_point_2d(legion_accessor_array_2d_t accessor,       \
                           legion_point_2d_t, int value);             \
  void NAME_int32_point_3d(legion_accessor_array_3d_t accessor,       \
                           legion_point_3d_t, int value);             \
  void NAME_int64(legion_accessor_array_1d_t accessor,                \
                  legion_ptr_t ptr, long long int value);             \
  void NAME_int64_point_1d(legion_accessor_array_1d_t accessor,       \
                            legion_point_1d_t, long long int value);  \
  void NAME_int64_point_2d(legion_accessor_array_2d_t accessor,       \
                           legion_point_2d_t, long long int value);   \
  void NAME_int64_point_3d(legion_accessor_array_3d_t accessor,       \
                           legion_point_3d_t, long long int value);   \
  void NAME_uint32(legion_accessor_array_1d_t accessor,               \
                   legion_ptr_t ptr, unsigned value);                 \
  void NAME_uint32_point_1d(legion_accessor_array_1d_t accessor,      \
                            legion_point_1d_t, unsigned value);       \
  void NAME_uint32_point_2d(legion_accessor_array_2d_t accessor,      \
                            legion_point_2d_t, unsigned value);       \
  void NAME_uint32_point_3d(legion_accessor_array_3d_t accessor,      \
                            legion_point_3d_t, unsigned value);       \
  void NAME_uint64(legion_accessor_array_1d_t accessor,               \
                   legion_ptr_t ptr, unsigned long long value);       \
  void NAME_uint64_point_1d(legion_accessor_array_1d_t accessor,      \
                            legion_point_1d_t, unsigned long long value); \
  void NAME_uint64_point_2d(legion_accessor_array_2d_t accessor,      \
                            legion_point_2d_t, unsigned long long value); \
  void NAME_uint64_point_3d(legion_accessor_array_3d_t accessor,      \
                            legion_point_3d_t, unsigned long long value);

DECLARE_C_REDUCTION(reduce_plus)
DECLARE_C_REDUCTION(reduce_minus)  
DECLARE_C_REDUCTION(reduce_times)
DECLARE_C_REDUCTION(reduce_divide)
DECLARE_C_REDUCTION(reduce_max)
DECLARE_C_REDUCTION(reduce_min)
DECLARE_C_REDUCTION(safe_reduce_plus)
DECLARE_C_REDUCTION(safe_reduce_minus)
DECLARE_C_REDUCTION(safe_reduce_times)
DECLARE_C_REDUCTION(safe_reduce_divide)
DECLARE_C_REDUCTION(safe_reduce_max)
DECLARE_C_REDUCTION(safe_reduce_min)

#undef DECLARE_C_REDUCTION

void regent_register_kernel_id(int64_t kernel_id);

#ifdef __cplusplus
}
#endif

#endif // __REGENT_H__
