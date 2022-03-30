#include <iostream>
#include <memory>
#include <string>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "util/address_translation.h"
#include "util/wal.h"
#include "util/txn.h"
#include "util/state.h"
#include "util/kv_store.h"
#include <map>
#include <semaphore.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <chrono>
#include <ctime> 
#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
#include "blockstorage.grpc.pb.h"
#include "servercomm.grpc.pb.h"

/******************************************************************************
 * NAMESPACE
 *****************************************************************************/
using grpc::Channel;
using grpc::ClientContext;
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using grpc::ServerReaderWriter;
using grpc::ServerWriter;
using namespace blockstorage;
using namespace std::chrono;
using namespace std;

/******************************************************************************
 * MACROS
 *****************************************************************************/
#define BLOCK_SIZE 4096
// Log Levels - can be simplified, but isolation gives granular control
#define INFO
#define WARN
#define IS_DEBUG_ON
#define LEVEL_O_COUNT 1024
#define LEVEL_1_COUNT 256

#define DEBUG                       1                     
#define dbgprintf(...)              if (DEBUG) { printf(__VA_ARGS__); } 
/******************************************************************************
 * GLOBALS
 *****************************************************************************/
string SERVER_STORAGE_PATH = "/home/benitakbritto/CS-739-P3/storage/";
AddressTranslation atl;
WAL *wal;
map<string, int> KV_STORE;
KVStore kvObj;
string SERVER_1;
string SERVER_2;
// stores the information about the txn
// once txn is replicated across all replicas, it should be removed to avoid memory overflow
volatile map<string, Txn> txn_map;

// use this to block all writes during syncing backups
sem_t global_write_lock;

// TODO:: make this atomic counter to count the number of writes in flight
// then just do normal ++ and -- operations
int writes_in_flight = 0;

class Helper {
  public:

  string GenerateTempPath(std::string path){
    return path + ".tmp";
  }

  int WriteToTempFile(const std::string temp_path, const std::string original_path, const char* buffer, unsigned int size, int offset){
    #ifdef IS_DEBUG_ON
	  	  cout << "START:" << __func__ << endl;
        cout<<"server to write buffer size "<< size <<" to file:"<<temp_path<<endl;
	  #endif

    int fd_original = open(original_path.c_str(), O_RDONLY);
    if (fd_original == -1)
    {
      cout<<"ERR: server open local failed"<<__func__<<endl;
      perror(strerror(errno));
      return errno;
    }

    int fd_tmp = open(temp_path.c_str(), O_CREAT | O_WRONLY , 0777);
    if (fd_tmp == -1)
    {
      cout<<"ERR: server open local failed"<<__func__<<endl;
      perror(strerror(errno));
      return errno;
    }

    // Copy original to temp completely
    char original_content[4096];
    memset(original_content, '\0', 4096);
    int read_rc = read(fd_original, original_content, 4096);
    cout << "[INFO]: bytes read:" << read_rc << endl;
    if (read_rc == -1)
    {
      cout<<"ERR: read local failed in "<<__func__<<endl;
      perror(strerror(errno));
      return errno;
    }

    // TODO: make only one write call
    int write_rc = write(fd_tmp, original_content, read_rc);
    cout << "[INFO]: Copying original contents" << endl;
    cout << "[INFO]: bytes written:" << write_rc << endl;
    if (write_rc == -1)
    {
      cout<<"ERR: write local failed in "<<__func__<<endl;
      perror(strerror(errno));
      return errno;
    }
    dbgprintf("buffer %s, size: %d, offset %d\n", buffer, size, offset);
    // Do the actual write to tmp
    int res = pwrite(fd_tmp, buffer, size, offset);
    cout << "[INFO]: Writing user contents" << endl;
    cout << "[INFO]: bytes written to file:" << res << endl;
    if(res == -1){
      cout<<"ERR: pwrite local failed in "<<__func__<<endl;
      perror(strerror(errno));
      return errno;
    }
    fsync(fd_tmp);
    close(fd_tmp);
    close(fd_original);

    #ifdef IS_DEBUG_ON
	  	  cout << "END:" << __func__ << endl;
	  #endif

    return 0;    // TODO: check the error code
  }

};


class ServiceCommImpl final: public ServiceComm::Service {
  Helper helper;
  
  Status Prepare(ServerContext* context, const PrepareRequest* request, PrepareReply* reply) override {
    string txnId = request->transationid();
    string buffer = request->buffer();
    vector<pair<string, string>> rename_movs;
    int start=0;

    for (int i = 0; i < request->file_data_size(); i++)
    {
      string original_path = request->file_data(i).file_name();
      int size = request->file_data(i).size();
      int offset = request->file_data(i).offset();
      string temp_path = helper.GenerateTempPath(original_path);
      rename_movs.push_back(make_pair(temp_path, original_path));
      
      // write to tmp file
      int writeResp = helper.WriteToTempFile(temp_path, original_path, buffer.c_str()+start, size, offset);
      if(writeResp!=0){
        #ifdef INFO
          cout << "write to temp failed in "<< __func__ << endl;
        #endif
        reply->set_status(errno);
        perror(strerror(errno));
        return grpc::Status(grpc::StatusCode::NOT_FOUND, "failed to write bytes\n"); // Check suitable err code
      }
      start+=size;
    }

    // 4.2 Write to WAL

    wal->log_prepare(txnId, rename_movs);
    // TODO 4.3 Create and cp undo // NOT NEEDED
    // 4.4 Add id to KV store
    kvObj.UpdateStateOnKVStore(KV_STORE, txnId, START);
    reply->set_status(0);
    return Status::OK;
  }

  Status Commit(ServerContext* context, const CommitRequest* request, CommitReply* reply) override {
    string txnId = request->transationid();
    // 6.1 Rename 
    for (int i = 0; i < request->file_data_size(); i++)
    {
      string original_path = request->file_data(i).file_name();
      string temp_path = helper.GenerateTempPath(original_path);
      rename(temp_path.c_str(), original_path.c_str()); // rename temp to file
    }
    // 6.2 WAL Commit
    wal->log_commit(txnId);
    // 6.3 Remove from KV store
    kvObj.DeleteFromKVStore(KV_STORE, txnId);

    reply->set_status(0);
    return Status::OK;
  }

  Status GetTransactionStatus(ServerContext* context, const GetTransactionStatusRequest* request, GetTransactionStatusReply* reply) override {
    reply->set_status(0);
    return Status::OK;
  }

  Status Sync(ServerContext* context, ServerReaderWriter<SyncReply, SyncRequest>* stream) override{
    SyncRequest request;

    while (stream->Read(&request)) {
        #ifdef DEBUG
          cout << "[INFO]: Recv sync request from [IP:]" << request.ip() << endl;
        #endif

        SyncRequest_Commands command = request.command();

        if (SyncRequest_Commands_STOP_WRITE == command) {
          sem_wait(&global_write_lock);
          SyncReply reply;
          reply.set_error(0);
          stream->Write(reply);
        } else if (SyncRequest_Commands_WRITES_IN_FLIGHT == command) {
          SyncReply reply;
          reply.set_count(writes_in_flight);
          stream->Write(reply);
        } else if (SyncRequest_Commands_GET_WRITES == command) {
          // TODO: get all write txn from map one by one

          // Step - 1: Send Ops
          // Step - 2: Wait for the ack and then update the local KV store
        } else {
            cout << "[ERROR]: Unknown command" << endl;
            break;
        }
        
    }

    sem_post(&global_write_lock);

    return Status::OK;
  }  

};

// Logic and buffer behind the server's behavior.
class BlockStorageServiceImpl final : public BlockStorage::Service {

  // string myIP;
  // string otherIP;
  std::unique_ptr<ServiceComm::Stub> _stub;
  Helper helper;
  
  public:
  BlockStorageServiceImpl(string _otherIP){
    // myIP=myIP;
    // otherIP=_otherIP;
    _stub = ServiceComm::NewStub(grpc::CreateChannel(_otherIP, grpc::InsecureChannelCredentials()));
  }

  string CreateTransactionId()
  {
    return to_string(duration_cast<nanoseconds>(system_clock::now().time_since_epoch()).count());
  }

  Status Read(ServerContext* context, const ReadRequest* request,
                  ReadReply* reply) override {
    
    int address = request->addr();
    std::string readContent;

    std::vector<PathData> pathData = atl.GetAllFileNames(address);

    for(PathData pd : pathData) {
      cout<<pd.path<<endl;
      int fd = open(pd.path.c_str(), O_RDONLY);
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

    dbgprintf("reached BS server write  \n");
    int address = 0;
    string buffer = "";
    int start = 0;
    vector<PathData> pathData;
    vector<pair<string, string>> rename_movs;
    string txnId = "";

    address = request->addr();
    buffer = request->buffer();
    
    // fetch files from ATL
    pathData = atl.GetAllFileNames(address);
    
    // 3.2 : create and cp tmp
    for(PathData pd : pathData) {
      // get tmp file name
      string temp_path = helper.GenerateTempPath(pd.path.c_str());
      string original_path = pd.path;
      // tmp file, orginal file
      rename_movs.push_back(make_pair(temp_path, original_path));
      // write to tmp file
      int writeResp = helper.WriteToTempFile(temp_path, original_path, buffer.c_str()+start, pd.size, pd.offset);
      if (writeResp != 0) {
        #ifdef INFO
          cout << "pwrite failed" << endl;
        #endif
        reply->set_error(errno);
        perror(strerror(errno));
        return grpc::Status(grpc::StatusCode::NOT_FOUND, "failed to write bytes\n"); // TODO: set correct status code
      }
      start += pd.size;
      // TODO: save to cache
     // rename(temp_path.c_str(), pd.path.c_str());
    }
    dbgprintf("temp files written\n");
    //  3.3 Write txn to WAL (start + mv)
    txnId = CreateTransactionId();
    wal->log_prepare(txnId, rename_movs);
    
    // 3.4 create and cp to undo file - NOT NEEDED

    // 3.5 Add id to KV store (ordered map)
    kvObj.UpdateStateOnKVStore(KV_STORE, txnId, START);
    
    // 3.6 call prepare()
    int prepareResp = callPrepare(txnId, buffer, pathData);
    
    if(prepareResp == grpc::StatusCode::OK) { //if prepare() succeeds  
      // 5.1 rename 
      for(pair<string,string> temp_file_pair: rename_movs){
        // old name (tmp file), new name (original file)
        rename(temp_file_pair.first.c_str(), temp_file_pair.second.c_str());
      }
      // 5.2 WAL RPCinit
      wal->log_replication_init(txnId);
      
      // 5.3 Update KV store
      kvObj.UpdateStateOnKVStore(KV_STORE, txnId, RPC_INIT);

      // 5.4 call commit()
      int commitResp = callCommit(txnId, pathData);
        // 5.5 if commit() succeeds:
        if (commitResp == grpc::StatusCode::OK)
        {
          // 7.1 WAL commit
          wal->log_commit(txnId);
          // 7.2 Remove from KV store
          kvObj.DeleteFromKVStore(KV_STORE, txnId);
          // 7.3 Respond success
          return Status::OK;
        }
        // commit() fails: if backup is unavailable
        else if (commitResp == grpc::StatusCode::UNAVAILABLE)
        {
          // 6.1 rename 
          for(pair<string,string> temp_file_pair: rename_movs) {
            // old name (tmp file), new name (original file)
            rename(temp_file_pair.first.c_str(), temp_file_pair.second.c_str());
          }
            // 6.2 WAL "pending replication"
            wal->log_pending_replication(txnId);
            // 6.3 Update KV store "Pending on backup"
            kvObj.UpdateStateOnKVStore(KV_STORE, txnId, PENDING_REPLICATION);
        }
        // if commit() fails
        else
        {
          // Remove from KV store
          kvObj.DeleteFromKVStore(KV_STORE, txnId);
          // Return failure
          return grpc::Status(grpc::StatusCode::UNKNOWN, "failed to complete write operation\n"); // TODO: Check if this status code is appropriate
        }       
    }
    // if prepare() fails
    else{ 
      // 5.1 check status==Unavailable
      if (prepareResp == grpc::StatusCode::UNAVAILABLE)
      {
        //  6.1 rename tmp to original file
        for(pair<string,string> temp_file_pair: rename_movs) {
          // old name (tmp file), new name (original file)
          rename(temp_file_pair.first.c_str(), temp_file_pair.second.c_str());
        }
        // 6.2 WAL "pending replication"
        wal->log_pending_replication(txnId);
        // 6.3 Update KV store "Pending on backup"
        kvObj.UpdateStateOnKVStore(KV_STORE, txnId, PENDING_REPLICATION);
      }
      // 5.1 Else
      else
      {
        //  6.1 WAL Abort
         wal->log_abort(txnId);
        // 6.2 Remove from KV
        kvObj.DeleteFromKVStore(KV_STORE, txnId);
        // 6.3 Send failure status
        return grpc::Status(grpc::StatusCode::UNKNOWN, "failed to complete write operation\n"); // TODO: Check if this status code is appropriate
      }   
    }
    return Status::OK;
  }

  int callPrepare(string txnId, string buf, vector<PathData> pathData){
    dbgprintf("Entering callPrepare\n");
    ClientContext context;
    PrepareRequest request;
    PrepareReply reply;

    request.set_transationid(txnId);
    request.set_buffer(buf);
    FileData* fileData = request.add_file_data();

    for(PathData pd: pathData){
      fileData->set_file_name(pd.path);
      fileData->set_size(pd.size); 
      fileData->set_offset(pd.offset);
    }
    
    Status status = _stub->Prepare(&context, request, &reply);
    dbgprintf("status code: %d\n", status.error_code());
    dbgprintf("Exiting callPrepare\n");
    return status.error_code();
  }

  int callCommit(string txnId, vector<PathData> pathData){
    dbgprintf("Entering callCommit\n");
    ClientContext context;
    CommitRequest request;
    CommitReply reply;

    request.set_transationid(txnId);
    
    auto * file_data = request.add_file_data();
    for(PathData pd: pathData){
      file_data->set_file_name(pd.path);
    }
    
    Status status = _stub->Commit(&context, request, &reply);
    dbgprintf("status code: %d\n", status.error_code());
    dbgprintf("Exiting callCommit\n");
    return status.error_code();
  }
  
};

void PrepareStorage() {
  
  #ifdef INFO
    cout << "[INFO] Preparing Storage Path" << endl;
  #endif

  int res = mkdir(SERVER_STORAGE_PATH.c_str(), 0777);

  if (res == -1 && errno == EEXIST) {
    // #ifdef WARN
    //   cout << "[WARN] Server storage dir:" << SERVER_STORAGE_PATH << " already exists." << endl;
    // #endif
  }

  for(int dirId = 0; dirId < LEVEL_O_COUNT; dirId++) {

    // check if the directory is already is created
    string dir = SERVER_STORAGE_PATH + "/" + to_string(dirId);

    res = mkdir(dir.c_str(), 0777);

    if (res == -1 && errno == EEXIST) {
      // #ifdef WARN
      //   cout << "[WARN] Dir:" << dir << " already exists." << endl;
      // #endif
    }

    for(int fileId = 0; fileId < LEVEL_1_COUNT; fileId++) {
      string file = dir + "/" + std::to_string(fileId);

      res = open(file.c_str(), O_CREAT | O_EXCL, 0777);

      if (res == -1 && errno == EEXIST) {
        // #ifdef WARN
        //   cout << "[WARN] File:" << file << " already exists." << endl;
        // #endif
      }
    }
  }

}

void *RunBlockStorageServer(void* _otherIP) {
  // SERVER_1 = "0.0.0.0:" + std::to_string(port);
  char* otherIP = (char*)_otherIP;
  cout<< otherIP <<" received\n";
  std::string server_address("0.0.0.0:50051");
  BlockStorageServiceImpl service(otherIP);

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
  std::cout << "BlockStorage Server listening on " << server_address << std::endl;

  // Wait for the server to shutdown. Note that some other thread must be
  // responsible for shutting down the server for this call to ever return.
  server->Wait();

  return NULL;
}

void *RunCommServer(void* _otherIP) {
  // SERVER_2 = "20.109.180.121:";
  string server_address("0.0.0.0:50052");
  ServiceCommImpl service;

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
  std::cout << "CommServer listening on " << server_address << std::endl;

  // Wait for the server to shutdown. Note that some other thread must be
  // responsible for shutting down the server for this call to ever return.
  server->Wait();

  return NULL;
}

int main(int argc, char** argv) {
  PrepareStorage();
  // Write Ahead Logger
  wal = new WAL(SERVER_STORAGE_PATH);

  // global write semaphore
  sem_init(&global_write_lock, 0, 1);

  pthread_t block_server_t, comm_server_t;
  // int port1 = 50051, port2 = 50052;
  
  pthread_create(&block_server_t, NULL, RunBlockStorageServer, argv[1]);
  pthread_create(&comm_server_t, NULL, RunCommServer, NULL);

  pthread_join(block_server_t, NULL);
  pthread_join(comm_server_t, NULL);

  return 0;
}

