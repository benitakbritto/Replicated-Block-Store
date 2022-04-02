#include <iostream>
#include <thread>
#include <future>
#include <chrono>

using namespace std;

void warmup() {
    
}

int read_worker(int start_addr, int jump, int N) {
    warmup();

    int time_taken = 0;

    for(int i = 0, addr = start_addr; i < N; i++, addr += jump) {
        
    }

    return time_taken;
}

void read_perf() {
    // int workers = 
    // future<int> workers[8];

    // for (int i = 0; i < )
    // future<int> ret = async(pause_thread2, 0, 2);
    // int i = ret.get();
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
    read_perf();
    // sample();
    // sample2();
    return 0;
}