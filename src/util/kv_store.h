#ifndef KV_H
#define KV_H

#include <iostream>
#include <map>
#include <string>
#include <vector>
#include "state.h"

/******************************************************************************
 * MACROS
 *****************************************************************************/
#define DEBUG                       1                     
#define dbgprintf(...)              if (DEBUG) { printf(__VA_ARGS__); } 

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

class KVStore
{
public:
    KVStore() {}

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
