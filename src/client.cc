#include<errno.h>
#include "client.h"
#include <unistd.h>

Status BlockStorageClient::Read(const ReadRequest request,
                  ReadReply* reply, int address) 
{
    // dbgprintf("[INFO] Read: Invoking RPC\n");
    int attempt = 0;
    Status status;

    do {
        reply.clear();
        ClientContext context;
        status = stub_->Read(&context, request, reply);    
        attempt++;
    } while(attempt < 3 && status.error_code() != grpc::StatusCode::OK)
    
    // dbgprintf("[INFO] Read: Exiting function\n");
    return status;
}

Status BlockStorageClient::Write(int address,string buf)
{
    // dbgprintf("[INFO] Write: Entering function\n");
    WriteRequest request;
    request.set_addr(address);
    request.set_buffer(buf);
    Status status;
    int attempt = 0;
    int backoff = 2;

    do {
        sleep(attempt * backoff);
        WriteReply reply;
        ClientContext context;
        status = stub_->Write(&context, request, &reply);
        attempt++;
    } while(attempt < 3 && status.error_code() != grpc::StatusCode::OK)

    return status;
}

Status BlockStorageClient::Ping() {
    ClientContext context;
    PingRequest request;
    PingReply reply;
    return stub_->Ping(&context, request, &reply);
}