#include "locks.h"

shared_lock<shared_mutex> MutexMap::GetReadLock(string key) {
    outer_mutex.lock();
    shared_mutex &file_mutex = mutices[key];
    outer_mutex.unlock();

    return shared_lock(file_mutex);
}

std::unique_lock<shared_mutex> MutexMap::GetWriteLock(string key) {
    outer_mutex.lock();
    shared_mutex &file_mutex = mutices[key];
    outer_mutex.unlock();

    return unique_lock(file_mutex);
}

void MutexMap::ReleaseLock(shared_lock<shared_mutex>& file_mutex)
{
    file_mutex.unlock();
}

// // Tester
// int main()
// {
//     MutexMap m;
//     shared_lock<shared_mutex> a_lock = m.GetReadLock("a");
//     m.ReleaseLock(a_lock);
//     return 0;
// }