#include "wal.h"

WAL::WAL(string base_path) {
    string log_file_path = base_path + "self.log";
    fd = open(log_file_path.c_str(), O_WRONLY | O_APPEND | O_CREAT, 0666);

    if (fd == -1) {
       	cout << errno << endl;
	throw runtime_error("[ERROR]: Could not create log file");
    }

    dbgprintf("[INFO]: Init logging at: %s\n", log_file_path.c_str());
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

    string log = txn_id + DELIM + STATE_START + "\n";

    for(auto paths: rename_movs) {
        log += txn_id + DELIM + MV + DELIM + paths.first + DELIM + paths.second + "\n"; 
    }

    int res = write(fd, log.c_str(), log.size());

    if (res == -1) {
        dbgprintf("[ERROR]: PREPARE - failed to write %s\n", log.c_str());
        dbgprintf("[ERROR]: %s\n", strerror(errno));
    } else {
        fsync(fd);
    }

    sem_post(&lock);
    return res;
}

int WAL::log_abort(string txn_id) {
    sem_wait(&lock);

    string log = txn_id + DELIM + STATE_ABORT + "\n";
    int res = write(fd, log.c_str(), log.size());

    if (res == -1) {
        dbgprintf("[ERROR]: ABORT - failed to write %s\n", log.c_str());
        dbgprintf("[ERROR]: %s\n", strerror(errno));
    } else {
        fsync(fd);
    }

    sem_post(&lock);
    return res;
}

int WAL::log_commit(string txn_id) {
    sem_wait(&lock);

    string log = txn_id + DELIM + STATE_COMMIT + "\n";
    int res = write(fd, log.c_str(), log.size());
    
    if (res == -1) {
        dbgprintf("[ERROR]: COMMIT - failed to write %s\n", log.c_str());
        dbgprintf("[ERROR]: %s\n", strerror(errno));
    } else {
        fsync(fd);
    }

    sem_post(&lock);
    return res;
}

int WAL::log_replication_init(string txn_id) {
    sem_wait(&lock);

    string log = txn_id + DELIM + STATE_RPC_INIT + "\n";
    int res = write(fd, log.c_str(), log.size());

    if (res == -1) {
        dbgprintf("[ERROR]: REPLT_INIT - failed to write %s\n", log.c_str());
        dbgprintf("[ERROR]: %s\n", strerror(errno));
    } else {
        fsync(fd);
    }

    sem_post(&lock);
    return res;
}

int WAL::log_pending_replication(string txn_id) {
    sem_wait(&lock);

    string log = txn_id + DELIM + STATE_PENDING_REPLICATION + "\n";
    int res = write(fd, log.c_str(), log.size());

    if (res == -1) {
        dbgprintf("[ERROR]: PENDING_REPLICATION - failed to write %s\n", log.c_str());
        dbgprintf("[ERROR]: %s\n", strerror(errno));
    } else {
        fsync(fd);
    }

    sem_post(&lock);
    return res;
}

const string WAL::DELIM = ":";

// COMMANDS
const string WAL::MV = "MV";

// Tester
// int main()
// {
//     WAL wal("/home/benitakbritto/CS-739-P3/src/WAL/");
//     wal.log_abort("1");
//     vector<pair<string, string>> p;
//     p.push_back({"a", "b"});
//     wal.log_prepare("1", p);
//     wal.log_commit("2");
//     wal.log_replication_init("3");
//     wal.log_pending_replication("400");
//     return 0;
// }
