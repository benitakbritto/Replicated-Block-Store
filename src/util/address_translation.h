#ifndef ATL_H
#define ATL_H

#include <iostream>
#include <vector>
#include <exception>
#include <string>
#include "common.h"

/******************************************************************************
 * MACROS
 *****************************************************************************/
#define BLOCK_STORAGE_MAX_SIZE      1073741824
#define BLOCK_SIZE                  4096
#define PARTITION_SIZE              1048576     
#define MAX_FILES_IN_DIR            256
#define MAX_DIR                     1024   
#define SERVER_STORAGE_PATH         "/home/benitakbritto/CS-739-P3/storage/"

/******************************************************************************
 * NAMESPACES
 *****************************************************************************/
using namespace std;

/******************************************************************************
 * GLOBALS
 *****************************************************************************/
struct PathData
{
    string path;
    int offset;
    int size;
    PathData(string path, 
            int offset,
            int size) :
            path(path),
            offset(offset),
            size(size) 
            {}
};

/******************************************************************************
 * DECLARATION
 *****************************************************************************/
class AddressTranslation  
{
private:
    bool IsValidAddress(int address);
    string GetDirectory(int address);
    string GetFile(int address);
    string GetPath(string dir, string file);
    bool IsAddressAligned(int address);
    int GetAlignedAddress(int address);
    int GetOffset(int address);
    int GetSize(int address);

public:
    AddressTranslation() {}

    vector<PathData> GetAllFileNames(int address);

};

#endif