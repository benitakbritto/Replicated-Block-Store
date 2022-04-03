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
#include "common.h"
#include <map>
#include <fstream>
#include <chrono>

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
using blockstorage::GetTransactionStateRequest;
using blockstorage::GetTransactionStateReply;
using blockstorage::CommitRequest;
using blockstorage::CommitReply;
using grpc::ClientReader;
using namespace chrono;

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
#define LOG_FILE_PATH               "/home/benitakbritto/CS-739-P3/storage/self.log"
#define DELIM                       ":"    
#define OPERATION_MOVE              "MV"
#define TIMER_ON                    0       

class CrashRecovery
{
public:
    CrashRecovery() {}
    int Recover(unique_ptr<ServiceComm::Stub> &_stub);
};

#endif