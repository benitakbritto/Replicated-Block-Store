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

#define PRIMARY_STR "PRIMARY"
#define BACKUP_STR "BACKUP"

#define DEBUG                       1                     
#define dbgprintf(...)              if (DEBUG) { printf(__VA_ARGS__); } 

map<string, string> live_servers;
map<string, BlockStorageClient*> bs_clients;

class BlockStorageService final : public BlockStorage::Service {

    private:
        // for round-robin
        int idx = 
        0;
        map<string, string>* nodes;
        map<string, BlockStorageClient*>* bs_clients;

        bool is_primary_registered() {
            return nodes->find(PRIMARY_STR) == nodes->end();
        }

        string getServerToRouteTo(){
            idx = 1 - idx;
            if(idx == 0 && is_primary_registered()) {
                return PRIMARY_STR;
            }

            return BACKUP_STR;
        }

    public:
        BlockStorageService(map<string, string> &servers, map<string, BlockStorageClient*>& clients){
            nodes = &servers;
            bs_clients = &clients;
        }

        Status Read(ServerContext* context, const ReadRequest* request,
                  ReadReply* reply) override {
            string key = getServerToRouteTo();
            dbgprintf("Routing read to %s\n", key.c_str());
            return (*bs_clients)[key]->Read(*request, reply, request->addr());
        }

        Status Write(ServerContext* context, const WriteRequest* request,
                  WriteReply* reply) override {
            dbgprintf("reached LB write \n");
            return (*bs_clients)[PRIMARY_STR]->Write(request->addr(), request->buffer());
        }
};

class LBNodeCommService final: public LBNodeComm::Service {
    private:
        map<string, string>* nodes;
        map<string, BlockStorageClient*>* bs_clients;

        void register_node(string identity, string target_str) {
            (*nodes)[identity] = target_str;

            bs_clients->insert(make_pair(
                identity,
                new BlockStorageClient (grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials()))));
        }

        void erase_node(string identity) {
            if (identity.empty()) return;

            nodes->erase(identity);
            bs_clients->erase(identity);
        }

        string get_peer_ip(string identity) {
            if (identity.compare(PRIMARY_STR) == 0) {
                return (*nodes)[BACKUP_STR];
            } else {
                return (*nodes)[PRIMARY_STR];
            }
        }

        void print_identity() {
            cout << PRIMARY_STR << ":" << get_peer_ip(PRIMARY_STR) << endl;
            cout << BACKUP_STR << ":" << get_peer_ip(BACKUP_STR) << endl;
        }

    public:
        LBNodeCommService(map<string, string> &servers, map<string, BlockStorageClient*> &clients) {
            nodes = &servers;
            bs_clients = &clients;
        }

        Status SendHeartBeat(ServerContext* context, ServerReaderWriter<HeartBeatReply, HeartBeatRequest>* stream) override {
            cout << "[INFO]: server IP" << context->peer() << endl;
            HeartBeatRequest request;
            HeartBeatReply reply;

            string prev_identity;
            string identity;
            bool first_time = true;
            // TODO: create client on the fly

            while(1) {
                if(!stream->Read(&request)) {
                    break;
                }
                cout << "[INFO]: recv heartbeat from IP:[" << request.ip() <<  "]" << endl;

                identity = Identity_Name(request.identity());

                if (identity.compare(prev_identity) != 0) {
                    if (first_time) {
                        cout << "[INFO]: registering first time" << endl;
                        first_time = false;
                    } else {
                        cout << "[WARN]: Failover" << endl;
                        erase_node(identity);
                    }
                    
                    register_node(identity, request.ip());
                } else {
                    cout << "[INFO]: not registering again" << endl;
                }

                prev_identity = identity;
                
                string peer_ip = get_peer_ip(identity);

                if (peer_ip.empty()) {
                    reply.clear_peer_ip();
                } else {
                    reply.set_peer_ip(peer_ip);
                }

                if(!stream->Write(reply)) {
                    break;
                }
                cout << "[INFO]: sent heartbeat reply" << endl;
            }

            cout << "[ERROR]: stream broke" << endl;
            erase_node(identity);

            return Status::OK;
        }
};

void* RunServerForClient(void* arg) {
  int port = 50051;
  std::string server_address("0.0.0.0:" + std::to_string(port));
  BlockStorageService service(live_servers, bs_clients);

  grpc::EnableDefaultHealthCheckService(true);
  grpc::reflection::InitProtoReflectionServerBuilderPlugin();
  ServerBuilder builder;
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);
  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cout << "Server for Client listening on " << server_address << std::endl;

  server->Wait();

  return NULL;
}

void* RunServerForNodes(void* arg) {
  int port = 50056;
  std::string server_address("0.0.0.0:" + std::to_string(port));
  LBNodeCommService service(live_servers, bs_clients);

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