-- Copyright 2023 Stanford University, NVIDIA Corporation
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

-- runs-with:
-- [["-ll:cpu", "2"]]

import "regent"
import "bishop"

local c = bishoplib.c

mapper

$procs = processors[isa=x86]

task#ta task#tb {
  target : $procs[1];
}

end

function get_proc()
  return rexpr
    c.legion_runtime_get_executing_processor(__runtime(), __context())
  end
end

fspace fs
{
  x : int,
  y : int,
}

task tc(idx : int, r : region(fs))
where reads(r.x) do
end

task tb(idx : int, r : region(fs))
where reads(r.x), reads writes(r.y) do
  var proc = [get_proc()]
  var procs = c.bishop_all_processors()
  tc(idx, r)
  regentlib.assert(procs.list[idx].id == proc.id, "test failed in tb")
end

task ta(idx : int, r : region(fs))
where reads(r.x), reads writes(r.y) do
  tb(idx, r)
end

task toplevel()
  var r = region(ispace(ptr, 10), fs)
  fill(r.{x, y}, 0)
  ta(1, r)
end

regentlib.start(toplevel, bishoplib.make_entry())
