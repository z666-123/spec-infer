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
-- annotations_for_num_parallel.rg:24: __demand(__parallel) is no longer supported on loops, please use __demand(__index_launch)
--   for i = 0, 10 do end
--     ^

import "regent"

task f()
  __demand(__parallel)
  for i = 0, 10 do end
end
f:compile()
