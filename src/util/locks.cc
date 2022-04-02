#include "locks.h"

#include <cassert>
#include <chrono>
#include <iostream>
#include <thread>

MutexMap::MutexMap() {}

std::shared_lock<std::shared_mutex> MutexMap::GetReadLock(std::string key) {
    outer_mutex.lock();
    std::shared_mutex &file_mutex = mutices[key];
    outer_mutex.unlock();

    return std::shared_lock(file_mutex);
}

std::unique_lock<std::shared_mutex> MutexMap::GetWriteLock(std::string key) {
    outer_mutex.lock();
    std::shared_mutex &file_mutex = mutices[key];
    outer_mutex.unlock();

    return std::unique_lock(file_mutex);
}

void MutexMap::GetWriteLock(std::lock(file_mutex)) {
    file_mutex.unlock();
}
