#include <mutex>
#include <shared_mutex>
#include <unordered_map>

class MutexMap {
    std::mutex outer_mutex;
    std::unordered_map<std::string, std::shared_mutex> mutices;

   public:
    MutexMap();

    std::shared_lock<std::shared_mutex> GetReadLock(std::string key);
    std::unique_lock<std::shared_mutex> GetWriteLock(std::string key);
};