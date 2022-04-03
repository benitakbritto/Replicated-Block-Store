#include <mutex>
#include <shared_mutex>
#include <unordered_map>
#include <cassert>
#include <chrono>
#include <iostream>
#include <thread>

/******************************************************************************
 * NAMESPACES
 *****************************************************************************/
using namespace std;

class MutexMap {
    mutex outer_mutex;
    unordered_map<string, shared_mutex> mutices;

    public:
    MutexMap() {}

    shared_lock<shared_mutex> GetReadLock(string key);
    unique_lock<shared_mutex> GetWriteLock(string key);
    void ReleaseReadLock(shared_lock<shared_mutex>& file_mutex);
    void ReleaseWriteLock(unique_lock<shared_mutex>& file_mutex);
};