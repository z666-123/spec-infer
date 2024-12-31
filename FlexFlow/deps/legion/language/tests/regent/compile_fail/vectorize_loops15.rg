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
-- vectorize_loops15.rg:40: vectorization failed: found a loop-carried dependence
--     e.p1.v = e.p2.v
--      ^

import "regent"

fspace fs2
{
  v : float,
}

fspace fs1(r : region(fs2), s : region(fs2))
{
  p1 : ptr(fs2, r),
  p2 : ptr(fs2, r, s),
}

task f(r : region(fs2), s : region(fs2), t : region(fs1(r, s)))
where
  reads(r.v, s.v, t.p1, t.p2),
  writes(r.v, s.v)
do
  __demand(__vectorize)
  for e in t do
    e.p1.v = e.p2.v
  end
end

-- FIXME: This test was supposed to check this case. Put this back once the vectorizer gets fixed.
-- vectorize_loops15.rg:40: vectorization failed: loop body has aliasing update of path region(fs2()).v
--     e.p1.v = e.p2.v
--     ^
