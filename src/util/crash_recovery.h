#ifndef CR_H
#define CR_H

#include <iostream>
#include <vector>
#include <string>
#include "state.h"
#include <grpcpp/grpcpp.h>
#include "blockstorage.grpc.pb.h"
#include "servercomm.grpc.pb.h"
#include <fcntl.h>

/******************************************************************************
 * NAMESPACE
 *****************************************************************************/
using namespace std;
using namespace blockstorage;
using grpc::ClientContext;
using grpc::Status;
using blockstorage::ServiceComm;
using blockstorage::GetPendingReplicationTransactionsRequest;
using blockstorage::GetPendingReplicationTransactionsReply;
using blockstorage::ForcePendingWritesRequest;
using blockstorage::ForcePendingWritesReply;
using grpc::ClientReader;

/******************************************************************************
 * GLOBALS
 *****************************************************************************/
enum Operation
{
    MOVE
};

struct Command
{
    int op;
    vector<string> file_names;
};

typedef struct Command Cmd;

struct LogData 
{
    Cmd cmd;
    int state;
};

typedef LogData Data;

/******************************************************************************
 * MACROS
 *****************************************************************************/
#define LOG_FILE_PATH               "/home/benitakbritto/CS-739-P3/src/self.log" // TODO: Check this
#define DEBUG                       1                     
#define dbgprintf(...)              if (DEBUG) { printf(__VA_ARGS__); } 
#define DELIM                       ":"    
#define OPERATION_MOVE              "MV"       

class CrashRecovery
{
public:
    CrashRecovery() {}
    int Recover(unique_ptr<ServiceComm::Stub> &_stub);
};

#endif