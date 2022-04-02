#include <iostream>
#include <thread>
#include <future>
#include <chrono>
#include <unistd.h>

#include "../src/client.h"
#include <grpcpp/grpcpp.h>
#include "blockstorage.grpc.pb.h"

using namespace std;

uint64_t DIR_SIZE = 107374182;
uint64_t FILE_SIZE = 4096;


void warmup() {
    string data(5, 'w');
    cout << data << endl;
}

uint64_t read_worker(int start_addr, int jump, int N, int id) {
    uint64_t time_taken = 0;

    for(int i = 0, addr = start_addr; i < N; i++, addr += jump) {
        cout << "WORKER:" << id << " running addr:[" << addr << "]" << endl;
        auto begin = chrono::high_resolution_clock::now();

        // actual call
        sleep(1);

        auto end = chrono::high_resolution_clock::now();
        time_taken += chrono::duration_cast<chrono::nanoseconds> (end - begin).count();
    }

    return time_taken;
}

void read_perf(int num_requests) {
    warmup();

    int N_workers = 4;

    uint64_t addrs[N_workers];
    future<uint64_t> workers[N_workers];
    uint64_t times[N_workers];

    // prepare all addresses
    for(int addr = 0, i = 0; i < N_workers; i++, addr += DIR_SIZE) {
        addrs[i] = addr;
    }

    // start all workers by passing necessary instructions
    for (int i = 0; i < N_workers; i++) {
        workers[i] = async(read_worker, addrs[i], FILE_SIZE, num_requests, i); 
    }

    // wait for them to finish
    for(int i = 0; i < N_workers; i++) {
        times[i] = workers[i].get();
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

int main() {
    warmup();
    // read_perf(5);
    // sample();
    // sample2();
    return 0;
}