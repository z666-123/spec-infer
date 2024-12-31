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

-- runs-with:
-- [["-fflow", "0"]]

-- FIXME: RDIR emits the same variable declaration twice for cp0[0]

import "regent"

task main()
  var r = region(ispace(int1d, 10), int)
  var p = partition(equal, r, ispace(int1d, 1))
  var q = partition(equal, r, ispace(int1d, 1))
  var cp = cross_product(p, q)

  var cp0 = cp[0]

  var x = 0
  var k = 0
  for i = 0, 10 do
    cp0[0][i] = i
    x = cp0[0][i]
    x = cp0[0][i]
    x = cp0[k][i]
  end

  var j = 0
  for e in r do
    regentlib.assert(@e == j, "test failed")
    j += 1
  end
end

regentlib.start(main)
