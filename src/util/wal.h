#ifndef WAL_H
#define WAL_H

#include <iostream>
#include <fcntl.h>
#include <vector>
#include <errno.h>
#include <stdio.h>
#include <threads.h>
#include <semaphore.h>
#include <unistd.h>
#include <cstring>
#include "state.h"
#include "wal.h"
#include "common.h"

/******************************************************************************
 * NAMESPACES
 *****************************************************************************/
using namespace std;

/******************************************************************************
 * DECLARATION
 *****************************************************************************/
class WAL {
    private:
        string log_file_path;
        int fd;
        sem_t lock;

        static const string DELIM;

        // COMMANDS
        static const string MV;
       
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

        int log_pending_replication(string txn_id);
};

#endif