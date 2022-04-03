#ifndef KV_H
#define KV_H

#include <iostream>
#include <map>
#include <string>
#include <vector>
#include "state.h"
#include <semaphore.h>
#include "common.h"

/******************************************************************************
 * NAMESPACE
 *****************************************************************************/
using namespace std;

/******************************************************************************
 * GLOBALS
 *****************************************************************************/
struct TransactionData
{
    vector<string> original_files;
    vector<int> sizes;
    vector<int> offsets;
    int state;
};
typedef TransactionData TxnData;
extern sem_t global_kv_store_lock;

/******************************************************************************
 * DECLARATION
 *****************************************************************************/
class KVStore
{
public:
    KVStore() {
        sem_init(&global_kv_store_lock, 0, 1);
    }

    void AddToKVStore(map<string, TxnData> &KV_STORE, 
                    string txn_id, 
                    vector<string> &original_files,
                    vector<int> &sizes,
                    vector<int> &offsets);
    int GetStateFromKVStore(map<string, TxnData> &KV_STORE, string txn_id);
    void UpdateStateOnKVStore(map<string, TxnData> &KV_STORE, string txn_id, int state);
    void DeleteFromKVStore(map<string, TxnData> &KV_STORE, string txn_id);
    void GetTransactionDataFromKVStore(map<string, TxnData> &KV_STORE, 
                                            string txn_id,
                                            vector<string> &original_files,
                                            vector<int> &sizes,
                                            vector<int> &offsets);
};

#endif
