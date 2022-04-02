#include <iostream>
#include <chrono>
#include "../src/client.h"
#include <memory>
#include <string>
#include <grpcpp/grpcpp.h>
#include <unistd.h>

/******************************************************************************
 * ENUM
 *****************************************************************************/
enum Test
{
    READ,
    WRITE,
};

/******************************************************************************
 * MACROS
 *****************************************************************************/
#define BLOCK_SIZE          4096

/******************************************************************************
 * NAMESPACES
 *****************************************************************************/
using namespace std;
using namespace chrono;
using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using blockstorage::BlockStorage;
using blockstorage::ReadReply;
using blockstorage::ReadRequest;
using blockstorage::WriteReply;
using blockstorage::WriteRequest;

/******************************************************************************
 * PROTOTYPES
 *****************************************************************************/
void warmup();
void read_test(int start_addr, int jump, int limit, int iterations);
void write_test(int start_addr, int jump, int limit, int iterations);
void print_time();
string generate_str(string letter);

/******************************************************************************
 * DRIVER
 *****************************************************************************/
int main() 
{
    // Init
    BlockStorageClient blockstorageClient(grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials()));
    int start_addr = 0;
    int jump = 0;
    int limit = 0;
    int interations = 1;
    int test = 0;
    char c = '';

    // Get command line args
    while ((c = getopt(argc, argv, "a:j:l:i:t:")) != -1)
    {
        switch (c)
        {
            case 'a':
                start_addr = stoi(optarg);
                break;
            case 'j':
                jump = stoi(optarg);
                break;
            case 'l':
                limit = stoi(optarg);
                break;
            case 'i':
                iterations = stoi(optarg);
                break;
            case 't':
                test = stoi(optarg);
        }
    }


    // Run test
    switch(test)
    {
        case READ:
            read_test(blockstorageClient, start_addr, jump, limit, iterations);
            break;
        case WRITE:
            write_test(blockstorageClient, start_addr, jump, limit, iterations);
            break;
        default:
            cout << "Invalid test" << endl;
    }

    return 0;
}

/******************************************************************************
 * DEFINITIONS
 *****************************************************************************/
// TODO
void warmup()
{

}

void read_test(BlockStorageClient* blockstorageClient, int start_addr, int jump, int limit, int iterations)
{
    int addr = 0;
    ReadRequest request;
    ReadReply reply;

    // TODO: Call warmup
    
    for (int i = 0; i < limit; i++)
    {
        int addr = start_addr + jump * i;
        string addr_str = to_string(addr);
        
        cout << "ADDRESS: " << addr_str << endl;

        for (int itr = 0; itr < iterations; itr++)
        {
            request.Clear();
            reply.Clear();
            request.set_addr(addr_str);

            auto start = steady_clock::now();
            Status readStatus = blockstorageClient->Read(request, &reply, address);
            auto end = steady_clock::now();
            
            nanoseconds elapsed_time = end - start;
            print_time("Read", addr_str, itr, elapsed_time);
        }
    }
}

void write_test(BlockStorageClient* blockstorageClient, int start_addr, int jump, int limit, int iterations)
{
    int addr = 0;
    string letter = "A";
    string buffer = generateStr(letter);

    // TODO: Call warmup

    for (int i = 0; i < limit; i++)
    {
        int addr = start_addr + jump * i;
        string addr_str = to_string(addr);
        
        cout << "ADDRESS: " << addr_str << endl;

        for (int itr = 0; itr < iterations; itr++)
        {
            auto start = steady_clock::now();
            Status writeStatus = blockstorageClient->Write(addr_str, buffer);
            auto end = steady_clock::now();
            
            nanoseconds elapsed_time = end - start;
            print_time("Read", addr_str, itr, elapsed_time);
        }
    }
}

void print_time(string metric, string addr, int iteration_num, nanoseconds elapsed_time)
{
    cout << "[Metric:" << metric <<":]"
        << "[Address:" << addr << ":]"
        << "[Iteration:" << iteration_num << ":]"
        << "[Elapsed Time:" << elapsed_time.count() << ":]"
        << endl;
}

string generateStr(string letter)
{
    string buffer = "";
    for (int i = 0; i < BLOCK_SIZE; i++)
    {
        buffer += letter;
    }
    return buffer;
}