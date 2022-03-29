#include "kv_store.h"

int KVStore::GetStateFromKVStore(map<string, int> &KV_STORE, string txn_id)
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
        return KV_STORE[txn_id];
    }
    dbgprintf("GetStateFromKVStore: Exiting function\n");
}

void KVStore::UpdateStateOnKVStore(map<string, int> &KV_STORE, string txn_id, int state)
{
    dbgprintf("UpdateStateOnKVStore: Entering function\n");
    KV_STORE[txn_id] = state;
    dbgprintf("UpdateStateOnKVStore: Exiting function\n");
}

void KVStore::DeleteFromKVStore(map<string, int> &KV_STORE, string txn_id)
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

//     map<string, int> KV_STORE;
//     kv_store.GetStateFromKVStore(KV_STORE, "1");

//     kv_store.UpdateStateOnKVStore(KV_STORE, "1", 1); 
//     printf("State = %d\n", kv_store.GetStateFromKVStore(KV_STORE, "1"));  

//     kv_store.DeleteFromKVStore(KV_STORE, "1");
//     kv_store.GetStateFromKVStore(KV_STORE, "1");

//     return 0;
// }
