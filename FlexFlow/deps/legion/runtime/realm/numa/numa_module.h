/* Copyright 2023 Stanford University, NVIDIA Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef REALM_NUMA_MODULE_H
#define REALM_NUMA_MODULE_H

#include "realm/module.h"

namespace Realm {

  class MemoryImpl;

  namespace Numa {

    class NumaModuleConfig : public ModuleConfig {
      friend class NumaModule;
    protected:
      NumaModuleConfig(void);

      bool discover_resource(void);

    public:
      virtual void configure_from_cmdline(std::vector<std::string>& cmdline);

    protected:
      size_t cfg_numa_mem_size = 0;
      ssize_t cfg_numa_nocpu_mem_size = -1;
      int cfg_num_numa_cpus = 0;
      bool cfg_pin_memory = false;
      size_t cfg_stack_size = 2 << 20;

      // resources
      bool resource_discovered = false;
      bool res_numa_available = false;
    };

    // our interface to the rest of the runtime
    class REALM_INTERNAL_API_EXTERNAL_LINKAGE NumaModule : public Module {
    protected:
      NumaModule(void);
      
    public:
      virtual ~NumaModule(void);

      static ModuleConfig *create_module_config(RuntimeImpl *runtime);

      static Module *create_module(RuntimeImpl *runtime);

      // do any general initialization - this is called after all configuration is
      //  complete
      virtual void initialize(RuntimeImpl *runtime);

      // create any memories provided by this module (default == do nothing)
      //  (each new MemoryImpl should use a Memory from RuntimeImpl::next_local_memory_id)
      virtual void create_memories(RuntimeImpl *runtime);

      // create any processors provided by the module (default == do nothing)
      //  (each new ProcessorImpl should use a Processor from
      //   RuntimeImpl::next_local_processor_id)
      virtual void create_processors(RuntimeImpl *runtime);

      // create any DMA channels provided by the module (default == do nothing)
      virtual void create_dma_channels(RuntimeImpl *runtime);

      // create any code translators provided by the module (default == do nothing)
      virtual void create_code_translators(RuntimeImpl *runtime);

      // clean up any common resources created by the module - this will be called
      //  after all memories/processors/etc. have been shut down and destroyed
      virtual void cleanup(void);

    public:
      NumaModuleConfig *config;

      // "global" variables live here too
      std::map<int, void *> numa_mem_bases;
      std::map<int, size_t> numa_mem_sizes;
      std::map<int, int> numa_cpu_counts;
      std::map<int, MemoryImpl *> memories;
    };

  }; // namespace Numa

}; // namespace Realm

#endif
