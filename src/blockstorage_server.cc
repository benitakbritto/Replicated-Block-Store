/*
 *
 * Copyright 2015 gRPC authors.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include <iostream>
#include <memory>
#include <string>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "address_translation.h"

#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>

#ifdef BAZEL_BUILD
#include "examples/protos/blockstorage.grpc.pb.h"
#else
#include "blockstorage.grpc.pb.h"
#endif

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

#define BLOCK_SIZE 4096

using namespace std;

// Log Levels - can be simplified, but isolation gives granular control
#define INFO
// #define WARN

#define LEVEL_O_COUNT 1024
#define LEVEL_1_COUNT 256

string SERVER_STORAGE_PATH = "/home/benitakbritto/CS-739-P3/storage/";

AddressTranslation atl;

// Logic and data behind the server's behavior.
class BlockStorageServiceImpl final : public BlockStorage::Service {
  
  // std::vector<PathData> testATLSim(){
  //   // cout<<"reached atl \n";
  //   PathData testpd;
  //   testpd.path="/home/benitakbritto/CS-739-P3/src/abc.txt";
  //   testpd.size=30;
  //   testpd.offset=0;
  //   std::vector<PathData> pdVec;
  //   pdVec.push_back(testpd);
  //   return pdVec;
  // }

  Status Read(ServerContext* context, const ReadRequest* request,
                  ReadReply* reply) override {
    
    int address = request->addr();
    std::string readContent;

    std::vector<PathData> pathData = atl.GetAllFileNames(address);

    for(PathData pd : pathData) {
      cout<<SERVER_STORAGE_PATH + pd.path<<endl;
      int fd = open((SERVER_STORAGE_PATH + pd.path).c_str(), O_RDONLY);
      if (fd == -1){
        reply->set_error(errno);
        return grpc::Status(grpc::StatusCode::NOT_FOUND, "failed to get fd\n");
      }

      char *buf = new char[pd.size];
      int bytesRead = pread(fd, buf, pd.size, pd.offset);
      if (bytesRead == -1){
        cout << "pread failed" << endl;
        reply->set_error(errno);
        perror(strerror(errno));
        close(fd);
        return grpc::Status(grpc::StatusCode::NOT_FOUND, "failed to read bytes\n");
      }
      readContent += buf;
      close(fd);
    }
    
    reply->set_buffer(readContent);
    return Status::OK;
  }

  Status Write(ServerContext* context, const WriteRequest* request,
                  WriteReply* reply) override {
    
    int address = request->addr();
    string data = request->buffer();
    int start = 0;
    // TODO: Call ATL to fetch actual address
    std::vector<PathData> pathData = atl.GetAllFileNames(address);

    for(PathData pd : pathData) {
      int fd = open((SERVER_STORAGE_PATH + pd.path).c_str(), O_WRONLY);
      if (fd == -1){
        reply->set_error(errno);
        return grpc::Status(grpc::StatusCode::NOT_FOUND, "failed to get fd\n");
      }
      std::string temp_path = generateTempPath(pd.path.c_str());
      int bytesWritten = WriteToTempFile(temp_path, data.c_str()+start, pd.size, pd.offset);
      
      if (bytesWritten == -1){
        
        #ifdef INFO
          cout << "pwrite failed" << endl;
        #endif

        reply->set_error(errno);
        perror(strerror(errno));
        close(fd);
        return grpc::Status(grpc::StatusCode::NOT_FOUND, "failed to write bytes\n");
      }
      start+=pd.size;
      close(fd);
      // TODO: save to cache
      rename(temp_path.c_str(), pd.path.c_str());
    }
    
    return Status::OK;
  }

  int WriteToTempFile(const std::string temp_path, std::string buffer, unsigned int size, int offset){
    #ifdef IS_DEBUG_ON
	  	  cout << "START:" << __func__ << endl;
        cout<<"server to write data size "<< size <<" to file:"<<temp_path<<endl;
	  #endif
    
    int fd = open(temp_path.c_str(), O_CREAT|O_EXCL, 0777);
    close(fd);
    fd = open(temp_path.c_str(), O_RDWR, 0644);
    if(fd == -1){
      cout<<"ERR: server open local failed"<<__func__<<endl;
      perror(strerror(errno));
      return errno;
    }
    cout<<"buffer: "<<buffer<<endl;

    int res = pwrite(fd, buffer.c_str(), size, offset);
    if(res == -1){
      cout<<"ERR: pwrite local failed in "<<__func__<<endl;
      perror(strerror(errno));
      return errno;
    }
    fsync(fd);
    close(fd);

    #ifdef IS_DEBUG_ON
	  	  cout << "END:" << __func__ << endl;
	  #endif

    return 0;    // TODO: check the error code
  }

  string generateTempPath(std::string path){
    return path + ".tmp" + to_string(rand() % 101743);
  }
};

void PrepareStorage() {
  
  #ifdef INFO
    cout << "[INFO] Preparing Storage Path" << endl;
  #endif

  int res = mkdir(SERVER_STORAGE_PATH.c_str(), 0777);

  if (res == -1 && errno == EEXIST) {
    #ifdef WARN
      cout << "[WARN] Server storage dir:" << SERVER_STORAGE_PATH << " already exists." << endl;
    #endif
  }

  for(int dirId = 0; dirId < LEVEL_O_COUNT; dirId++) {

    // check if the directory is already is created
    string dir = SERVER_STORAGE_PATH + "/" + to_string(dirId);

    res = mkdir(dir.c_str(), 0777);

    if (res == -1 && errno == EEXIST) {
      #ifdef WARN
        cout << "[WARN] Dir:" << dir << " already exists." << endl;
      #endif
    }

    for(int fileId = 0; fileId < LEVEL_1_COUNT; fileId++) {
      string file = dir + "/" + std::to_string(fileId);

      res = open(file.c_str(), O_CREAT | O_EXCL, 0777);

      if (res == -1 && errno == EEXIST) {
        #ifdef WARN
          cout << "[WARN] File:" << file << " already exists." << endl;
        #endif
      }
    }
  }
}

void RunServer(int port) {

  std::string server_address("0.0.0.0:" + std::to_string(port));
  BlockStorageServiceImpl service;

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

int main(int argc, char** argv) {
  PrepareStorage();
  RunServer(50051);
}