#include "../src/client.h"
#include <functional>
#include <iostream>
#include <future>

BlockStorageClient *blockstorageClient;
int N_workers=2;

void testReadWrite(int address, char c){
  
  string buffer(4096, c);
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
  return;
}

void testReadWriteConcurrent(int offset) {
  future<void> workers[N_workers];
  for (int i = 0; i < N_workers; i++) {
    char c = 'a'+i;
    int address = 4096*i+offset;
    workers[i] = async(testReadWrite, address, c); 
  }
  for(int i = 0; i < N_workers; i++) {
    workers[i].get();
  }
}

void testReadWriteAligned(){
  int offset = 0;
  testReadWriteConcurrent(offset);
}

void testReadWriteUnaligned(){
  int offset = 1; 
  testReadWriteConcurrent(offset);
}

int main() {
    blockstorageClient = new BlockStorageClient(grpc::CreateChannel("0.0.0.0:50051", grpc::InsecureChannelCredentials()));
    testReadWriteAligned();
    testReadWriteUnaligned();
}