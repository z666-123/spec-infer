-- Copyright 2023 Stanford University
--
-- Licensed under the Apache License, Version 2.0 (the "License");
-- you may not use this file except in compliance with the License.
-- You may obtain a copy of the License at
--
--     http://www.apache.org/licenses/LICENSE-2.0
--
-- Unless required by applicable law or agreed to in writing, software
-- distributed under the License is distributed on an "AS IS" BASIS,
-- WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
-- See the License for the specific language governing permissions and
-- limitations under the License.

-- fails-with:
-- type_mismatch_partition_by_image9.rg:37: type mismatch in argument 3: expected {rect2d} -> rect2d but got {int2d} -> int2d
--   var p_image = image(r_interior, p_equal, calculate_halo_size)
--                     ^

import "regent"

local c = regentlib.c

task calculate_halo_size(bounds : int2d) : int2d
  return bounds
end

task toplevel()
  var r = region(ispace(int2d, {10, 10}), int)
  var coloring = c.legion_domain_coloring_create()
  var bounds = r.ispace.bounds
  c.legion_domain_coloring_color_domain(coloring, 0, rect2d { {2, 2}, {7, 7} })
  var p_interior = partition(disjoint, r, coloring)
  c.legion_domain_coloring_destroy(coloring)
  var r_interior = p_interior[0]
  var p_equal = partition(equal, r_interior, ispace(int2d, {2, 2}))
  var p_image = image(r_interior, p_equal, calculate_halo_size)
end

regentlib.start(toplevel)
