#include "kv_store.h"

sem_t global_kv_store_lock;

// Set the state to START
// Add the list of original file names
void SetTxnData(TxnData * data, 
                vector<string> &original_files,
                vector<int> &sizes,
                vector<int> &offsets)
{
    data->original_files.insert(data->original_files.begin(), 
                                original_files.begin(),
                                original_files.end());
    data->sizes.insert(data->sizes.begin(),
                    sizes.begin(),
                    sizes.end());
    data->offsets.insert(data->offsets.begin(),
                    offsets.begin(),
                    offsets.end());
    data->state = START;
}


void KVStore::AddToKVStore(map<string, TxnData> &KV_STORE, 
                            string txn_id, 
                            vector<string> &original_files,
                            vector<int> &sizes,
                            vector<int> &offsets)
{
    dbgprintf("AddToKVStore: Entering function\n");
    sem_wait(&global_kv_store_lock);
    if (KV_STORE.count(txn_id) != 0)
    {
        dbgprintf("AddToKVStore: Duplicate key!\n");
    }
    else
    {
        TxnData data;
        SetTxnData(&data, original_files, sizes, offsets);
        KV_STORE[txn_id] = data;
    }
    sem_post(&global_kv_store_lock);
    dbgprintf("AddToKVStore: Exiting function\n");
}


int KVStore::GetStateFromKVStore(map<string, TxnData> &KV_STORE, string txn_id)
{
    dbgprintf("GetStateFromKVStore: Entering function\n");
    sem_wait(&global_kv_store_lock);
    if (KV_STORE.count(txn_id) == 0)
    {
        dbgprintf("GetStateFromKVStore: txn_id does not exist\n");
        dbgprintf("GetStateFromKVStore: Exiting function\n");
        sem_post(&global_kv_store_lock);
        return -1;
    }
    else
    {
        dbgprintf("GetStateFromKVStore: Exiting function\n");
        sem_post(&global_kv_store_lock);
        return KV_STORE[txn_id].state;
    }
}

void KVStore::UpdateStateOnKVStore(map<string, TxnData> &KV_STORE, string txn_id, int state)
{
    dbgprintf("UpdateStateOnKVStore: Entering function\n");
    sem_wait(&global_kv_store_lock);
    KV_STORE[txn_id].state = state;
    sem_post(&global_kv_store_lock);
    dbgprintf("UpdateStateOnKVStore: Exiting function\n");
}

void KVStore::DeleteFromKVStore(map<string, TxnData> &KV_STORE, string txn_id)
{
    dbgprintf("DeleteFromKVStore: Entering function\n");
    sem_wait(&global_kv_store_lock);
    if (KV_STORE.count(txn_id) == 0)
    {
        dbgprintf("GetStateFromKVStore: txn_id does not exist\n");
        dbgprintf("DeleteFromKVStore: Exiting function\n");
        sem_post(&global_kv_store_lock);
        return;
    }
    else
    {
        KV_STORE.erase(txn_id);
    }
    sem_post(&global_kv_store_lock);
    dbgprintf("DeleteFromKVStore: Exiting function\n");
}

void KVStore::GetTransactionDataFromKVStore(map<string, TxnData> &KV_STORE, 
                                            string txn_id,
                                            vector<string> &original_files,
                                            vector<int> &sizes,
                                            vector<int> &offsets)
{
    dbgprintf("GetTransactionDataFromKVStore: Entering function\n");
    sem_wait(&global_kv_store_lock);
    original_files.insert(original_files.begin(), 
                        KV_STORE[txn_id].original_files.begin(),
                        KV_STORE[txn_id].original_files.end());
    sizes.insert(sizes.begin(), 
                        KV_STORE[txn_id].sizes.begin(),
                        KV_STORE[txn_id].sizes.end());
    offsets.insert(offsets.begin(), 
                        KV_STORE[txn_id].offsets.begin(),
                        KV_STORE[txn_id].offsets.end());
    sem_post(&global_kv_store_lock);
    dbgprintf("GetTransactionDataFromKVStore: Exiting function\n");
}



// Tester
// int main()
// {
//     KVStore kv_store;
//     map<string, TxnData> kv;
//     vector<string> files;
//     vector<int> sizes;
//     vector<int> offsets;
//     files.push_back("a.txt");
//     files.push_back("b.txt");
//     sizes.push_back(1);
//     sizes.push_back(2);
//     offsets.push_back(100);
//     offsets.push_back(200);
//     kv_store.AddToKVStore(kv, "1", files, sizes, offsets);
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
