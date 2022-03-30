#include <iostream>
#include <memory>
#include <string>
#include <grpcpp/grpcpp.h>
#include "blockstorage.grpc.pb.h"
#include "client.h"

#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/health_check_service_interface.h>

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using blockstorage::BlockStorage;
using blockstorage::ReadReply;
using blockstorage::ReadRequest;
using blockstorage::WriteReply;
using blockstorage::WriteRequest;

using namespace std;

#define PRIMARY_IP "0.0.0.0:50051"
#define BACKUP_IP "20.109.180.121:50051"
#define PRIMARY "primary"
#define BACKUP "backup"

#define DEBUG                       1                     
#define dbgprintf(...)              if (DEBUG) { printf(__VA_ARGS__); } 

class LoadBalancer final : public BlockStorage::Service {

    private:
        map<string, BlockStorageClient*> bs_clients;
        map<string, string> live_servers;
        int idx=0;

    public:
        LoadBalancer(){
            RegisterLiveServers();
            initLB();
        }

        void RegisterLiveServers(){
            live_servers[PRIMARY] = PRIMARY_IP;
            live_servers[BACKUP] = BACKUP_IP;
        }

        void initLB(){
            dbgprintf("Live servers size = %ld\n", live_servers.size());
            for (auto itr = live_servers.begin(); itr != live_servers.end(); itr++)
            {   // iterate over live servers and establish a connection with each
                string target_str = itr->second;
                string key = itr->first;
                dbgprintf("Pushing to bs_clients target ip = %s: %s\n", key.c_str(), target_str.c_str());
                bs_clients.insert(make_pair(key,
                    new BlockStorageClient (grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials()))));
            }
        }

        map<string, string> GetLiveServers(){
            return live_servers;
        }

        string getServerToRouteTo(){
            idx=1-idx;
            if(idx==0) return PRIMARY;
            return BACKUP;
        }

        Status Read(ServerContext* context, const ReadRequest* request,
                  ReadReply* reply) override {
            string key = getServerToRouteTo();
            dbgprintf("Routing read to %s\n", key.c_str());
            return bs_clients[key]->Read(request->addr());
        }

        Status Write(ServerContext* context, const WriteRequest* request,
                  WriteReply* reply) override {
            dbgprintf("reached LB write \n");
            return bs_clients[PRIMARY]->Write(request->addr(), request->buffer());
        }

};

void RunServer(int port) {

  std::string server_address("0.0.0.0:" + std::to_string(port));
  LoadBalancer service;

  grpc::EnableDefaultHealthCheckService(true);
  grpc::reflection::InitProtoReflectionServerBuilderPlugin();
  ServerBuilder builder;
  // Listen on the given address without any authentication mechanism.
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  // Register "service" as the instance through which we'll communicate with
  // clients. In this case it corresponds to an *synchronous* service.
  builder.RegisterService(&service);
  // Finally assemble the server.
  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cout << "Server listening on " << server_address << std::endl;

  // Wait for the server to shutdown. Note that some other thread must be
  // responsible for shutting down the server for this call to ever return.
  server->Wait();
}

int main(){
    RunServer(50053);
}