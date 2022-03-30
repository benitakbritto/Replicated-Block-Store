#include<errno.h>
#include "client.h"

#define DEBUG                       1                     
#define dbgprintf(...)              if (DEBUG) { printf(__VA_ARGS__); } 

string BlockStorageClient::Read(int address) {
    ReadRequest request;
    request.set_addr(address);
    ReadReply reply;
    ClientContext context;
    Status status = stub_->Read(&context, request, &reply);

    if (status.ok()) {
        return reply.buffer().c_str();
    } else {
        cout << status.error_code() << ": " << status.error_message()<< endl;
        reply.set_error(errno);
        return "Error";
    }
}

int BlockStorageClient::Write(int address,string buf){
    WriteRequest request;
    request.set_addr(address);
    request.set_buffer(buf);
    WriteReply reply;
    ClientContext context;
    dbgprintf("reached BS write: trying to write buf = %s \n", buf);
    Status status = stub_->Write(&context, request, &reply);

    return status.error_code();
}

    // auto Client::Connect(string target_str){
    //     return Client(
    //         grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials()));
    // }