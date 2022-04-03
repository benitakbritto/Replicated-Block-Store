#include "txn.h"

Txn::Txn() {
    this->status = "";
}

void Txn::set_status(string new_status) {
    this->status = new_status;
}

string Txn::get_status() {
    return this->status;
}

void Txn::set_ops(vector<pair<string, string>> ops) {
    this->ops.assign(ops.begin(), ops.end());
}

vector<pair<string, string>> Txn::get_ops() {
    return this->ops;
}

// Tester
// int main() {
//     Txn txn;

//     txn.set_status("ST1");
//     dbgprintf("%s\n", txn.get_status().c_str());

//     vector<pair<string, string>> ops;
//     ops.push_back(make_pair("f1", "t1"));
//     ops.push_back(make_pair("f2", "t2"));

//     txn.set_ops(ops);

//     vector<pair<string, string>> ops_from_class = txn.get_ops();
//     dbgprintf("%s\n", ops_from_class[0].first.c_str());
//     dbgprintf("%s\n", ops_from_class[1].first.c_str());

//     sem_t lock;

//     sem_init(&lock, 0, 2);

//     int val = 100;  
//     dbgprintf("%d\n", sem_getvalue(&lock, &val));
//     dbgprintf("%d\n", val);

//     return 0;
// }