#ifndef CR_H
#define CR_H

#include <iostream>
#include <vector>
#include <string>

/******************************************************************************
 * NAMESPACE
 *****************************************************************************/
using namespace std;

/******************************************************************************
 * GLOBALS
 *****************************************************************************/
enum State 
{
    START = 0,
    ABORT = 1,
    RPC_INIT = 2,
    COMMIT = 3
};

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
#define STATE_START                 "TXN_START"
#define STATE_ABORT                 "ABORT"
#define STATE_RPC_INIT              "REPL_INIT"
#define STATE_COMMIT                "COMMIT"
#define LOG_FILE_PATH               "/home/benitakbritto/CS-739-P3/src/WAL/self.log"
#define DEBUG                       1                     
#define dbgprintf(...)              if (DEBUG) { printf(__VA_ARGS__); } 
#define DELIM                       ":"    
#define OPERATION_MOVE              "MV"       

class CrashRecovery
{
public:
    CrashRecovery() {}
    int Recover();
    
};

#endif