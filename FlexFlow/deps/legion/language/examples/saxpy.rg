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

import "regent"

-- This code has not been optimized and is not high performance.

local fabs = regentlib.fabs(double)

__demand(__cuda)
task saxpy(x : region(ispace(int1d), float), y : region(ispace(int1d), float), a : float)
where
  reads(x, y), writes(y)
do
  __demand(__vectorize)
  for i in x do
    y[i] += a*x[i]
  end
end

__demand(__local)
task test(n : int, np : int)
  var is = ispace(int1d, n)
  var x = region(is, float)
  var y = region(is, float)

  var cs = ispace(int1d, np)
  var px = partition(equal, x, cs)
  var py = partition(equal, y, cs)

  fill(x, 1.0)
  fill(y, 0.0)

  for c in cs do
    saxpy(px[c], py[c], 0.5)
  end

  for i in is do
    regentlib.assert(fabs(y[i] - 0.5) < 0.00001, "test failed")
  end
end

task main()
  test(20, 4)
end

if os.getenv('SAVEOBJ') == '1' then
  local root_dir = arg[0]:match(".*/") or "./"
  local out_dir = (os.getenv('OBJNAME') and os.getenv('OBJNAME'):match('.*/')) or root_dir
  local link_flags = terralib.newlist({"-L" .. out_dir})

  if os.getenv('STANDALONE') == '1' then
    os.execute('cp ' .. os.getenv('LG_RT_DIR') .. '/../bindings/regent/' ..
        regentlib.binding_library .. ' ' .. out_dir)
  end

  local exe = os.getenv('OBJNAME') or "saxpy"
  regentlib.saveobj(main, exe, "executable", nil, link_flags)
else
  regentlib.start(main)
end
