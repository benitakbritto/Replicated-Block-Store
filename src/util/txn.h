#include <iostream>
#include <vector>

using namespace std;

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