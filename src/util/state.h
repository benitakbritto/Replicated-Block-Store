#ifndef STATE_H
#define STATE_H

/******************************************************************************
 * GLOBALS
 *****************************************************************************/
enum State 
{
    START = 0,
    ABORT = 1,
    RPC_INIT = 2,
    COMMIT = 3,
    PENDING_REPLICATION = 4
};

/******************************************************************************
 * MACROS
 *****************************************************************************/
#define STATE_START                 "TXN_START"
#define STATE_ABORT                 "ABORT"
#define STATE_RPC_INIT              "REPL_INIT"
#define STATE_COMMIT                "COMMIT"
#define STATE_PENDING_REPLICATION   "PENDING_REPLICATION"

#endif
