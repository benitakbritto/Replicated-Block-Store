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

uint64_t DIR_SIZE = 256*4096;
uint64_t FILE_SIZE = 4096;
int MAX_REQUESTS_PER_WORKER = 64;

BlockStorageClient *blockstorageClient;

void ping() {
    for(int i = 0; i < 2; i++) {
        blockstorageClient->Ping();
    }
}

int read_worker(int start_addr, int jump, int num_requests, int id) {
    ReadReply reply;

    for(int i = 0, addr = start_addr; i < num_requests; i++, addr += jump) {
        ReadRequest request;
        request.set_addr(addr);

        // actual call
        Status readStatus = blockstorageClient->Read(request, &reply, addr);
    
        if (readStatus.error_code() != grpc::StatusCode::OK) {
            cout << "Read test failed" << endl;
        }
    }
    return 0;
}

void exec_reads(int N_workers, int num_requests, int jump_within_worker, int start_addr) {
    // stores start addrs for each worker
    uint64_t start_addrs[N_workers];
    future<int> workers[N_workers];

    // prepare all addresses
    for(int addr = start_addr, i = 0; i < N_workers; i++, addr += DIR_SIZE) {
        // cout << addr << endl;
        start_addrs[i] = addr;
    }

    // start all workers by passing necessary instructions
    for (int i = 0; i < N_workers; i++) {
        workers[i] = async(read_worker, start_addrs[i], jump_within_worker, num_requests, i); 
    }

    // wait for them to finish
    for(int i = 0; i < N_workers; i++) {
        workers[i].get();
    }
}

void read_performance(int workers) {

    // 2 - read non-aligned wrt number of requests
    cout << "[Metrics_START:READ_NON_ALIGNED]" << endl;
    cout << "num_requests,time(in ms)" << endl;
    for(int num_request = 1; num_request <= MAX_REQUESTS_PER_WORKER; num_request*=2) {
        auto begin = chrono::high_resolution_clock::now();
        exec_reads(workers, num_request, FILE_SIZE*3, FILE_SIZE + 2048);
        auto end = chrono::high_resolution_clock::now();
        uint64_t time_taken = chrono::duration_cast<chrono::nanoseconds> (end - begin).count();
        cout << num_request*workers << "," << time_taken/1e6 << endl;
    }
    cout << "[Metrics_END:READ_NON_ALIGNED]" << endl;

    // 1 - read aligned wrt number of requests
    cout << "[Metrics_START:READ_ALIGNED]" << endl;
    cout << "num_requests,time(in ms)" << endl;
    for(int num_request = 1; num_request <= MAX_REQUESTS_PER_WORKER; num_request*=2) {
        auto begin = chrono::high_resolution_clock::now();
        exec_reads(workers, num_request, FILE_SIZE*3, 0);
        auto end = chrono::high_resolution_clock::now();
        uint64_t time_taken = chrono::duration_cast<chrono::nanoseconds> (end - begin).count();
        cout << num_request*workers << "," << time_taken/1e6 << endl;
    }
    cout << "[Metrics_END:READ_ALIGNED]" << endl;
}

int write_worker(int start_addr, int jump, int num_requests, int id) {
    string buffer(FILE_SIZE, 'w');
    Status writeStatus;

    for(int i = 0, addr = start_addr; i < num_requests; i++, addr += jump) {
        // actual call
        writeStatus = blockstorageClient->Write(addr, buffer);

        if (writeStatus.error_code() != grpc::StatusCode::OK) {
            cout << "Write test failed" << endl;
        }
    }

    return 0;
}

void exec_writes(int N_workers, int num_requests, int jump_within_worker, int start_addr) {
    // stores start addrs for each worker
    uint64_t start_addrs[N_workers];
    future<int> workers[N_workers];

    // prepare all addresses
    for(int addr = start_addr, i = 0; i < N_workers; i++, addr += DIR_SIZE) {
        start_addrs[i] = addr;
    }

    // start all workers by passing necessary instructions
    for (int i = 0; i < N_workers; i++) {
        workers[i] = async(write_worker, start_addrs[i], jump_within_worker, num_requests, i); 
    }

    // wait for them to finish
    for(int i = 0; i < N_workers; i++) {
        workers[i].get();
    }
}

void write_performance(int workers) {

     // 1 - read aligned wrt number of requests
    cout << "[Metrics_START:WRITE_ALIGNED]" << endl;
    cout << "num_requests,time(in ms)" << endl;
    for(int num_request = 1; num_request <= MAX_REQUESTS_PER_WORKER; num_request*=2) {
        auto begin = chrono::high_resolution_clock::now();
        exec_writes(workers, num_request, FILE_SIZE*3, 0);
        auto end = chrono::high_resolution_clock::now();
        uint64_t time_taken = chrono::duration_cast<chrono::nanoseconds> (end - begin).count();
        cout << num_request*workers << "," << time_taken/1e6 << endl;
    }
    cout << "[Metrics_END:WRITE_ALIGNED]" << endl;

     // 2 - read non-aligned wrt number of requests
    cout << "[Metrics_START:WRITE_NON_ALIGNED]" << endl;
    cout << "num_requests,time(in ms)" << endl;
    for(int num_request = 1; num_request <= MAX_REQUESTS_PER_WORKER; num_request*=2) {
        auto begin = chrono::high_resolution_clock::now();
        exec_writes(workers, num_request, FILE_SIZE*3, FILE_SIZE + 2048);
        auto end = chrono::high_resolution_clock::now();
        uint64_t time_taken = chrono::duration_cast<chrono::nanoseconds> (end - begin).count();
        cout << num_request*workers << "," << time_taken/1e6 << endl;
    }
    cout << "[Metrics_END:WRITE_NON_ALIGNED]" << endl;

}

void write_congestion(int worker_limit) {

    cout << "[Metrics_START:WRITE_ALIGNED_CONGESTION]" << endl;
    cout << "workers,time(in ms)" << endl;
    for(int num_worker = 1; num_worker <= worker_limit; num_worker++) {
        future<int> workers[num_worker];
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
        future<int> workers[num_worker];
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

void read_congestion(int worker_limit) {
    cout << "[Metrics_START:READ_ALIGNED_CONGESTION]" << endl;
    cout << "workers,time(in ms)" << endl;
    for(int num_worker = 1; num_worker <= worker_limit; num_worker++) {
        future<int> workers[num_worker];
        uint64_t time_taken = 0;

        for(int i = 0; i < num_worker; i++) {
            workers[i] = async(read_worker, 0, 0, 1, i); 

        }
        for (int i = 0; i < num_worker; i++) {
            workers[i].get();
        }

        cout << num_worker << "," << time_taken/1e6 << endl;
    }
    cout << "[Metrics_END:READ_ALIGNED_CONGESTION]" << endl;

    cout << "[Metrics_START:READ_NON_ALIGNED_CONGESTION]" << endl;
    cout << "workers,time(in ms)" << endl;
    int start_addr = FILE_SIZE/2;
    for(int num_worker = 1; num_worker <= worker_limit; num_worker++) {
        future<int> workers[num_worker];
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

// Usage: ./rw_perf 1 4 - write with 4 workers
// Usage: ./rw_perf 0 3 - read with 3 workers
int main(int argc, char** argv) {
    blockstorageClient = new BlockStorageClient(grpc::CreateChannel("128.105.144.16:50051", grpc::InsecureChannelCredentials()));
    string arg(argv[1]);
    int workers = stoi(string(argv[2]));

    if (arg.compare("1") == 0) {
        write_performance(workers);
    } else if (arg.compare("2") == 0) {
        read_performance(workers);
    } else if (arg.compare("3") == 0) {
        write_congestion(workers);
    } else if (arg.compare("4") == 0) {
        read_congestion(workers);
    } else if (arg.compare("5") == 0) {
        write_performance(workers);
        read_performance(workers);
        write_congestion(workers);
        read_congestion(workers);
    } 
    
    return 0;
}
