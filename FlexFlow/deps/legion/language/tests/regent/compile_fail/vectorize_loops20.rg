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
-- vectorize_loops20.rg:51: vectorization failed: loop body has an expression of an inadmissible type
--     e.g = 0.5 * e.p.f
--                  ^

import "regent"

local c = regentlib.c

fspace fs1
{
  f : double,
}

fspace fs2(r : region(ispace(int2d), fs1))
{
  p : int2d(fs1, r),
  g : double,
}

task init(r1 : region(ispace(int2d), fs1), r2 : region(fs2(r1)), size : int)
where reads writes(r1, r2)
do
  for e in r1 do e.f = c.drand48() end
  for e in r2 do
    e.p = unsafe_cast(int2d(fs1, r1), int2d { __raw(e).value / size,
                                              __raw(e).value % size })
    e.g = 0
  end
end

task stencil(r1 : region(ispace(int2d), fs1), r2 : region(fs2(r1)))
where reads(r1.f, r2.p), writes(r2.g)
do
  __demand(__vectorize)
  for e in r2 do
    e.g = 0.5 * e.p.f
  end
end
