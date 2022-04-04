#ifndef TXN_H
#define TXN_H

#include <iostream>
#include <vector>
#include "common.h"
#include <utility>
#include <semaphore.h>
#include <thread>

/******************************************************************************
 * NAMESPACES
 *****************************************************************************/
using namespace std;

/******************************************************************************
 * DECLARATION
 *****************************************************************************/
class Txn {
    private:
        string status;
        vector<pair<string, string>> ops;
    
    public:
        Txn();
        void set_status(string new_status);
        string get_status();
        void set_ops(vector<pair<string, string>> ops);
        vector<pair<string, string>> get_ops();
};

#endif
