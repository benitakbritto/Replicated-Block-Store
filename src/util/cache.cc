#include "cache.h"

void Cache::AddToCache(unordered_map<string, string> &CACHE, string key, string value)
{
    dbgprintf("[INFO] AddToCache: Entering function\n");
    if (CACHE.count(key) != 0)
    {
        dbgprintf("[ERR] AddToCache: key already exists\n");
    }
    else
    {
        CACHE[key] = value;
        dbgprintf("[INFO] AddToCache: Added value\n");
    }
    dbgprintf("[INFO] AddToCache: Exiting function\n");
}

string Cache::GetValueFromCache(unordered_map<string, string> &CACHE, string key)
{
    dbgprintf("[INFO] GetValueFromCache: Entering function\n");
    if (CACHE.count(key) == 0)
    {
        dbgprintf("[ERR] GetValueFromCache: key does not exist\n");
        return string("");
    }
    else
    {
        return CACHE[key];
    }
    dbgprintf("[INFO] GetValueFromCache: Exiting function\n");
}

void Cache::UpdateCache(unordered_map<string, string> &CACHE, string key, string value)
{
    dbgprintf("[INFO] UpdateCache: Entering function\n");
    if (CACHE.count(key) == 0)
    {
        dbgprintf("[ERR] AddToCache: key does not exist\n");
    }
    else
    {
        CACHE[key] = value;
        dbgprintf("[INFO] AddToCache: Updated value\n");
    }
    dbgprintf("[INFO] UpdateCache: Exiting function\n");
}

void Cache::DeleteKey(unordered_map<string, string> &CACHE, string key)
{
    dbgprintf("[INFO] DeleteKey: Entering function\n");
    if (CACHE.count(key) == 0)
    {
        dbgprintf("[ERR] DeleteKey: key does not exist\n");
    }
    else
    {
        CACHE.erase(key);
        dbgprintf("[INFO] DeleteKey: Updated value\n");
    }
    dbgprintf("[INFO] DeleteKey: Exiting function\n");
}

// Tester
int main()
{
    unordered_map<string, string> CACHE;
    Cache c;

    c.AddToCache(CACHE, "1", "a");
    cout << "Key: 1, Value: " << c.GetValueFromCache(CACHE, "1") << endl;
    return 0;
}
