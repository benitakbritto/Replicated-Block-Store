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

void testReadWrite(BlockStorageClient* blockstorageClient, int address){
  
  string buffer(4096, 'a');
  Status writeStatus = blockstorageClient->Write(address, buffer);
  cout<<writeStatus.error_code();
  if (writeStatus.error_code() != grpc::StatusCode::OK) {
    cout << "Write test failed" << endl;
    return;
  }

  ReadRequest request;
  ReadReply reply;
  request.set_addr(address);

  Status readStatus = blockstorageClient->Read(request, &reply, address);
  
  if (readStatus.error_code() != grpc::StatusCode::OK) {
    cout << "Read test failed" << endl;
    return;
  }

  if (reply.buffer().compare(buffer) == 0) {
    cout << "TEST PASSED: Aligned read data is same as write buffer " << endl;
  }
  
  else {
    cout << "TEST FAILED: Aligned read data is not the same as write buffer " << endl;
  }
  // TODO: checksum
  // cout << "Read : " << reply.buffer() << endl;
  // cout << "Written: " << buffer << endl; 
}

void *testReadWriteConcurrent(void* bsClientPtr){
  BlockStorageClient* blockstorageClient((BlockStorageClient*)bsClientPtr);
  testReadWrite(blockstorageClient, 0);
  return NULL;
}

void TestConcurrentAccess(BlockStorageClient* bsClientPtr){
  pthread_t test_t1, test_t2;
  pthread_create(&test_t1, NULL, testReadWriteConcurrent, bsClientPtr);
  pthread_create(&test_t1, NULL, testReadWriteConcurrent, bsClientPtr);
  pthread_join(test_t1, NULL);
  pthread_join(test_t2, NULL);
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
    target_str = "52.151.53.152:50051"; // LoadBalancer
  }

  BlockStorageClient blockstorageClient(grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials()));
  
  // Test: Read Write Aligned
  int address = 0;
  testReadWrite(&blockstorageClient, address);

  // Test: Read Write Unaligned
  address = 4097;
  testReadWrite(&blockstorageClient, address);
  return 0;
}