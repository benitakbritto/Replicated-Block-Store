#include<errno.h>
#include "client.h"

#define DEBUG                       1                     
#define dbgprintf(...)              if (DEBUG) { printf(__VA_ARGS__); } 

Status BlockStorageClient::Read(int address) {
    ReadRequest request;
    request.set_addr(address);
    ReadReply reply;
    ClientContext context;
    return stub_->Read(&context, request, &reply);
}

Status BlockStorageClient::Write(int address,string buf){
    WriteRequest request;
    request.set_addr(address);
    request.set_buffer(buf);
    WriteReply reply;
    ClientContext context;
    // dbgprintf("reached BS write: trying to write buf = %s \n", buf.c_str());
    return stub_->Write(&context, request, &reply);
}

    // auto Client::Connect(string target_str){
    //     return Client(
    //         grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials()));
    // }