#include "client.h"

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
        return "RPC failed";
    }
}

string BlockStorageClient::Write(int address,string buf){
    WriteRequest request;
    request.set_addr(address);
    request.set_buffer(buf);
    WriteReply reply;
    ClientContext context;
    Status status = stub_->Write(&context, request, &reply);

    if (status.ok()) {
        return "Write successful";
    } else {
        cout << status.error_code() << ": " << status.error_message() << endl;
        return "RPC failed";
    }
}

    // auto Client::Connect(string target_str){
    //     return Client(
    //         grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials()));
    // }