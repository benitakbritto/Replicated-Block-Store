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
#include <mutex>
#include <shared_mutex>
#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
#include "blockstorage.grpc.pb.h"
#include "servercomm.grpc.pb.h"
#include "lb.grpc.pb.h"
#include "util/crash_recovery.h"
#include "util/locks.h"

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
using grpc::ClientReaderWriter;
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
/*****************************************************************************
 * GLOBALS
 *****************************************************************************/
AddressTranslation atl;
WAL *wal;
map<string, TxnData> KV_STORE;
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

int HB_SLEEP_IN_SEC = 5;

string PRIMARY_STR("PRIMARY");
string BACKUP_STR("BACKUP");

string self_addr_lb;
string lb_addr;

class Helper {
  public:

  string GenerateTempPath(std::string path){
    return path + ".tmp";
  }

  int WriteToTempFile(const std::string temp_path, const std::string original_path, const char* buffer, unsigned int size, int offset){
    dbgprintf("[INFO] WriteToTempFile: Entering function\n");
    dbgprintf("[INFO] WriteToTempFile: temp_path = %s | original_path = %s\n", temp_path.c_str(), original_path.c_str());
    dbgprintf("[INFO] WriteToTempFile: buffer %s\n", buffer);
    dbgprintf("[INFO] WriteToTempFile: size = %d | offset = %d\n", size, offset);

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
    dbgprintf("[INFO] WriteToTempFile: read_rc = %d\n", read_rc);
    dbgprintf("[INFO] WriteToTempFile: original_content = %s\n", original_content);
    if (read_rc == -1)
    {
      cout<<"ERR: read local failed in "<<__func__<<endl;
      perror(strerror(errno));
      return errno;
    }

    // TODO: make only one write call
    int write_rc = write(fd_tmp, original_content, read_rc);
    dbgprintf("[INFO] WriteToTempFile: write_rc = %d\n", write_rc);
    if (write_rc == -1)
    {
      cout<<"[ERR] WriteToTempFile: write local failed" << endl;
      perror(strerror(errno));
      return errno;
    }
    // Do the actual write to tmp
    int pwrite_rc = pwrite(fd_tmp, buffer, size, offset);
    dbgprintf("[INFO] WriteToTempFile: pwrite_rc = %d\n", pwrite_rc);
    if(pwrite_rc == -1){
      cout<<"[ERR] WriteToTempFile: pwrite local failed" << endl;
      perror(strerror(errno));
      return errno;
    }
    fsync(fd_tmp);
    close(fd_tmp);
    close(fd_original);
    dbgprintf("[INFO] WriteToTempFile: Exiting function\n");
    return 0;    // TODO: check the error code
  }

  string GetData(string file_path, int size, int offset)
  {
    dbgprintf("[INFO] GetData: Entering function\n");
    int fd =  open(file_path.c_str(), O_RDONLY);
    if (fd == -1)
    {
      dbgprintf("[INFO] GetData: failed to open file\n");
      return string("");
    }

    char content[size];
    memset(content, '\0', size);
    int pread_rc = pread(fd, content, size, offset);
    if (pread_rc == -1)
    {
      dbgprintf("[ERR] GetData: pread failed\n");
      close(fd);
      return string("");
    }

    dbgprintf("[INFO] GetData: content = %s\n", string(content).c_str());
    dbgprintf("[INFO] GetData: Exiting function\n");
    close(fd);
    return string(content);
  }
};

/*
* For Backup
*/
class ServiceCommImpl final: public ServiceComm::Service {
  Helper helper;

  Status Ping(ServerContext* context, const PingServRequest* request, PingServReply* reply) override {
    return Status::OK;
  }
  
  Status Prepare(ServerContext* context, const PrepareRequest* request, PrepareReply* reply) override {
    dbgprintf("[INFO] Prepare: Entering function\n");
    string txnId = request->transationid();
    string buffer = request->buffer();
    dbgprintf("[INFO] Prepare: txnId = %s | buffer = %s\n", txnId.c_str(), buffer.c_str());

    vector<pair<string, string>> rename_movs;
    vector<string> original_files;
    vector<int> sizes;
    vector<int> offsets;
    int start=0;

    for (int i = 0; i < request->file_data_size(); i++)
    {
      dbgprintf("[INFO] Prepare: start %d\n", start);
      string original_path = request->file_data(i).file_name();
      original_files.push_back(original_path);
      dbgprintf("[INFO] Prepare: original_path %s\n", original_path.c_str());
      int size = request->file_data(i).size();
      sizes.push_back(size);
      int offset = request->file_data(i).offset();
      offsets.push_back(offset);
      dbgprintf("[INFO] Prepare: size = %d | offset = %d\n", size, offset);
      string temp_path = helper.GenerateTempPath(original_path);
      dbgprintf("[INFO] Prepare: temp_path = %s\n", temp_path.c_str());
      rename_movs.push_back(make_pair(temp_path, original_path));
      
      // write to tmp file
      int writeResp = helper.WriteToTempFile(temp_path, original_path, buffer.c_str()+start, size, offset);
      dbgprintf("[INFO] Prepare: writeResp = %d\n", writeResp);
      if(writeResp!=0){
        dbgprintf("[ERR] Prepare: write to temp failed\n");
        // reply->set_status(errno);
        perror(strerror(errno));
        return grpc::Status(grpc::StatusCode::NOT_FOUND, "failed to write bytes\n"); // Check suitable err code
      }
      start+=size;
    }

    // 4.2 Write to WAL
    wal->log_prepare(txnId, rename_movs);

    // TODO 4.3 Create and cp undo // NOT NEEDED

    // 4.4 Add id to KV store
    kvObj.AddToKVStore(KV_STORE, txnId, original_files, sizes, offsets);
    reply->set_status(0);
    dbgprintf("[INFO] Prepare: Exiting function\n");
    return Status::OK;
  }

  Status Commit(ServerContext* context, const CommitRequest* request, CommitReply* reply) override {
    dbgprintf("[INFO] Commit: Entering function\n");
    string txnId = request->transationid();
    dbgprintf("[INFO] Commit: txnId %s\n", txnId.c_str());
    // 6.1 Rename 
    for (int i = 0; i < request->file_data_size(); i++)
    {
      string original_path = request->file_data(i).file_name();
      dbgprintf("[INFO] Commit: original_path = %s\n", original_path.c_str());
      string temp_path = helper.GenerateTempPath(original_path);
      dbgprintf("[INFO] Commit: temp_path = %s\n", temp_path.c_str());
      rename(temp_path.c_str(), original_path.c_str()); // rename temp to file
    }
    // 6.2 WAL Commit
    wal->log_commit(txnId);
    // 6.3 Remove from KV store
    kvObj.DeleteFromKVStore(KV_STORE, txnId);

    reply->set_status(0);
    dbgprintf("[INFO] Commit: Exiting function\n");
    return Status::OK;
  }

  Status GetPendingReplicationTransactions(ServerContext* context, 
                                          const GetPendingReplicationTransactionsRequest* request, 
                                          GetPendingReplicationTransactionsReply* reply) override {
    dbgprintf("[INFO] GetPendingReplicationTransactions: Entering function\n");
    
    for (auto itr = KV_STORE.begin(); itr != KV_STORE.end(); itr++)
    {
      if (itr->second.state == PENDING_REPLICATION)
      {
        dbgprintf("[INFO] GetPendingReplicationTransactions: transaction id = %s\n", (itr->first).c_str());
        auto data = reply->add_txn();
        data->set_transaction_id(itr->first);
        dbgprintf("[INFO] GetPendingReplicationTransactions: transaction id = %s\n", (data->transaction_id()).c_str());
      }
    }

    dbgprintf("[INFO] GetPendingReplicationTransactions: Exiting function\n");
    return Status::OK;
  }

  Status ForcePendingWrites(ServerContext* context, const ForcePendingWritesRequest* request, ServerWriter<ForcePendingWritesReply>*writer) override {
    dbgprintf("[INFO] ForcePendingWrites: Entering function\n");
    ForcePendingWritesReply reply;
    for (int i = 0; i < request->txn_size(); i++)
    {
      string pending_write_transaction_id = request->txn(i).transaction_id();
      vector<string> original_files;
      vector<int> sizes;
      vector<int> offsets;
      kvObj.GetTransactionDataFromKVStore(KV_STORE, 
                                          pending_write_transaction_id,
                                          original_files,
                                          sizes,
                                          offsets);
      int len = original_files.size();
      for (int j = 0; j < len; j++)
      {
        string content = helper.GetData(original_files[j], sizes[j], offsets[j]);
        reply.set_transaction_id(pending_write_transaction_id);
        reply.set_file_name(original_files[j]);
        reply.set_content(content);
        reply.set_size(sizes[j]);
        reply.set_offset(offsets[j]);
        writer->Write(reply);
      }
      dbgprintf("[INFO] ForcePendingWrites: Exiting function\n");
      return Status::OK;
    }

    dbgprintf("[INFO] ForcePendingWrites: Exiting function\n");
    return Status::OK;
  }

  Status Sync(ServerContext* context, ServerReaderWriter<SyncReply, SyncRequest>* stream) override{
    SyncRequest request;

    int count_down = 3;
    int pending_writes = 3;

    while (stream->Read(&request)) {
        SyncRequest_Commands command = request.command();
        #ifdef DEBUG
          cout << "[INFO] Sync: Recv sync request from [IP:]" << request.ip() << ",[CMD:]" << SyncRequest_Commands_Name(command) << endl;
        #endif

        if (SyncRequest_Commands_STOP_WRITE == command) {
          cout << "[INFO] Sync: acquiring the global write lock" << endl;
          sem_wait(&global_write_lock);

          SyncReply reply;
          reply.set_error(0);
          stream->Write(reply);
        } else if (SyncRequest_Commands_WRITES_STATUS == command) {
          SyncReply reply;
          reply.set_inflight_writes(count_down);
          // TODO: set the KV size
          reply.set_pending_writes(pending_writes);
          stream->Write(reply);
          count_down--;
        } else if (SyncRequest_Commands_GET_WRITES == command) {
          // TODO: get all write txn from map one by one
          SyncReply reply;
          SyncRequest request;
          for(int i = 0; i < pending_writes; i++) {
            // Step - 1: Send Ops
            reply.set_error(i);
            stream->Write(reply);
            cout << "[INFO] Sync: sent txn num:" << reply.error() << endl;
            
            // Step - 2: Wait for the ack and then do some bookkeeping 
            stream->Read(&request);
            cout << "[INFO] Sync: got ack" << endl;

            // Step - 2.1: book keeping
          }

          goto RELEASE_GLOBAL_LOCK;
          
        } else {
            cout << "[ERROR] Sync: Unknown command" << endl;
            break;
        }
        
    }

    RELEASE_GLOBAL_LOCK: 
    cout << "[INFO] Sync: releasing the global write lock" << endl;
    sem_post(&global_write_lock);

    return Status::OK;
  }  

  Status GetTransactionState(ServerContext* context, const GetTransactionStateRequest* request, GetTransactionStateReply* reply) override {
    dbgprintf("[INFO] GetTransactionState: Entering function\n");
    int state = kvObj.GetStateFromKVStore(KV_STORE, request->txn_id());
    reply->set_state(state);
    dbgprintf("[INFO] GetTransactionState: Exiting function\n");
    return Status::OK;
  }
};

// Logic and buffer behind the server's behavior.
class BlockStorageServiceImpl final : public BlockStorage::Service {
private:
  Helper helper;
  MutexMap mutexMap;
  
public:
  std::unique_ptr<ServiceComm::Stub> _stub;

  BlockStorageServiceImpl(string _otherIP){
    _stub = ServiceComm::NewStub(grpc::CreateChannel(_otherIP, grpc::InsecureChannelCredentials()));
    // mutexMap = MutexMap();
  }

  string CreateTransactionId()
  {
    return to_string(duration_cast<nanoseconds>(system_clock::now().time_since_epoch()).count());
  }

  Status Ping(ServerContext* _context, const PingRequest* _request, PingReply* _reply) override {
    ClientContext context;
    PingServRequest request;
    PingServReply reply;
    return _stub->Ping(&context, request, &reply);
  }

  Status Read(ServerContext* context, const ReadRequest* request,
                  ReadReply* reply) override {
    
    dbgprintf("[INFO] Read: Entering function\n");
    int address = request->addr();
    dbgprintf("[INFO] Read: address = %d\n", address);
    std::string readContent;

    std::vector<PathData> pathData = atl.GetAllFileNames(address);

    for(PathData pd : pathData) {
      dbgprintf("[INFO] Read: pd.path = %s\n", pd.path.c_str());
      int fd = open(pd.path.c_str(), O_RDONLY);
      if (fd == -1){
         cout << "[ERR] Read: open failed" << endl;
        reply->set_error(errno);
        return grpc::Status(grpc::StatusCode::NOT_FOUND, "failed to get fd\n");
      }

      char *buf = new char[pd.size+1];
      memset(buf, '\0', pd.size+1);
      
      dbgprintf("[INFO] Read: Acquiring read lock");
      std::shared_lock<std::shared_mutex> readLock = mutexMap.GetReadLock(pd.path.c_str());
      int bytesRead = pread(fd, buf, pd.size, pd.offset);
      readLock.unlock();
      dbgprintf("[INFO] Read: Released read lock");

      dbgprintf("[INFO] Read: bytesRead = %d, starting at offset=%d, size=%d\n", bytesRead, pd.offset, pd.size);
      if (bytesRead == -1){
        cout << "[ERR] Read: pread failed" << endl;
        reply->set_error(errno);
        perror(strerror(errno));
        close(fd);
        return grpc::Status(grpc::StatusCode::NOT_FOUND, "failed to read bytes\n");
      }
      dbgprintf("[INFO] Read: pread buf = %s\n", buf);
      readContent += string(buf);
      dbgprintf("[INFO] Read: readContent = %s\n", readContent.c_str());
      close(fd);
    }

    reply->set_buffer(readContent);
    return Status::OK;
  }

  Status Write(ServerContext* context, const WriteRequest* request,
                  WriteReply* reply) override {

    dbgprintf("[INFO] Write: Entering function\n");
    int address = 0;
    string buffer = "";
    int start = 0;
    vector<PathData> pathData;
    vector<pair<string, string>> rename_movs;
    vector<string> original_files;
    vector<int> sizes;
    vector<int> offsets;
    string txnId = "";

    address = request->addr();
    dbgprintf("[INFO] Write: address = %d\n", address);
    buffer = request->buffer();
    dbgprintf("[INFO] Write: buffer = %s\n", buffer.c_str());
    
    // fetch files from ATL
    pathData = atl.GetAllFileNames(address);
    
    // 3.2 : create and cp tmp
    for(PathData pd : pathData) {
      dbgprintf("[INFO] Write: start = %d\n", start);
      // get tmp file name
      string temp_path = helper.GenerateTempPath(pd.path.c_str());
      dbgprintf("[INFO] Write: temp_path = %s\n", temp_path.c_str());
      string original_path = pd.path;
      original_files.push_back(original_path);
      sizes.push_back(pd.size);
      offsets.push_back(pd.offset);
      dbgprintf("[INFO] Write: original_path = %s | size = %d | offset = %d \n", original_path.c_str(), pd.size, pd.offset);
      // tmp file, orginal file
      rename_movs.push_back(make_pair(temp_path, original_path));
      // write to tmp file
      int writeResp = helper.WriteToTempFile(temp_path, original_path, buffer.c_str()+start, pd.size, pd.offset);
      dbgprintf("[INFO] Write: writeResp = %d\n", writeResp);
      if (writeResp != 0) {
        #ifdef INFO
          cout << "[ERR] Write: WriteToTempFile failed" << endl;
        #endif
        reply->set_error(errno);
        perror(strerror(errno));
        return grpc::Status(grpc::StatusCode::NOT_FOUND, "failed to write bytes\n"); // TODO: set correct status code
      }
      start += pd.size;
      // TODO: save to cache
     // rename(temp_path.c_str(), pd.path.c_str());
    }
    //  3.3 Write txn to WAL (start + mv)
    txnId = CreateTransactionId();
    dbgprintf("[INFO] Write: txnId = %s\n", txnId.c_str());
    wal->log_prepare(txnId, rename_movs);
    
    // 3.4 create and cp to undo file - TODO

    // 3.5 Add id to KV store (ordered map)
    kvObj.AddToKVStore(KV_STORE, txnId, original_files, sizes, offsets);
    
    // 3.6 call prepare()
    Status prepareResp = callPrepare(txnId, buffer, pathData);
    dbgprintf("[INFO] Write: prepareResp = %d\n", prepareResp.error_code());
    
    if(prepareResp.error_code() == grpc::StatusCode::OK) { //if prepare() succeeds  
      // 5.1 rename 
      for(pair<string,string> temp_file_pair: rename_movs){
        // old name (tmp file), new name (original file)
        rename(temp_file_pair.first.c_str(), temp_file_pair.second.c_str());
      }
      dbgprintf("[INFO] Write: rename complete\n");
      // 5.2 WAL RPCinit
      wal->log_replication_init(txnId);
      
      // 5.3 Update KV store
      kvObj.UpdateStateOnKVStore(KV_STORE, txnId, RPC_INIT);

      // 5.4 call commit()
      Status commitResp = callCommit(txnId, pathData);
      dbgprintf("[INFO] Write: commitResp = %d\n", commitResp.error_code());
      // 5.5 if commit() succeeds:
      if (commitResp.error_code() == grpc::StatusCode::OK)
      {
        // 7.1 WAL commit
        wal->log_commit(txnId);
        // 7.2 Remove from KV store
        kvObj.DeleteFromKVStore(KV_STORE, txnId);
        // 7.3 Respond success
        return Status::OK;
      }
        // commit() fails: if backup is unavailable
      else if (commitResp.error_code() == grpc::StatusCode::UNAVAILABLE)
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
        // TODO: Undo changes
        // Return failure
        return grpc::Status(grpc::StatusCode::UNKNOWN, "failed to complete write operation\n"); // TODO: Check if this status code is appropriate
      }       
    }
    // if prepare() fails
    else{ 
      // 5.1 check status==Unavailable
      if (prepareResp.error_code() == grpc::StatusCode::UNAVAILABLE)
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
        // TODO: Delete temp and undo files
        // 6.3 Send failure status
        return grpc::Status(grpc::StatusCode::UNKNOWN, "failed to complete write operation\n"); // TODO: Check if this status code is appropriate
      }   
    }
    dbgprintf("[INFO] Write: Exiting function\n");
    return Status::OK;
  }

  Status callPrepare(string txnId, string buf, vector<PathData> pathData){
    dbgprintf("[INFO] callPrepare: Entering function\n");
    ClientContext context;
    PrepareRequest request;
    PrepareReply reply;
    FileData* fileData;

    request.set_transationid(txnId);
    request.set_buffer(buf);
    
    for(PathData pd: pathData){
      dbgprintf("[INFO] callPrepare: Iterating over pathData\n");
      fileData = request.add_file_data();
      fileData->set_file_name(pd.path);
      fileData->set_size(pd.size); 
      fileData->set_offset(pd.offset);
    }
    
    Status status = _stub->Prepare(&context, request, &reply);
    dbgprintf("[INFO] callPrepare: status code = %d\n", status.error_code());
    dbgprintf("[INFO] callPrepare: Exiting function\n");
    return status;
  }

  Status callCommit(string txnId, vector<PathData> pathData){
    dbgprintf("[INFO] callCommit: Entering function\n");
    ClientContext context;
    CommitRequest request;
    CommitReply reply;
    FileData* fileData;

    request.set_transationid(txnId);

    for(PathData pd: pathData){
      fileData = request.add_file_data();
      fileData->set_file_name(pd.path);
    }
    
    Status status = _stub->Commit(&context, request, &reply);
    dbgprintf("[INFO] callCommit: Commit status code: %d\n", status.error_code());
    dbgprintf("[INFO] callCommitL Exiting function\n");
    return status;
  }
  
};

void PrepareStorage() {
  
  #ifdef INFO
    cout << "[INFO] Preparing Storage Path" << endl;
  #endif

  int res = mkdir(SERVER_STORAGE_PATH, 0777);

  if (res == -1 && errno == EEXIST) {
    // #ifdef WARN
    //   cout << "[WARN] Server storage dir:" << SERVER_STORAGE_PATH << " already exists." << endl;
    // #endif
  }

  for(int dirId = 0; dirId < LEVEL_O_COUNT; dirId++) {

    // check if the directory is already is created
    string dir = string(SERVER_STORAGE_PATH) + "/" + to_string(dirId);

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

string convert_to_local_addr(string addr) {
  int colon = addr.find(":");
  return "0.0.0.0:" + addr.substr(colon+1); 
}

void *RunBlockStorageServer(void* _otherIP) {
  // Init Service
  // SERVER_1 = "0.0.0.0:" + std::to_string(port);
  string self_addr_lb_local = convert_to_local_addr(self_addr_lb);

  char* otherIP = (char*)_otherIP;
  dbgprintf("RunBlockStorageServer: otherIP = %s\n", otherIP);
  // std::string server_address("0.0.0.0:50051");
  BlockStorageServiceImpl service(otherIP);
  grpc::EnableDefaultHealthCheckService(true);
  grpc::reflection::InitProtoReflectionServerBuilderPlugin();
  ServerBuilder builder;
  builder.AddListeningPort(self_addr_lb_local, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);
  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cout << "[INFO] BlockStorage Server listening on " << self_addr_lb_local << std::endl;

  // TODO: Uncomment later -- Start Recovery
  dbgprintf("[INFO] Recover: Starting\n");
  CrashRecovery cr;
  int recover_rc = cr.Recover(service._stub);

  // Start Service
  if (recover_rc == 0) 
  {
    dbgprintf("[INFO] Recovery done. Starting server\n");
    server->Wait();
  }

  return NULL;
}

void *RunCommServer(void* _self_addr_peer) {
  // SERVER_2 = "20.109.180.121:";
  string self_addr_peer((char*)_self_addr_peer);
  ServiceCommImpl service;

  grpc::EnableDefaultHealthCheckService(true);
  grpc::reflection::InitProtoReflectionServerBuilderPlugin();
  ServerBuilder builder;
  builder.AddListeningPort(self_addr_peer, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);
  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cout << "[INFO] CommServer listening on " << self_addr_peer << std::endl;

  server->Wait();

  return NULL;
}

class ServiceCommClient {
private:
      unique_ptr<ServiceComm::Stub> stub_;
      
public:
      ServiceCommClient(std::shared_ptr<Channel> channel)
        : stub_(ServiceComm::NewStub(channel)) {}
      
      void stop_writes(std::shared_ptr<ClientReaderWriter<SyncRequest, SyncReply> > stream) {
          SyncRequest request;
          SyncReply reply;

          request.set_ip("0.0.0.0");
          request.set_command(SyncRequest_Commands_STOP_WRITE);
          
          stream->Write(request);
          cout << "[INFO] stop_writes: sent SyncRequest_Commands_STOP_WRITE request to Primary" << endl;
          
          stream->Read(&reply);
          cout << "[INFO] stop_writes: recv from primary:" << reply.error() << endl;
      }

      int wait(std::shared_ptr<ClientReaderWriter<SyncRequest, SyncReply> > stream) {
          SyncRequest request;
          SyncReply reply;

          request.set_ip("0.0.0.0");
          request.set_command(SyncRequest_Commands_WRITES_STATUS);

          while(1) {
            stream->Write(request);
            cout << "[INFO] stop_writes: sent SyncRequest_Commands_WRITES_STATUS request to Primary" << endl;
            
            stream->Read(&reply);

            int inflight_writes = reply.inflight_writes();
            int pending_writes = reply.pending_writes();

            cout << "[INFO] stop_writes: recv from primary:" << pending_writes  << "," << inflight_writes << endl;

            if (inflight_writes == 0) {
              return pending_writes; 
            }
          }
      }

      bool commit_txns(std::shared_ptr<ClientReaderWriter<SyncRequest, SyncReply> > stream, int txn_count) {
        SyncRequest request;
        SyncReply reply;

        request.set_ip("0.0.0.0");
        request.set_command(SyncRequest_Commands_GET_WRITES);

        
        stream->Write(request);
        cout << "[INFO] commit_txns: sent SyncRequest_Commands_WRITES_STATUS request to Primary" << endl;

        request.set_command(SyncRequest_Commands_ACK_PREV);

        for(int i = 0; i < txn_count; i++) {
            stream->Read(&reply);
            cout << "[INFO] commit_txns: recv reply for packet number- " << reply.error() << endl;
            
            

            stream->Write(request);
            cout << "[INFO] commit_txns: sent ack for packet number - " << reply.error() << endl;
        }
        
        return true;
      }

      void Sync() {
        ClientContext context;

        std::shared_ptr<ClientReaderWriter<SyncRequest, SyncReply> > stream(
            stub_->Sync(&context));

        // step - 1: stop writes at Primary
        stop_writes(stream);

        // step - 2: get the status of writes
        int txn_count = wait(stream);

        cout << "[INFO] Sync: starting syncing for txns:" << txn_count << endl;
        // step - 3: get all writes
        bool txn_status = commit_txns(stream, txn_count);

        stream->WritesDone();
        Status status = stream->Finish();

        if (!status.ok()) {
          cout << "[ERR] Sync: SYNC failed." << endl;
        } else {
          cout << "[INFO] Sync: SYNC worked" << endl;
        }
      }
};

class LBNodeCommClient {
private:
    unique_ptr<LBNodeComm::Stub> stub_;
    Identity identity;
    string self_addr;
  
public:
    LBNodeCommClient(string target_str, Identity _identity, string _self_addr) {
      identity = _identity;
      stub_ = LBNodeComm::NewStub(grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials()));
      self_addr = _self_addr;
    }

    void SendHeartBeat() {
        ClientContext context;

        std::shared_ptr<ClientReaderWriter<HeartBeatRequest, HeartBeatReply> > stream(
            stub_->SendHeartBeat(&context));

        HeartBeatReply reply;
        HeartBeatRequest request;
        request.set_ip(self_addr);

        while(1) {
          request.set_identity(identity);
          stream->Write(request);
          cout << "[INFO] SendHeartBeat: sent heartbeat" << endl;

          stream->Read(&reply);
          cout << "[INFO] SendHeartBeat: recv heartbeat response" << endl;
          cout << "[INFO] SendHeartBeat: has peer ? " << reply.has_peer_ip() << endl;

          if (identity == BACKUP && !reply.has_peer_ip()) {
            cout << "[INFO] SendHeartBeat: FAILOVER" << endl;
            identity = PRIMARY;
          }

          sleep(HB_SLEEP_IN_SEC);
        }
    }
};

void *Test(void* arg) {
  string target_str = "localhost:50052";
  ServiceCommClient serviceCommClient(grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials()));
  serviceCommClient.Sync();

  return NULL;
}

void *StartHB(void* _identity) {
  string identity_str((char*)_identity);

  cout << "[INFO] StartHB: starting as:" << identity_str << endl;

  Identity identity_enum = PRIMARY;

  if (identity_str.compare(BACKUP_STR) == 0) {
    identity_enum = BACKUP;
  }

  LBNodeCommClient lBNodeCommClient(lb_addr, identity_enum, self_addr_lb);
  lBNodeCommClient.SendHeartBeat();

  return NULL;
}

// ./blockstorage_server [identity] [self_addr_lb] [self_addr_peer] [peer_addr] [lb_addr] 
// ./blockstorage_server PRIMARY 20.124.236.11:40051 0.0.0.0:60052 0.0.0.0:70053 20.124.236.11:50056
// e.g ./blockstorage_server PRIMARY 52.151.53.152:40051 0.0.0.0:60052 20.109.180.121:60053 52.151.53.152:50056
// e.g ./blockstorage_server BACKUP 20.109.180.121:40052 0.0.0.0:60053 52.151.53.152:60052 52.151.53.152:50056

int main(int argc, char** argv) {
  self_addr_lb = string(argv[2]);
  lb_addr = string(argv[5]);

  PrepareStorage();
  // Write Ahead Logger
  wal = new WAL(SERVER_STORAGE_PATH);
  // global write semaphore
  sem_init(&global_write_lock, 0, 1);

  pthread_t block_server_t, comm_server_t;
  pthread_t test_t, hb_t;
  
  pthread_create(&block_server_t, NULL, RunBlockStorageServer, argv[4]);
  pthread_create(&comm_server_t, NULL, RunCommServer, argv[3]);
  // pthread_create(&test_t, NULL, Test, NULL);
  pthread_create(&hb_t, NULL, StartHB, argv[1]);

  pthread_join(block_server_t, NULL);
  pthread_join(comm_server_t, NULL);
  // pthread_join(test_t, NULL);
  pthread_join(hb_t, NULL);

  return 0;
}


