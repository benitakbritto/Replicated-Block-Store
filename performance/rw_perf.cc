#include "../src/client.h"
#include <iostream>
#include <thread>
#include <future>
#include <chrono>
#include <unistd.h>
#include <grpcpp/grpcpp.h>
#include "blockstorage.grpc.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using blockstorage::BlockStorage;
using blockstorage::ReadReply;
using blockstorage::ReadRequest;
using blockstorage::WriteReply;
using blockstorage::WriteRequest;

using namespace std;

uint64_t DIR_SIZE = 107374182;
uint64_t FILE_SIZE = 4096;

BlockStorageClient *blockstorageClient;

void warmup() {
    string buffer(5, 'w');
    int addr = 0;
    
    Status writeStatus = blockstorageClient->Write(addr, buffer);
    cout<<writeStatus.error_code();
    if (writeStatus.error_code() != grpc::StatusCode::OK) {
        cout << "Write test failed" << endl;
        return;
    }

    ReadRequest request;
    ReadReply reply;
    request.set_addr(addr);

    Status readStatus = blockstorageClient->Read(request, &reply, addr);
    
    if (readStatus.error_code() != grpc::StatusCode::OK) {
        cout << "Read test failed" << endl;
        return;
    }

    if (reply.buffer().compare(buffer) == 0) {
        cout << "TEST PASSED:" << endl;
    } else {
        cout << "TEST FAILED: Aligned read data is not the same as write buffer " << endl;
        cout << "Read : " << reply.buffer() << endl;
        cout << "Written: " << buffer << endl; 
    }
}

void ping() {
    for(int i = 0; i < 10; i++) {
        blockstorageClient->Ping();
    }
}

uint64_t read_worker(int start_addr, int jump, int num_requests, int id) {
    uint64_t time_taken = 0;

    ReadReply reply;

    for(int i = 0, addr = start_addr; i < num_requests; i++, addr += jump) {
        // cout << "WORKER:" << id << " running addr:[" << addr << "]" << endl;
        auto begin = chrono::high_resolution_clock::now();

        ReadRequest request;
        request.set_addr(addr);

        // actual call
        Status readStatus = blockstorageClient->Read(request, &reply, addr);
    
        if (readStatus.error_code() != grpc::StatusCode::OK) {
            // cout << "Read test failed" << endl;
        }

        auto end = chrono::high_resolution_clock::now();
        time_taken += chrono::duration_cast<chrono::nanoseconds> (end - begin).count();
    }

    return time_taken;
}

uint64_t exec_reads(int N_workers, int num_requests, int jump_within_worker, int start_addr) {
    // stores start addrs for each worker
    uint64_t start_addrs[N_workers];
    future<uint64_t> workers[N_workers];
    uint64_t time_taken = 0;

    // prepare all addresses
    for(int addr = start_addr, i = 0; i < N_workers; i++, addr += DIR_SIZE) {
        start_addrs[i] = addr;
    }

    // start all workers by passing necessary instructions
    for (int i = 0; i < N_workers; i++) {
        workers[i] = async(read_worker, start_addrs[i], jump_within_worker, num_requests, i); 
    }

    // wait for them to finish
    for(int i = 0; i < N_workers; i++) {
        time_taken += workers[i].get();
    }

    return time_taken;
}

void read_performance() {
    int workers = 1;

    // 1 - read aligned wrt number of requests
    for(int num_request = 1; num_request <= 1024; num_request*=2) {
        uint64_t time_taken = exec_reads(workers, num_request, 0, FILE_SIZE);
        cout << "[num_requests:" << num_request*workers << "]:[time_taken:" << time_taken/1e6 << " ms]" << endl;
    }

    // 2 - read non-aligned wrt number of requests
    for(int num_request = 1; num_request <= 1024; num_request*=2) {
        uint64_t time_taken = exec_reads(workers, num_request, 2048, FILE_SIZE);
        cout << "[num_requests:" << num_request*workers << "]:[time_taken:" << time_taken/1e6 << " ms]" << endl;
    }
}

void pause_thread(int n, int id) {
    std::this_thread::sleep_for (std::chrono::seconds(n));
    std::cout << "ID:" << id << ", pause of " << n << " seconds ended\n";
}

int pause_thread2(int n, int id) {
    std::this_thread::sleep_for (std::chrono::seconds(n));
    std::cout << "ID:" << id << ", pause of " << n << " seconds ended\n";
    return id;
}

void sample() {
    std::thread threads[5];                         // default-constructed threads

    std::cout << "Spawning 5 threads...\n";
    for (int i=0; i<5; ++i)
        threads[i] = std::thread(pause_thread, i+1, i + 1);   // move-assign threads

    std::cout << "Done spawning threads. Now waiting for them to join:\n";
    for (int i=0; i<5; ++i)
        threads[i].join();

    std::cout << "All threads joined!\n";
}

void sample2() {
    future<int> ret = async(pause_thread2, 0, 2);
    int i = ret.get();
}

// Usage: ./rw_perf 1 - just for warmup - no actual perf done
// Usage: ./rw_perf 0 - just actual perf
int main(int argc, char** argv) {
    blockstorageClient = new BlockStorageClient(grpc::CreateChannel("0.0.0.0:50051", grpc::InsecureChannelCredentials()));
    
    string ping_arg(argv[1]);
    if (ping_arg.compare("1") == 0) {
        cout << "Warming up" << endl;
        ping();
        cout << "WARM up done" << endl;
        return 0;    
    } else {
        cout << "not warming up" << endl;
    }
    
    read_performance();
    // sample();
    // sample2();
    return 0;
}