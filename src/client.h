#ifndef CLIENT_H
#define CLIENT_H

#include <iostream>
#include <memory>
#include <string>

#include <grpcpp/grpcpp.h>

#ifdef BAZEL_BUILD
#include "examples/protos/blockstorage.grpc.pb.h"
#else
#include "blockstorage.grpc.pb.h"
#endif

/******************************************************************************
 * MACROS
 *****************************************************************************/


/******************************************************************************
 * NAMESPACES
 *****************************************************************************/
using namespace std;
using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using blockstorage::BlockStorage;
using blockstorage::ReadReply;
using blockstorage::ReadRequest;
using blockstorage::WriteReply;
using blockstorage::WriteRequest;

/******************************************************************************
 * GLOBALS
 *****************************************************************************/

class BlockStorageClient  
{

private:
    unique_ptr<BlockStorage::Stub> stub_;
    
public:
    BlockStorageClient(std::shared_ptr<Channel> channel)
      : stub_(BlockStorage::NewStub(channel)) {}
    
    void Connect(string target);
    Status Write(int address, string buf);
    Status Read(int address);
};

#endif