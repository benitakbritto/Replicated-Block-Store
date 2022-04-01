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

string generateStr(){
  string buffer = "";
  for (int i = 0; i < 4096; i++)
  {
    buffer += "a";
  }
  return buffer;
}

int main(int argc, char** argv) {
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
  
  // Test: Write
  int address = 0;
  string buffer = generateStr();
  auto status = blockstorageClient.Write(address, buffer);
  cout << "BlockStorage received errorcode: " << status.error_code() << std::endl;

  // Test: Read(0) - Aligned read
  // int address = 1;
  // string content = "";
  // ReadRequest request;
  // ReadReply reply;
  // request.set_addr(address);
  // auto status = blockstorageClient.Read(request, &reply, address);
  // cout << "BlockStorage received errorcode: " << status.error_code() << endl;


  return 0;
}