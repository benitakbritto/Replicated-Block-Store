#include "crash_recovery.h"
#include <unordered_map>
#include <fstream>

/******************************************************************************
 * NAMESPACE
 *****************************************************************************/
using namespace std;

/******************************************************************************
 * GLOBALS
 *****************************************************************************/
map<string, Data> logMap;

/******************************************************************************
 * HELPER FUNCTIONS
 *****************************************************************************/
void LoadData();
void ExecuteTransactionStartRecovery(string id);
void ExecuteTransactionAbortRecovery(string id);
void ExecuteTransactionRpcInitRecovery(string id);
void ExecuteTransactionCommitRecovery(string id);
void ExecuteTransactionPendingReplicationRecovery(string id);
void DeleteFiles(vector<string> file_names);
bool IsState(string val);
int GetStateOfCurrentServer(string val);
int GetOperation(string op);
void PrintLogData(string id);
string GetUndoFileName(string file_name);
void WriteData(string file_path, string content, int size, int offset);

// Tester
// int main()
// {
//     CrashRecovery cr;
//     cr.Recover();
//     PrintLogData("1");
//     return 0;
// }

int CrashRecovery::Recover(unique_ptr<ServiceComm::Stub> &_stub)
{
    dbgprintf("Recover: Entering function\n");
    // Parse log
    LoadData();

    // TODO: Test - getting status as unavailable
    // Get pending writes
    ClientContext context_gprt;
    GetPendingReplicationTransactionsRequest request_gprt;
    GetPendingReplicationTransactionsReply reply_gprt;
    Status status = _stub->GetPendingReplicationTransactions(&context_gprt, request_gprt, &reply_gprt);
    dbgprintf("Recover: GetPendingReplicationTransactions status code = %d\n", status.error_code());

    // TODO: Test
    // Go through RPC result and prune the list
    ForcePendingWritesRequest request_fpw;
    for (int i = 0; i < reply_gprt.txn_size(); i++)
    {
        string txnId = reply_gprt.txn(i).transaction_id();
        string("Recover: txnId = %s\n", txnId.c_str());

        // Transaction was not commited on this machine
        if ((logMap.count(txnId) != 0 
                && logMap[txnId].state != COMMIT)
            || 
            (logMap.count(txnId) == 0)) 
        {
            auto data = request_fpw.add_txn();
            data->set_transaction_id(txnId);
            // Set the state to commit
            // as we will be forcing the write in the next step
            if (logMap.count(txnId) != 0) logMap[txnId].state = COMMIT;
        }
    }

    // TODO: Test
    // Apply pending writes
    ClientContext context_fpw;
    ForcePendingWritesReply reply_fpw;
    std::unique_ptr<ClientReader<ForcePendingWritesReply>> reader(
                            _stub->ForcePendingWrites(&context_fpw, request_fpw));
    while (reader->Read(&reply_fpw))
    {
        string transaction_id = reply_fpw.transaction_id();
        string file_name = reply_fpw.file_name();
        string content = reply_fpw.content();
        int offset = reply_fpw.offset();
        int size = reply_fpw.size();
        WriteData(file_name, content, size, offset);
    }
    status = reader->Finish();
    dbgprintf("Recover: ForcePendingWritesReply status code = %d\n", status.error_code());


    // Recover from other states    
    for (auto it = logMap.begin(); it != logMap.end(); it++)
    {
        dbgprintf("Recover: Transation id = %s\n", it->first.c_str());
        switch(it->second.state)
        {
            case START:
                ExecuteTransactionStartRecovery(it->first);
                break;
            case ABORT:
                ExecuteTransactionAbortRecovery(it->first);
                break;
            case RPC_INIT:
                ExecuteTransactionRpcInitRecovery(it->first);
                break;
            case COMMIT:
                ExecuteTransactionCommitRecovery(it->first);
                break;
            case PENDING_REPLICATION:
                ExecuteTransactionPendingReplicationRecovery(it->first);
            default:
                dbgprintf("Recover: Error: Invalid state\n");
                return -1;
        }
    }

    // TODECIDE
    // Truncate log?
    // Delete logMap ?

    dbgprintf("Recover: Exiting function\n");
    return 0;
}

void LoadData()
{
    dbgprintf("LoadData: Entering function\n");
    ifstream file(LOG_FILE_PATH);
    if (file.is_open())
    {
        string line;
        while (getline(file, line))
        {
            int id_start = 0;
            int id_end = 0;  
            int file_start = 0;
            int file_end = 0;
            int val_start = 0;
            int val_end = 0;
            string id = "";
            string op = "";
            string state = "";
            string file_name = "";
            string val = "";
            Data data; 
            Cmd cmd;
            
            // get id
            id_start = 0;
            id_end = line.find(DELIM);
            id = line.substr(id_start, id_end);
            dbgprintf("LoadData: id = %s\n", id.c_str());

            // get val (can be either state or op)
            val_start = id_end + 1;
            val_end = line.find(DELIM, val_start);
            val = line.substr(val_start, val_end - id_end - 1);
            dbgprintf("LoadData: val = %s\n", val.c_str());

            if (IsState(val))
            {
                dbgprintf("LoadData: State!\n");
                state = val;
                data.state = GetStateOfCurrentServer(state); 
                dbgprintf("LoadData: data.state = %d\n", data.state);

                // Update state in map
                if (logMap.count(id) == 0)
                {
                    logMap[id] = data;
                }
                else
                {
                    logMap[id].state = data.state;
                }            
            }
            else
            {
                dbgprintf("LoadData: Operation!\n");
                op = val;
                cmd.op = GetOperation(op);

                // file 1 
                file_start = val_end + 1;
                file_end = line.find(DELIM, file_start);
                file_name = line.substr(file_start, file_end - val_end - 1);
                dbgprintf("LoadData: file_name = %s\n", file_name.c_str());
                cmd.file_names.push_back(file_name);

                // file 2
                file_start = file_end + 1;
                file_end = line.find(DELIM, file_start);  
                file_name = line.substr(file_start, file_end - file_start - 2); // TO CHECK
                dbgprintf("LoadData: file_name = %s\n", file_name.c_str());
                cmd.file_names.push_back(file_name);  

                // Update state in map
                logMap[id].cmd.op = cmd.op;
                logMap[id].cmd.file_names.insert(logMap[id].cmd.file_names.end(),
                                                cmd.file_names.begin(),
                                                cmd.file_names.end());          
            } 
        }
    }
    dbgprintf("LoadData: Exiting function\n");
}

bool IsState(string val)
{
    return (val.compare(STATE_START) == 0 ||
            val.compare(STATE_ABORT) == 0 ||
            val.compare(STATE_RPC_INIT) == 0 ||
            val.compare(STATE_COMMIT) == 0);
}

int GetStateOfCurrentServer(string val)
{
    if (val.compare(STATE_START) == 0) return START;
    else if (val.compare(STATE_ABORT) == 0) return ABORT;
    else if (val.compare(STATE_RPC_INIT) == 0) return RPC_INIT;
    else if (val.compare(STATE_COMMIT) == 0) return COMMIT;
    return -1;
}

int GetOperation(string op)
{
    if (op.compare(OPERATION_MOVE) == 0) return MOVE;
    return -1;
}

// For debug
void PrintLogData(string id)
{
    cout << "***** TXN ******" << endl;
    cout << "Id: " << id << endl;
    cout << "Command op: " << logMap[id].cmd.op << endl;
    for (auto file_name : logMap[id].cmd.file_names)
    {
        cout << "Command file name = " << file_name << endl;
    }
    cout << "State: " << logMap[id].state << endl;
    cout << "***** TXN ******" << endl;
}

// TODO Check this
string GetUndoFileName(string file_name)
{
    return file_name + ".undo";
}

void ExecuteTransactionStartRecovery(string id)
{
    dbgprintf("ExecuteTransactionStartRecovery: Entering function\n");

    // delete tmp and undo files
    vector <string> files_to_delete;

    int len = logMap[id].cmd.file_names.size();
    for (int i = 0; i < len; i++)
    {
        // add tmp files to the list
        if (i % 2 == 0)
            files_to_delete.push_back(logMap[id].cmd.file_names[i]);
        // add undo files to the list
        else
            files_to_delete.push_back(GetUndoFileName(logMap[id].cmd.file_names[i]));
    }
    
    DeleteFiles(files_to_delete);

    dbgprintf("ExecuteTransactionStartRecovery: Exiting function\n");
}

void ExecuteTransactionAbortRecovery(string id)
{
    dbgprintf("ExecuteTransactionAbortRecovery: Entering function\n");

    // delete tmp and undo files
    vector <string> files_to_delete;

    int len = logMap[id].cmd.file_names.size();
    for (int i = 0; i < len; i++)
    {
        // add tmp files to the list
        if (i % 2 == 0)
            files_to_delete.push_back(logMap[id].cmd.file_names[i]);
        // add undo files to the list
        else
            files_to_delete.push_back(GetUndoFileName(logMap[id].cmd.file_names[i]));
    }
    
    DeleteFiles(files_to_delete);

    dbgprintf("ExecuteTransactionAbortRecovery: Exiting function\n");
}

// TODO
void ExecuteTransactionRpcInitRecovery(string id)
{
    dbgprintf("ExecuteTransactionRpcInitRecovery: Entering function\n");

    // Get state of txn id from other server

    // If state was commited on other server - del undo file

    // else force write on other server

    dbgprintf("ExecuteTransactionRpcInitRecovery: Exiting function\n");
}

void ExecuteTransactionCommitRecovery(string id)
{
    dbgprintf("ExecuteTransactionCommitRecovery: Entering function\n");

    // delete undo files
    vector <string> files_to_delete;

    int len = logMap[id].cmd.file_names.size();
    for (int i = 0; i < len; i++)
    {
        // add undo files to the list
        if (i % 2 != 0)
            files_to_delete.push_back(GetUndoFileName(logMap[id].cmd.file_names[i]));            
    }
    
    DeleteFiles(files_to_delete);

    dbgprintf("ExecuteTransactionCommitRecovery: Exiting function\n");
}

void ExecuteTransactionPendingReplicationRecovery(string id)
{
    dbgprintf("ExecuteTransactionPendingReplicationRecovery: Entering function\n");
    // Do nothing for now
    dbgprintf("ExecuteTransactionPendingReplicationRecovery: Exiting function\n");
}

void DeleteFiles(vector<string> file_names)
{
    for (auto file_name : file_names)
    {
        if (remove(file_name.c_str()) != 0)
        {
            dbgprintf("DeleteFiles: Failed to delete %s\n", file_name.c_str());
        }
    }
}

// TODO: Test
// TODECIDE if we should write to temp and rename instead
void WriteData(string file_path, string content, int size, int offset)
{
    dbgprintf("WriteData: Entering function\n");
    int fd = open(file_path.c_str(), O_WRONLY);
    if (fd == -1)
    {
      dbgprintf("WriteData: failed to open file\n");
      return;
    }

    int pwrite_rc = pwrite(fd, content.c_str(), size, offset);
    if (pwrite_rc == -1)
    {
      dbgprintf("WriteData: pread failed\n");
      close(fd);
      return;
    }
    dbgprintf("WriteData: Exiting function\n");
    close(fd);
    return;
}
