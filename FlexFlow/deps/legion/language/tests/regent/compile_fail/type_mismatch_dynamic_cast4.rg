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
-- type_mismatch_dynamic_cast4.rg:28: incompatible partitions for dynamic_cast: partition(disjoint, $s, $cs) and partition(aliased, $r, ispace(int1d))
--   var p_disjoint = dynamic_cast(partition(disjoint, s, cs), p_aliased)
--                               ^

import "regent"

task main()
  var r = region(ispace(int1d, 4), int)
  var s = region(ispace(int1d, 4), int1d(int, r))
  var q = partition(equal, s, ispace(int1d, 4))
  var p_aliased = image(r, q, s)
  var cs = p_aliased.colors
  var p_disjoint = dynamic_cast(partition(disjoint, s, cs), p_aliased)
end
