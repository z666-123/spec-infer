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
-- local_task_idempotent1.rg:29: task is not a valid idempotent task
--   f()
--    ^

import "regent"

__demand(__local)
__forbid(__idempotent)
task f()
end

__demand(__idempotent)
task g()
  f()
end
g:compile()
