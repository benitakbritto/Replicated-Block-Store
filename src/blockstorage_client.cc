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

int main(int argc, char** argv) {
  string target_str = "52.151.53.152:50051"; // LoadBalancer
  BlockStorageClient blockstorageClient(grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials()));
  
  // int address = 0;
  // blockstorageClient.Read(&blockstorageClient, address);
  return 0;
}