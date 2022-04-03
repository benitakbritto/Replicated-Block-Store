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

uint64_t DIR_SIZE = 256*1024;
uint64_t FILE_SIZE = 4096;
int MAX_REQUESTS_PER_WORKER = 128;

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
    for(int i = 0; i < 2; i++) {
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
            cout << "Read test failed" << endl;
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
    int workers = 4;

    // 1 - read aligned wrt number of requests
    cout << "[Metrics_START:READ_ALIGNED]" << endl;
    cout << "num_requests,time(in ms)" << endl;
    for(int num_request = 1; num_request <= MAX_REQUESTS_PER_WORKER; num_request*=2) {
        uint64_t time_taken = exec_reads(workers, num_request, 0, FILE_SIZE);
        cout << num_request*workers << "," << time_taken/1e6 << endl;
    }
    cout << "[Metrics_END:READ_ALIGNED]" << endl;

    // 2 - read non-aligned wrt number of requests
    cout << "[Metrics_START:READ_NON_ALIGNED]" << endl;
    cout << "num_requests,time(in ms)" << endl;
    for(int num_request = 1; num_request <= MAX_REQUESTS_PER_WORKER; num_request*=2) {
        uint64_t time_taken = exec_reads(workers, num_request, 2048, FILE_SIZE);
        cout << num_request*workers << "," << time_taken/1e6 << endl;
    }
    cout << "[Metrics_END:READ_NON_ALIGNED]" << endl;
}

uint64_t write_worker(int start_addr, int jump, int num_requests, int id) {
    uint64_t time_taken = 0;
    string buffer(FILE_SIZE, 'w');
    Status writeStatus;

    for(int i = 0, addr = start_addr; i < num_requests; i++, addr += jump) {
        // cout << "WORKER:" << id << " running addr:[" << addr << "]" << endl;
        auto begin = chrono::high_resolution_clock::now();

        // actual call
        writeStatus = blockstorageClient->Write(addr, buffer);

        if (writeStatus.error_code() != grpc::StatusCode::OK) {
            cout << "Write test failed" << endl;
        }

        auto end = chrono::high_resolution_clock::now();
        time_taken += chrono::duration_cast<chrono::nanoseconds> (end - begin).count();
    }

    return time_taken;
}

uint64_t exec_writes(int N_workers, int num_requests, int jump_within_worker, int start_addr) {
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

void write_performance() {
    int workers = 4;

    // 1 - read aligned wrt number of requests
    cout << "[Metrics_START:WRITE_ALIGNED]" << endl;
    cout << "num_requests,time(in ms)" << endl;
    for(int num_request = 1; num_request <= MAX_REQUESTS_PER_WORKER; num_request*=2) {
        uint64_t time_taken = exec_reads(workers, num_request, 0, FILE_SIZE);
        cout << num_request*workers << "," << time_taken/1e6 << endl;
    }
    cout << "[Metrics_END:WRITE_ALIGNED]" << endl;

    // 2 - read non-aligned wrt number of requests
    cout << "[Metrics_START:WRITE_NON_ALIGNED]" << endl;
    cout << "num_requests,time(in ms)" << endl;
    for(int num_request = 1; num_request <= MAX_REQUESTS_PER_WORKER; num_request*=2) {
        uint64_t time_taken = exec_reads(workers, num_request, 2048, FILE_SIZE);
        cout << num_request*workers << "," << time_taken/1e6 << endl;
    }
    cout << "[Metrics_END:WRITE_NON_ALIGNED]" << endl;
}

void write_congestion() {
    int worker_limit = 8;

    cout << "[Metrics_START:WRITE_ALIGNED_CONGESTION]" << endl;
    cout << "workers,time(in ms)" << endl;
    for(int num_worker = 1; num_worker <= worker_limit; num_worker++) {
        future<uint64_t> workers[num_worker];
        uint64_t time_taken = 0;

        for(int i = 0; i < num_worker; i++) {
            workers[i] = async(write_worker, 0, 0, 1, i); 

        }
        for (int i = 0; i < num_worker; i++) {
            time_taken += workers[i].get();
        }

        cout << num_worker << "," << time_taken/1e6 << endl;
    }
    cout << "[Metrics_END:WRITE_ALIGNED_CONGESTION]" << endl;

    cout << "[Metrics_START:WRITE_NON_ALIGNED_CONGESTION]" << endl;
    cout << "workers,time(in ms)" << endl;
    int start_addr = FILE_SIZE/2;
    for(int num_worker = 1; num_worker <= worker_limit; num_worker++) {
        future<uint64_t> workers[num_worker];
        uint64_t time_taken = 0;

        for(int i = 0; i < num_worker; i++) {
            workers[i] = async(write_worker, start_addr, 0, 1, i); 

        }
        for (int i = 0; i < num_worker; i++) {
            time_taken += workers[i].get();
        }

        cout << num_worker << "," << time_taken/1e6 << endl;
    }
    cout << "[Metrics_END:WRITE_NON_ALIGNED_CONGESTION]" << endl;
}

void read_congestion() {
    int worker_limit = 8;

    cout << "[Metrics_START:READ_ALIGNED_CONGESTION]" << endl;
    cout << "workers,time(in ms)" << endl;
    for(int num_worker = 1; num_worker <= worker_limit; num_worker++) {
        future<uint64_t> workers[num_worker];
        uint64_t time_taken = 0;

        for(int i = 0; i < num_worker; i++) {
            workers[i] = async(read_worker, 0, 0, 1, i); 

        }
        for (int i = 0; i < num_worker; i++) {
            time_taken += workers[i].get();
        }

        cout << num_worker << "," << time_taken/1e6 << endl;
    }
    cout << "[Metrics_END:READ_ALIGNED_CONGESTION]" << endl;

    cout << "[Metrics_START:READ_NON_ALIGNED_CONGESTION]" << endl;
    cout << "workers,time(in ms)" << endl;
    int start_addr = FILE_SIZE/2;
    for(int num_worker = 1; num_worker <= worker_limit; num_worker++) {
        future<uint64_t> workers[num_worker];
        uint64_t time_taken = 0;

        for(int i = 0; i < num_worker; i++) {
            workers[i] = async(read_worker, start_addr, 0, 1, i); 

        }
        for (int i = 0; i < num_worker; i++) {
            time_taken += workers[i].get();
        }

        cout << num_worker << "," << time_taken/1e6 << endl;
    }
    cout << "[Metrics_END:READ_NON_ALIGNED_CONGESTION]" << endl;
}

// Usage: ./rw_perf 1 - just for warmup - no actual perf done
// Usage: ./rw_perf 0 - just actual perf
int main(int argc, char** argv) {
    blockstorageClient = new BlockStorageClient(grpc::CreateChannel("0.0.0.0:50051", grpc::InsecureChannelCredentials()));
    string arg(argv[1]);

    if (arg.compare("1") == 0) {
        write_performance();
    } else if (arg.compare("2") == 0) {
        read_performance();
    } else if (arg.compare("3") == 0) {
        write_congestion();
    } else if (arg.compare("4") == 0) {
        read_congestion();
    } else if (arg.compare("5") == 0) {
        write_performance();
        read_performance();
        write_congestion();
        read_congestion();
    } 
    
    return 0;
}