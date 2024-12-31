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

// GASNet-1 network module implementation for Realm

// NOTE: this should work with GASNet-EX's backward-compatibility interfaces,
//  but it is not recommended - there are known performance regressions compared
//  to using an actual GASNet-1 implementation

#ifndef GASNET1_MODULE_H
#define GASNET1_MODULE_H

#include "realm/network.h"

namespace Realm {

  class GASNet1Module : public NetworkModule {
  protected:
    GASNet1Module(void);

  public:
    // all subclasses should define this (static) method - its responsibilities
    // are:
    // 1) determine if the network module should even be loaded
    // 2) fix the command line if the spawning system hijacked it
    static NetworkModule *create_network_module(RuntimeImpl *runtime,
						int *argc, const char ***argv);

    // Enumerates all the peers that the current node could potentially share memory with
    virtual void get_shared_peers(NodeSet &shared_peers);

    // actual parsing of the command line should wait until here if at all
    //  possible
    virtual void parse_command_line(RuntimeImpl *runtime,
				    std::vector<std::string>& cmdline);

    // "attaches" to the network, if that is meaningful - attempts to
    //  bind/register/(pick your network-specific verb) the requested memory
    //  segments with the network
    virtual void attach(RuntimeImpl *runtime,
			std::vector<NetworkSegment *>& segments);

    virtual void create_memories(RuntimeImpl *runtime);

    // detaches from the network
    virtual void detach(RuntimeImpl *runtime,
			std::vector<NetworkSegment *>& segments);

    // collective communication within this network
    virtual void barrier(void);
    virtual void broadcast(NodeID root,
			   const void *val_in, void *val_out, size_t bytes);
    virtual void gather(NodeID root,
			const void *val_in, void *vals_out, size_t bytes);
    virtual void allgatherv(const char *val_in, size_t bytes, std::vector<char> &vals_out,
                            std::vector<size_t> &lengths);

    virtual size_t sample_messages_received_count(void);
    virtual bool check_for_quiescence(size_t sampled_receive_count);

    // used to create a remote proxy for a memory
    virtual MemoryImpl *create_remote_memory(Memory m, size_t size, Memory::Kind kind,
					     const ByteArray& rdma_info);
    virtual IBMemory *create_remote_ib_memory(Memory m, size_t size, Memory::Kind kind,
					      const ByteArray& rdma_info);

    virtual ActiveMessageImpl *create_active_message_impl(NodeID target,
							  unsigned short msgid,
							  size_t header_size,
							  size_t max_payload_size,
							  const void *src_payload_addr,
							  size_t src_payload_lines,
							  size_t src_payload_line_stride,
							  void *storage_base,
							  size_t storage_size);

    virtual ActiveMessageImpl *create_active_message_impl(NodeID target,
							  unsigned short msgid,
							  size_t header_size,
							  size_t max_payload_size,
							  const void *src_payload_addr,
							  size_t src_payload_lines,
							  size_t src_payload_line_stride,
							  const RemoteAddress& dest_payload_addr,
							  void *storage_base,
							  size_t storage_size);

    virtual ActiveMessageImpl *create_active_message_impl(const NodeSet& targets,
							  unsigned short msgid,
							  size_t header_size,
							  size_t max_payload_size,
							  const void *src_payload_addr,
							  size_t src_payload_lines,
							  size_t src_payload_line_stride,
							  void *storage_base,
							  size_t storage_size);

    virtual size_t recommended_max_payload(NodeID target,
					   bool with_congestion,
					   size_t header_size);
    virtual size_t recommended_max_payload(const NodeSet& targets,
					   bool with_congestion,
					   size_t header_size);
    virtual size_t recommended_max_payload(NodeID target,
					   const RemoteAddress& dest_payload_addr,
					   bool with_congestion,
					   size_t header_size);
    virtual size_t recommended_max_payload(NodeID target,
					   const void *data, size_t bytes_per_line,
					   size_t lines, size_t line_stride,
					   bool with_congestion,
					   size_t header_size);
    virtual size_t recommended_max_payload(const NodeSet& targets,
					   const void *data, size_t bytes_per_line,
					   size_t lines, size_t line_stride,
					   bool with_congestion,
					   size_t header_size);
    virtual size_t recommended_max_payload(NodeID target,
					   const void *data, size_t bytes_per_line,
					   size_t lines, size_t line_stride,
					   const RemoteAddress& dest_payload_addr,
					   bool with_congestion,
					   size_t header_size);

  protected:
    bool active_msg_worker_bgwork;
    int active_msg_worker_threads;
    size_t gasnet_mem_size, amsg_stack_size;
  };

}; // namespace Realm

#endif

