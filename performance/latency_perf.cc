/******************************************************************************
 * @usage: ./latency_perf -a <num> -j <num> -l <num> -i <num> -t <num>
 * where 
 *  a is the start address
 *  j is the jump amount
 *  l is the limit (number of times jump will increment start address)
 *  i is the iteration amount
 *  t is the test (0: READ, 1: WRITE)
 * 
 *  @prereq: Things to do before running this test
 *  1. CLEAN self.log on PRIMARY and BACKUP
 *  2. Check Load Balancer address
 *****************************************************************************/

#include <iostream>
#include <chrono>
#include "../src/client.h"
#include <memory>
#include <string>
#include <grpcpp/grpcpp.h>
#include <unistd.h>

/******************************************************************************
 * GLOBALS
 *****************************************************************************/
enum Test
{
    READ,
    WRITE,
};
BlockStorageClient *blockstorageClient;

/******************************************************************************
 * MACROS
 *****************************************************************************/
#define BLOCK_SIZE          4096
#define LOAD_BALANCER_ADDR  "128.105.144.16:50051"

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
void print_time(string metric, int addr, int iteration_num, nanoseconds elapsed_time);
string generate_str(string letter);

/******************************************************************************
 * DRIVER
 *****************************************************************************/
int main(int argc, char** argv) 
{
    // Init
    blockstorageClient = new BlockStorageClient(grpc::CreateChannel(LOAD_BALANCER_ADDR, grpc::InsecureChannelCredentials()));
    int start_addr = 0;
    int jump = 0;
    int limit = 0;
    int iterations = 0;
    int test = 0;
    char c = '\0';

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
                break;
            default:
                cout << "Invalid arg" << endl;
                return -1;
        }
    }


    // Run test
    switch(test)
    {
        case READ:
            read_test(start_addr, jump, limit, iterations);
            break;
        case WRITE:
            write_test(start_addr, jump, limit, iterations);
            break;
        default:
            cout << "Invalid test" << endl;
            return -1;
    }

    return 0;
}

/******************************************************************************
 * DEFINITIONS
 *****************************************************************************/
// TODO: Change to rw_perf warmup
void warmup()
{
    string buffer(5, 'w');
    int address = 0;

    Status writeStatus = blockstorageClient->Write(address, buffer);
    
    ReadRequest request;
    ReadReply reply;
    request.set_addr(address);
    Status readStatus = blockstorageClient->Read(request, &reply, address);
}

void read_test(int start_addr, int jump, int limit, int iterations)
{
    int addr = 0;
    ReadRequest request;
    ReadReply reply;

    // warmup();
    
    for (int i = 0; i < limit; i++)
    {
        int addr = start_addr + jump * i;
        
        cout << "ADDRESS: " << addr << endl;

        for (int itr = 0; itr < iterations; itr++)
        {
            request.Clear();
            reply.Clear();
            request.set_addr(addr);

            auto start = steady_clock::now();
            Status readStatus = blockstorageClient->Read(request, &reply, addr);
            auto end = steady_clock::now();
            
            nanoseconds elapsed_time = end - start;
            print_time("Read", addr, itr, elapsed_time);
        }
    }
}

void write_test(int start_addr, int jump, int limit, int iterations)
{
    int addr = 0;
    string letter = "A";
    string buffer = generate_str(letter);

    // warmup();

    for (int i = 0; i < limit; i++)
    {
        int addr = start_addr + jump * i;
        
        cout << "ADDRESS: " << addr << endl;

        for (int itr = 0; itr < iterations; itr++)
        {
            auto start = steady_clock::now();
            Status writeStatus = blockstorageClient->Write(addr, buffer);
            auto end = steady_clock::now();
            
            nanoseconds elapsed_time = end - start;
            print_time("Write", addr, itr, elapsed_time);
        }
    }
}

void print_time(string metric, int addr, int iteration_num, nanoseconds elapsed_time)
{
    cout << "[Metric:" << metric <<"]"
        << "[Address:" << addr << "]"
        << "[Iteration:" << iteration_num << "]"
        << "[Elapsed Time:" << (elapsed_time.count() / 1e6) << "ms]"
        << endl;
}

string generate_str(string letter)
{
    string buffer = "";
    for (int i = 0; i < BLOCK_SIZE; i++)
    {
        buffer += letter;
    }
    return buffer;
}