#include "address_translation.h"

/*
    @input: takes in the client provided address
    @output: returns validity of client provided address
*/
bool AddressTranslation::IsValidAddress(int address)
{
    if (address < 0 || address >= BLOCK_STORAGE_MAX_SIZE) 
    {
        throw "Address out of bounds";
    }
    return true;
}

/*
    @input: takes in an aligned address
    @output: returns the directory number
*/
string AddressTranslation::GetDirectory(int address)
{
    return to_string(address / PARTITION_SIZE);
}

/*
    @input: takes in an aligned address
    @output: returns the file number
*/
string AddressTranslation::GetFile(int address)
{
    int newAddress = address - (stoi(GetDirectory(address)) * PARTITION_SIZE);
    return to_string(newAddress / BLOCK_SIZE);
}

/*
    @input: dir and file num
    @output: returns the concat of dir and file
*/
string AddressTranslation::GetPath(string dir, string file)
{
    return SERVER_STORAGE_PATH + dir + '/' + file;
}

/*
    @input: takes in the client provided address
    @output: returns true if address is BLOCK_SIZE aligned
*/
bool AddressTranslation::IsAddressAligned(int address)
{
    return (address % BLOCK_SIZE) == 0;
}

/*
    @input: takes in the client provided address
    @output: returns the BLOCK_SIZE address
*/
int AddressTranslation::GetAlignedAddress(int address)
{
    return (address - (address % BLOCK_SIZE));
}

/*
    @input: takes in the client provided address
    @output: returns the offset within a file
*/
int AddressTranslation::GetOffset(int address)
{
    return address % BLOCK_SIZE;
}

/*
    @input: takes in the client provided address
    @output: returns the size of the file to be accessed
*/
int AddressTranslation::GetSize(int address)
{
    int partitionStartAddr = stoi(GetFile(GetAlignedAddress(address))) * BLOCK_SIZE;
    int newAddress = (address % PARTITION_SIZE) - partitionStartAddr;
    return BLOCK_SIZE - newAddress;
}

/*
    @input: takes in the client provided address
    @output: returns the path details of the files to be accessed
*/
vector<PathData> AddressTranslation::GetAllFileNames(int address)
{
    dbgprintf("GetAllFileNames: Entering function\n");

    vector<PathData> res;
    try
    {
        IsValidAddress(address);

        // First set of path data
        int alignedAddress = GetAlignedAddress(address);
        dbgprintf("GetAllFileNames: alignedAddress = %d\n", alignedAddress);
        string dir = GetDirectory(alignedAddress);
        dbgprintf("GetAllFileNames: dir = %s\n", dir.c_str());
        string file = GetFile(alignedAddress);
        dbgprintf("GetAllFileNames: file = %s\n", file.c_str());
        string path = GetPath(dir, file);
        int offset = GetOffset(address);
        int size = GetSize(address);
        dbgprintf("GetAllFileNames: path  = %s | offset = %d | size = %d\n", path.c_str(), offset, size);
        res.push_back(PathData(path, offset, size));

        // Second set of path data
        if (!IsAddressAligned(address))
        {
            dbgprintf("GetAllFileNames: address is not aligned!\n");
            alignedAddress = alignedAddress + BLOCK_SIZE;
            dbgprintf("GetAllFileNames: alignedAddress = %d\n", alignedAddress);
            string dir = GetDirectory(alignedAddress);
            dbgprintf("GetAllFileNames: dir = %s\n", dir.c_str());
            string file = GetFile(alignedAddress);
            dbgprintf("GetAllFileNames: file = %s\n", file.c_str());        
            path = GetPath(dir, file);
            offset = 0;
            size = BLOCK_SIZE - size;
            dbgprintf("GetAllFileNames: path  = %s | offset = %d | size = %d\n", path.c_str(), offset, size);
            res.push_back(PathData(path, offset, size));
        }

        dbgprintf("GetAllFileNames: res size = %ld\n", res.size());
        dbgprintf("GetAllFileNames: Exiting function on success path\n");

        return res;
    }
    catch (const char* msg)
    {
        dbgprintf("AddressTranslation::GetFileNames: Exception %s\n", msg); 
        dbgprintf("GetAllFileNames: Exiting function on error path\n");
        return vector<PathData>();
    }
}

// Test
// int main(int argc, char ** argv)
// {
//     AddressTranslation atl;
//     if (argc != 2)
//     {
//         cout << "usage: ./atl <addr>" << endl;
//         return 1;
//     }

//     int address = atoi(argv[1]);
//     dbgprintf("address = %d\n", address);
//     auto res = atl.GetAllFileNames(address);

//     for (auto val : res)
//     {
//         cout << val.path << " | " << val.offset << " | " << val.size << endl;
//     }

//     return 0;
// }
