
/* Copyright 2023 NVIDIA Corporation
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

#ifndef BOOTSTRAP_UTIL_H
#define BOOTSTRAP_UTIL_H

#include <stdio.h>

#define BOOTSTRAP_ERROR_PRINT(...)                                   \
  do {                                                               \
    fprintf(stderr, "%s:%s:%d: ", __FILE__, __FUNCTION__, __LINE__); \
    fprintf(stderr, __VA_ARGS__);                                    \
  } while (0)

#define BOOTSTRAP_NE_ERROR_JMP(status, expected, err, label, ...)                 \
  do {                                                                            \
    if (status != expected) {                                                     \
      fprintf(stderr, "%s:%d: non-zero status: %d ", __FILE__, __LINE__, status); \
      fprintf(stderr, __VA_ARGS__);                                               \
      status = err;                                                               \
      goto label;                                                                 \
    }                                                                             \
  } while (0)

#define BOOTSTRAP_NZ_ERROR_JMP(status, err, label, ...)                           \
  do {                                                                            \
    if (status != 0) {                                                            \
      fprintf(stderr, "%s:%d: non-zero status: %d ", __FILE__, __LINE__, status); \
      fprintf(stderr, __VA_ARGS__);                                               \
      status = err;                                                               \
      goto label;                                                                 \
    }                                                                             \
  } while (0)

#define BOOTSTRAP_NULL_ERROR_JMP(var, status, err, label, ...)   \
  do {                                                           \
    if (var == NULL) {                                           \
      fprintf(stderr, "%s:%d: NULL value ", __FILE__, __LINE__); \
      fprintf(stderr, __VA_ARGS__);                              \
      status = err;                                              \
      goto label;                                                \
    }                                                            \
  } while (0)

#endif
