/******************************************************************************
 * @usage: ./latency_perf -a <num> -j <num> -l <num> -i <num> -t <num>
 * where 
 *  a is the start address
 *  j is the jump amount
 *  l is the limit (number of times jump will increment start address)
 *  i is the iteration amount
 *  t is the test (0: READ, 1: WRITE)
 * 
 * @prereq: Things to do before running this test
 *  1. CLEAN self.log on PRIMARY and BACKUP
 *  2. PRIMARY up; BACKUP down
 *  3. Check Load Balancer address
 * 
 * @after: Things to do after executing this 
 *  1. Run BACKUP
 *  2. Redirect BACKUP output to a file -- This will be used to generate graphs
 *****************************************************************************/

#include <iostream>
#include "../src/client.h"
#include <memory>
#include <string>
#include <grpcpp/grpcpp.h>
#include <unistd.h>

/******************************************************************************
 * GLOBALS
 *****************************************************************************/
BlockStorageClient *blockstorageClient;

/******************************************************************************
 * MACROS
 *****************************************************************************/
#define BLOCK_SIZE          4096
#define LOAD_BALANCER_ADDR  "20.228.235.42:50051"

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
void exec_write(int start_addr, int jump, int limit, int iterations);
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
    char c = '\0';

    // Parse Command line
    while ((c = getopt(argc, argv, "a:j:l:i:")) != -1)
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
        }
    }

    // Run test
    exec_write(start_addr, jump, limit, iterations);

    return 0;
}

void exec_write(int start_addr, int jump, int limit, int iterations)
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
            Status writeStatus = blockstorageClient->Write(addr, buffer);            
        }
    }
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