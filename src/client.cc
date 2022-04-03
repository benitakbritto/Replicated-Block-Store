#include<errno.h>
#include "client.h"

#define DEBUG                       1                     
#define dbgprintf(...)              if (DEBUG) { printf(__VA_ARGS__); } 

Status BlockStorageClient::Read(const ReadRequest request,
                  ReadReply* reply, int address) 
{
    dbgprintf("[INFO] Read: Entering function\n");
    ClientContext context;
    dbgprintf("[INFO] Read: Invoking RPC\n");
    Status status = stub_->Read(&context, request, reply);
    dbgprintf("[INFO] Read: Exiting function\n");
    return status;
}

Status BlockStorageClient::Write(int address,string buf)
{
    dbgprintf("[INFO] Write: Entering function\n");
    WriteRequest request;
    request.set_addr(address);
    request.set_buffer(buf);
    WriteReply reply;
    ClientContext context;
    dbgprintf("[INFO] Write: Invoking RPC\n");
    Status status = stub_->Write(&context, request, &reply);
    dbgprintf("[INFO] Write: Entering function\n");
    return status;
}