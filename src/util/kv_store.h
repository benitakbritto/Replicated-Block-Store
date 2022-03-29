#ifndef KV_H
#define KV_H

#include <iostream>
#include <map>
#include <string>

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


class KVStore
{
public:
    KVStore() {}

    int GetStateFromKVStore(map<string, int> &KV_STORE, string txn_id);
    void UpdateStateOnKVStore(map<string, int> &KV_STORE, string txn_id, int state);
    void DeleteFromKVStore(map<string, int> &KV_STORE, string txn_id);
};

#endif
