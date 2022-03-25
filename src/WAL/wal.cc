#include <iostream>
#include <fstream>
#include <vector>
#include <utility>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>

using namespace std;

// COMMANDS
string TXN_START = "TXN_START";
string TXN_END = "TXN_END";
string MV = "MV";

string delim = ":";

class WALBuilder;

class WAL {
    public:
        friend class WALBuilder;
        static WALBuilder make();

    private:
        WAL() = default;
        string log;
    
};

// #define PRINT
class WALBuilder {
    private:
        WAL wal;

    public:
        WALBuilder& txn_init() {
            wal.log += "TXN_START\n";
            return *this;
        }

        WALBuilder& move(string from_path, string to_path) {
            wal.log += "MV:" + from_path + ":" + to_path + "\n";
            return *this;
        }

        WALBuilder& txn_end() {
            wal.log += "TXN_END\n";
            return *this;
        }

        string to_string() {
            return wal.log;
        }
};

WALBuilder WAL::make()
{
    return WALBuilder();
}

void sampleLogUsage() {
    WALBuilder builder = WAL::make();

    builder.txn_init();
    builder.move("from1.txt", "end1.txt");
    builder.move("from2.txt", "end2.txt");
    builder.txn_end();

    string log = builder.to_string();
    cout << log << endl;
}

int main() {

    sampleLogUsage();

    string FILENAME = "/home/benitakbritto/hemal/CS-739-P3/src/WAL/self.log";
    ifstream file(FILENAME);

    if (file.is_open()) {
        vector<pair<string, string>> ops;

        string line;

        while (getline(file, line)) {
            int cmd_start = 0;
            int cmd_end = line.find(delim);

            string cmd = line.substr(cmd_start, cmd_end);
            // cout << cmd << endl;

            if (TXN_START.compare(cmd) == 0) {
                // init vector for move operation
                ops.clear();
            } else if (TXN_END.compare(cmd) == 0) {
                // execute move operation(s)
                cout << "Collected ops are:" << endl; 
                
                for(int i = 0; i < ops.size(); i++) {
                    cout << "Renaming [" << ops[i].first << "] to [" << ops[i].second << "]" << endl;

                    int res = rename(ops[i].first.c_str(), ops[i].second.c_str());

                    if (res == -1) {
                        if (errno == ENOENT) {
                            cout << "[WARN]: The file [" << ops[i].first << "] may have been moved already" << endl;
                            continue;
                        }

                        cout << "[ERROR] Unsupported error from rename. Check crash recovery log" << endl;
                        exit(-1);
                    }
                }

            } else if (MV.compare(cmd) == 0) {
                // append vectors for move operation
                int next_delim = line.find(delim, cmd_end+1);
                string from = line.substr(cmd_end + 1, next_delim - cmd_end - 1);
                string to = line.substr(next_delim + 1);

                // cout << from << endl;
                // cout << to << endl;

                ops.push_back(make_pair(from, to));
            } else {
                cout << "[WARN]: Unsupported Command. Log file is corrupted" << endl;
            }
        }

        file.close();
    }

    return 0;
}