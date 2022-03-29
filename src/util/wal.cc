#include "wal.h"

WAL::WAL(string base_path) {
    string log_file_path = base_path + "self.log";
    fd = open(log_file_path.c_str(), O_WRONLY | O_APPEND | O_CREAT, 0666);

    if (fd == -1) {
        throw runtime_error("[ERROR]: Could not create log file");
    }

    cout << "[INFO]: Init logging at:" << log_file_path << endl;
    sem_init(&lock, 0, 1);
}

/**
 * @brief 
 * 
 * @param txn_id 
 * @param rename_movs should be in <temp_file_path, original_file_path> form
 * @return int 
 */
int WAL::log_prepare(string txn_id, vector<pair<string, string>> rename_movs) {
    if (rename_movs.empty()) {
        throw runtime_error("[ERROR]: rename_movs param cannot be empty");
    }

    sem_wait(&lock);

    string log = txn_id + DELIM + TXN_START + "\n";

    for(auto paths: rename_movs) {
        log += txn_id + DELIM + MV + DELIM + paths.first + DELIM + paths.second + "\n"; 
    }

    int res = write(fd, log.c_str(), log.size());

    if (res == -1) {
        cout << "[ERROR]: PREPARE - failed to write " << log << endl; 
        cout << "[ERROR]:" << strerror(errno) << endl;
    } else {
        fsync(fd);
    }

    sem_post(&lock);
    return res;
}

int WAL::log_abort(string txn_id) {
    sem_wait(&lock);

    string log = txn_id + DELIM + ABORT + "\n";
    int res = write(fd, log.c_str(), log.size());

    if (res == -1) {
        cout << "[ERROR]: ABORT - failed to write " << log << endl; 
        cout << "[ERROR]:" << strerror(errno) << endl;
    } else {
        fsync(fd);
    }

    sem_post(&lock);
    return res;
}

int WAL::log_commit(string txn_id) {
    sem_wait(&lock);

    string log = txn_id + DELIM + COMMIT + "\n";
    int res = write(fd, log.c_str(), log.size());
    
    if (res == -1) {
        cout << "[ERROR]: COMMIT - failed to write " << log << endl; 
        cout << "[ERROR]:" << strerror(errno) << endl;
    } else {
        fsync(fd);
    }

    sem_post(&lock);
    return res;
}

int WAL::log_replication_init(string txn_id) {
    sem_wait(&lock);

    string log = txn_id + DELIM + REPL_INIT + "\n";
    int res = write(fd, log.c_str(), log.size());

    if (res == -1) {
        cout << "[ERROR]: REPL_INIT - failed to write " << log << endl; 
        cout << "[ERROR]:" << strerror(errno) << endl;
    } else {
        fsync(fd);
    }

    sem_post(&lock);
    return res;
}

const string WAL::DELIM = ":";

// COMMANDS
const string WAL::TXN_START = "TXN_START";
const string WAL::MV = "MV";
const string WAL::COMMIT = "COMMIT";
const string WAL::ABORT = "ABORT";
const string WAL::REPL_INIT = "REPL_INIT";