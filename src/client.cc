#include<errno.h>
#include "client.h"

#define DEBUG                       1                     
#define dbgprintf(...)              if (DEBUG) { printf(__VA_ARGS__); } 

/*
*   @return: file content
*/
Status BlockStorageClient::Read(const ReadRequest request,
                  ReadReply* reply, int address) {
    dbgprintf("reached server read\n");
    ClientContext context;
    dbgprintf("calling read on stub\n");
    return stub_->Read(&context, request, reply);
}

Status BlockStorageClient::Write(int address,string buf){
    WriteRequest request;
    request.set_addr(address);
    request.set_buffer(buf);
    WriteReply reply;
    ClientContext context;
    return stub_->Write(&context, request, &reply);
}

Status BlockStorageClient::Ping() {
    ClientContext context;
    PingRequest request;
    PingReply reply;
    return stub_->Ping(&context, request, &reply);
}