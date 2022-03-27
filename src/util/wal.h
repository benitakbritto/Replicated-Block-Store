#include <iostream>
#include <fcntl.h>
#include <vector>
#include <errno.h>
#include <stdio.h>
#include <threads.h>
#include <semaphore.h>
#include <unistd.h>
#include <cstring>

using namespace std;

class WAL {
    private:
        string log_file_path;
        int fd;
        sem_t lock;

        static const string DELIM;

        // COMMANDS
        static const string TXN_START;
        static const string MV;
        static const string COMMIT;
        static const string ABORT;
        static const string REPL_INIT;
    
    public:
        /**
         * @brief Construct a new WAL object
         * 
         * @param base_path - base path should have a path in dir format, i.e. ending with "/"
         */
        WAL(string base_path);

        /**
         * @brief 
         * 
         * @param txn_id 
         * @param rename_movs should be in <temp_file_path, original_file_path> form
         * @return int 
         */
        int log_prepare(string txn_id, vector<pair<string, string>> rename_movs);

        int log_abort(string txn_id);

        int log_commit(string txn_id);

        int log_replication_init(string txn_id);
};