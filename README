# CS-739-P3: Replicated Block Store

## Replication Strategy
1. We use Primary-Backup. We support only 2 server nodes.
Source code: `src/blockstorage_server.cc`
- `class BlockStorageServiceImpl`: responds to client requests of `Read(Addr)` and `Write(Addr)`
- `class ServiceCommImpl`: communicates with the other server node to propogate writes
- `class ServiceCommClient`: communicates witht the other server for recovery
- `class LBNodeCommClient`: communicates with the Load Balancer to send heartbeats and identify failover

2. Client library
Souce code: `src/client.h` and `src/client/cc`

3. Load Balancer
Source code: `src/load_balancer.cc`
- `class BlockStorageService`: forwards client request to a server
- `class LBNodeCommService`: used to identify server unavailability

## Durability
We use a two phase strategy of writing to a temp file and atomic rename from temp to orginal file  
Source code: `src/blockstorage_server.cc`

## Crash Recovery Protocol
We perform 4 steps to recover from a server crash viz.
1. Parse log file
2. Get pending writes
3. Recover from other states
4. Cleanup   
Source code: `src/util/crash_recovery.h` and `src/util/crash_recovery.cc`

## Failover
Servers exchange hearbeats with the load balancer   
Source code: `src/blockstorage_server.cc` and `src/load_balancer.cc`


## Test code
Present in `performance/` and MACROS present in `src/util/common.h`
- `concurrent_rw.cc`: Client code that issues reads and writes concurrently
- `latency_perf.cc`: Client code that issues the read and write multiple times on the same address
- `recovery_perf.cc`: Client code that initialises the servers with writes to analyse recovery time
- `rw_perf.cc`: Client code that issues reads and writes on multiple addresses concurrently

## Utilities
1. Address Translation
Source code: `src/util/address_translation.h` and `src/util/address_translation.cc`

2. Cache
Source code: `src/util/cache.h` and `src/util/cache.cc`

3. Key Value Store
Source code: `src/util/kv_store.h` and `src/util/kv_store.cc`

4. Reader-Writer Locks
Source code: `src/util/locks.h` and `src/util/locks.cc`

5. Handling in-flight and future writes during recovery 
Source code: `src/util/txn.h` and `src/util/txn.cc`

6. Write Ahead Logging
Source code: `src/util/wal.h` and `src/util/wal.cc`

## Setup
### gRPC Installation
Follow these steps to install gRPC lib using cmake: https://grpc.io/docs/languages/cpp/quickstart/#setup. 
:warning: make sure to limit the processes by passing number(e.g. 4) during `make -j` command.

for example, instead of `make -j` use `make -j 4`

### Build
#### Main source code
0. cd src/
1. chmod 755 build.sh
2. ./build.sh

#### Performance scripts
0. cd performance/
1. chmod 755 build.sh
2. ./build.sh

### Run
#### Client
```
cd src/cmake/build
./blockstorage_client
```

OR

```
cd performance/
./build
cd cmake/build
<ANY OF THE EXECUTABLES LISTED IN THIS DIRECTORY>
```

#### Load Balancer
```
./load_balancer
```

#### Primary Server
```
./blockstorage_server PRIMARY [self_addr_lb] [self_addr_peer] [peer_addr] [lb_addr]
```
Eg.
```
./blockstorage_server PRIMARY 20.127.48.216:40051 0.0.0.0:60052 20.127.55.97:60053 20.228.235.42:50056
```

#### Backup Server
```
./blockstorage_server BACKUP [self_addr_lb] [self_addr_peer] [peer_addr] [lb_addr]
```
Eg.
```
./blockstorage_server BACKUP 20.127.55.97:40051 0.0.0.0:60053 20.127.48.216:60052 20.228.235.42:50056
```

#### Performance Scripts
- concurrent_rw
```
cd performance/cmake/build
./concurrent_rw
```
- latency_perf
```
cd performance/cmake/build
./latency_perf -a <num> -j <num> -l <num> -i <num> -t <num>
```
Where a stands for the start address
j stands for the next address jump
l stands for the count of addresses / limit
i stands for the number of iterations
t stands for the test, 0 for read and 1 for write
- recovery_perf
```
./recovery_perf -a <num> -j <num> -l <num> -i <num>
```
Where a stands for the start address
j stands for the next address jump
l stands for the count of addresses / limit
i stands for the number of iterations

- rw_perf
```
./rw_perf <num> <num>
```
Where the first num corresponds to 0 for read and 1 for write. 
The second num corresponds to the number of worker threads.



## Deliverables
1. [Demos](https://uwprod-my.sharepoint.com/personal/rmukherjee28_wisc_edu/_layouts/15/onedrive.aspx?id=%2Fpersonal%2Frmukherjee28%5Fwisc%5Fedu%2FDocuments%2FP3&ct=1649304954996&or=OWA%2DNT&cid=4634676a%2D47ff%2D53ca%2De53f%2D6d37b0e93c83&ga=1)
2. [Report](https://docs.google.com/document/d/1bRZoiuBFnKtdHMuaLA-XYtsdmE0j9GiTS8y4GpB_kJ0/edit)
3. [Presentation](https://uwprod-my.sharepoint.com/:p:/g/personal/rmukherjee28_wisc_edu/EeUXlw-LdeZGlavFvaAIW2cBMunLacGTDcP6zYge57fgqw?e=4%3AjKLeK7&at=9&wdLOR=cA98CA1D5-CAD0-DF49-A064-EC07D7B90F13&PreviousSessionID=b9354234-a437-b792-1eff-d34830abb2fe)
