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
-- vectorize_loops19.rg:43: vectorization failed: found a loop-carried dependence
--       t = e.p2.v
--            ^

import "regent"

fspace fs2
{
  v : float,
}

fspace fs1(r : region(fs2))
{
  p1 : ptr(fs2, r),
  p2 : ptr(fs2, r),
}

task f(r2 : region(fs2), r : region(fs1(r2)))
where
  reads(r2.v, r.p1, r.p2),
  writes(r2.v)
do
  __demand(__vectorize)
  for e in r do
    var t : float
    for idx = 0, 10 do
      e.p1.v += t
      t = e.p2.v
    end
  end
end
