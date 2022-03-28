/*
 *
 * Copyright 2015 gRPC authors.
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
 *
 */
#include "client.h"
#include <iostream>
#include <memory>
#include <string>

#include <grpcpp/grpcpp.h>

#ifdef BAZEL_BUILD
#include "examples/protos/blockstorage.grpc.pb.h"
#else
#include "blockstorage.grpc.pb.h"
#endif

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using blockstorage::BlockStorage;
using blockstorage::ReadReply;
using blockstorage::ReadRequest;
using blockstorage::WriteReply;
using blockstorage::WriteRequest;

// class BlockStorageClient {
//  public:
//   BlockStorageClient(std::shared_ptr<Channel> channel)
//       : stub_(BlockStorage::NewStub(channel)) {}

//  private:
//   std::unique_ptr<BlockStorage::Stub> stub_;
// };

int main(int argc, char** argv) {
  // Instantiate the client. It requires a channel, out of which the actual RPCs
  // are created. This channel models a connection to an endpoint specified by
  // the argument "--target=" which is the only expected argument.
  // We indicate that the channel isn't authenticated (use of
  // InsecureChannelCredentials()).
  std::string target_str;
  std::string arg_str("--target");
  if (argc > 1) {
    std::string arg_val = argv[1];
    size_t start_pos = arg_val.find(arg_str);
    if (start_pos != std::string::npos) {
      start_pos += arg_str.size();
      if (arg_val[start_pos] == '=') {
        target_str = arg_val.substr(start_pos + 1);
      } else {
        std::cout << "The only correct argument syntax is --target="
                  << std::endl;
        return 0;
      }
    } else {
      std::cout << "The only acceptable argument is --target=" << std::endl;
      return 0;
    }
  } else {
    target_str = "localhost:50053"; // LoadBalancer
  }

  BlockStorageClient blockstorageClient(grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials()));
  int address = 0; //TODO: remove
  std::string reply = blockstorageClient.Read(address);
  std::cout << "BlockStorage received: " << reply << std::endl;

  return 0;
}