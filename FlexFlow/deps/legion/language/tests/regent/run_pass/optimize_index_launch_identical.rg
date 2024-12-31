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
-- [["-fflow", "0"], ["-findex-launch", "0"]]

import "regent"

-- RDIR deduplicates same-valued AST nodes, and this causes issues
-- down the road for index launch optimization if you pass in the same
-- region twice. Probably not worth fixing, but this test serves as a
-- reminder that the conflict exists.

task f(r : region(int), s : region(int))
where reads(r, s) do
end

task main()
  var r = region(ispace(ptr, 10), int)
  fill(r, 0)
  __demand(__index_launch)
  for i = 0, 4 do
    rescape
      local re = rexpr r end
      remit rquote
        f(re, re)
      end
    end
  end
end
regentlib.start(main)
