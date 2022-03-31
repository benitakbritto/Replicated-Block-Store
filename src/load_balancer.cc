#include <iostream>
#include <memory>
#include <string>
#include <grpcpp/grpcpp.h>
#include "blockstorage.grpc.pb.h"
#include "lb.grpc.pb.h"
#include "client.h"

#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/health_check_service_interface.h>

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using grpc::ServerReaderWriter;

using namespace blockstorage;
using namespace std;

#define PRIMARY_IP "0.0.0.0:50051"
#define BACKUP_IP "20.109.180.121:50051"
#define PRIMARY "PRIMARY"
#define BACKUP "BACKUP"

#define DEBUG                       1                     
#define dbgprintf(...)              if (DEBUG) { printf(__VA_ARGS__); } 

map<string, string> live_servers;

class BlockStorageService final : public BlockStorage::Service {

    private:
        // for round-robin
        int idx=0;

        map<string, BlockStorageClient*> bs_clients;
        map<string, string>* nodes;

    public:
        BlockStorageService(map<string, string> &servers){
            // RegisterLiveServers();
            nodes = &servers;
            // initLB();
        }

        // void RegisterLiveServers(){
        //     (*nodes)[PRIMARY] = PRIMARY_IP;
        //     (*nodes)[BACKUP] = BACKUP_IP;
        // }

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

        // map<string, string> GetLiveServers(){
        //     return live_servers;
        // }

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

class LBNodeCommService final: public LBNodeComm::Service {
    private:
        map<string, string>* nodes;

        void register_identity(string identity, string ip) {
            (*nodes)[identity] = ip;
        }

        string get_peer_ip(string identity) {
            if (identity.compare(PRIMARY) == 0) {
                return (*nodes)[BACKUP];
            } else {
                return (*nodes)[PRIMARY];
            }
        }

        void erase_identity(string identity) {
            nodes->erase(identity);
        }

        void print_identity() {
            cout << PRIMARY << ":" << get_peer_ip(PRIMARY) << endl;
            cout << BACKUP << ":" << get_peer_ip(BACKUP) << endl;
        }

    public:
        LBNodeCommService(map<string, string> &servers) {
            nodes = &servers;
        }

        Status SendHeartBeat(ServerContext* context, ServerReaderWriter<HeartBeatReply, HeartBeatRequest>* stream) override {
            HeartBeatRequest request;
            HeartBeatReply reply;

            string identity;
            // TODO: create client on the fly

            while(1) {
                if(!stream->Read(&request)) {
                    break;
                }
                cout << "[INFO]: recv heartbeat from IP:[" << request.ip() <<  "]" << endl;

                identity = Identity_Name(request.identity());

                register_identity(identity, request.ip());

                string peer_ip = get_peer_ip(identity);

                if (!peer_ip.empty()) {
                    reply.set_peer_ip(peer_ip);
                }

                if(!stream->Write(reply)) {
                    break;
                }
                cout << "[INFO]: sent heartbeat reply" << endl;
            }

            cout << "[ERROR]: stream broke" << endl;

            erase_identity(identity);

            return Status::OK;
        }
};

void* RunServerForClient(void* arg) {
  int port = 50051;
  std::string server_address("0.0.0.0:" + std::to_string(port));
  BlockStorageService service(live_servers);

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
  std::cout << "Server for Client listening on " << server_address << std::endl;

  // Wait for the server to shutdown. Note that some other thread must be
  // responsible for shutting down the server for this call to ever return.
  server->Wait();

  return NULL;
}

void* RunServerForNodes(void* arg) {
  int port = 50056;
  std::string server_address("0.0.0.0:" + std::to_string(port));
  LBNodeCommService service(live_servers);

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
  std::cout << "Server for Nodes listening on " << server_address << std::endl;

  // Wait for the server to shutdown. Note that some other thread must be
  // responsible for shutting down the server for this call to ever return.
  server->Wait();

  return NULL;
}

int main(){
    pthread_t client_server_t, node_server_t;
  
    pthread_create(&client_server_t, NULL, RunServerForClient, NULL);
    pthread_create(&node_server_t, NULL, RunServerForNodes, NULL);

    pthread_join(client_server_t, NULL);
    pthread_join(node_server_t, NULL);
}