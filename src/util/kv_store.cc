#include "kv_store.h"

// Set the state to START
// Add the list of original file names
void SetTxnData(TxnData * data, vector<string> &original_files)
{
    data->original_files.insert(data->original_files.begin(), 
                                original_files.begin(),
                                original_files.end());
    data->state = START;
}


void KVStore::AddToKVStore(map<string, TxnData> &KV_STORE, string txn_id, vector<string> &original_files)
{
    dbgprintf("AddToKVStore: Entering function\n");
    if (KV_STORE.count(txn_id) != 0)
    {
        dbgprintf("AddToKVStore: Duplicate key!\n");
    }
    else
    {
        TxnData data;
        SetTxnData(&data, original_files);
        KV_STORE[txn_id] = data;
    }
    dbgprintf("AddToKVStore: Exiting function\n");
}


int KVStore::GetStateFromKVStore(map<string, TxnData> &KV_STORE, string txn_id)
{
    dbgprintf("GetStateFromKVStore: Entering function\n");
    if (KV_STORE.count(txn_id) == 0)
    {
        dbgprintf("GetStateFromKVStore: txn_id does not exist\n");
        dbgprintf("GetStateFromKVStore: Exiting function\n");
        return -1;
    }
    else
    {
        dbgprintf("GetStateFromKVStore: Exiting function\n");
        return KV_STORE[txn_id].state;
    }
    dbgprintf("GetStateFromKVStore: Exiting function\n");
}

void KVStore::UpdateStateOnKVStore(map<string, TxnData> &KV_STORE, string txn_id, int state)
{
    dbgprintf("UpdateStateOnKVStore: Entering function\n");
    KV_STORE[txn_id].state = state;
    dbgprintf("UpdateStateOnKVStore: Exiting function\n");
}

void KVStore::DeleteFromKVStore(map<string, TxnData> &KV_STORE, string txn_id)
{
    dbgprintf("DeleteFromKVStore: Entering function\n");
    if (KV_STORE.count(txn_id) == 0)
    {
        dbgprintf("GetStateFromKVStore: txn_id does not exist\n");
        dbgprintf("DeleteFromKVStore: Exiting function\n");
        return;
    }
    else
    {
        KV_STORE.erase(txn_id);
    }
    dbgprintf("DeleteFromKVStore: Exiting function\n");
}


// Tester
// int main()
// {
//     KVStore kv_store;
//     map<string, TxnData> kv;
//     vector<string> files;
//     files.push_back("a.txt");
//     files.push_back("b.txt");
    
//     kv_store.AddToKVStore(kv, "1", files);
//     kv_store.GetStateFromKVStore(kv, "1");

//     printf("State = %d\n", kv_store.GetStateFromKVStore(kv, "1"));  
//     for (auto file : kv["1"].original_files)
//     {
//         printf("File = %s\n", file.c_str());  
//     }

//     // kv_store.UpdateStateOnKVStore(kv, "1", 1); 
//     // printf("State = %d\n", kv_store.GetStateFromKVStore(kv, "1")); 
//     // kv_store.DeleteFromKVStore(kv, "1");
//     // kv_store.GetStateFromKVStore(kv, "1");

//     return 0;
// }
