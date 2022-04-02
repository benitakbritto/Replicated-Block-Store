#ifndef CACHE_H
#define CACHE_H

#include <iostream>
#include <unordered_map>
#include "common.h"

/******************************************************************************
 * MACROS
 *****************************************************************************/
#define CACHE_SIZE 100

/******************************************************************************
 * NAMESPACES
 *****************************************************************************/
using namespace std;

class Cache
{
public:
    Cache () {}

    void AddToCache(unordered_map<string, string> &CACHE, string key, string value);
    string GetValueFromCache(unordered_map<string, string> &CACHE, string key);
    void UpdateCache(unordered_map<string, string> &CACHE, string key, string value);
    void DeleteKey(unordered_map<string, string> &CACHE, string key);
};


#endif