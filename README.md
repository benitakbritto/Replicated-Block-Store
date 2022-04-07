# CS-739-P3

## Replication Strategy
1. We use Primary-Backup. We support only 2 server nodes.
Source code: `src/blockstorage_server.cc`

2. Client library
Souce code: `src/client.h` and `src/client/cc`

3. Load Balancer
Source code: `src/load_balancer.cc`

## Durability
We use a two phase strategy of writing to a temp file and atomic rename from temp to orginal file. 
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


## Setup
### gRPC Installation
Follow these steps to install gRPC lib using cmake: https://grpc.io/docs/languages/cpp/quickstart/#setup. 
:warning: make sure to limit the processes by passing number(e.g. 4) during `make -j` command.

for example, instead of `make -j` use `make -j 4`

### Build
0. cd src/
1. chmod 755 build.sh
2. ./build.sh

### Clean
execute `make clean` from `cmake/build` directory.


## Deliverables
1. [Demos](https://uwprod-my.sharepoint.com/personal/rmukherjee28_wisc_edu/_layouts/15/onedrive.aspx?id=%2Fpersonal%2Frmukherjee28%5Fwisc%5Fedu%2FDocuments%2FP3&ct=1649304954996&or=OWA%2DNT&cid=4634676a%2D47ff%2D53ca%2De53f%2D6d37b0e93c83&ga=1)
2. [Report](https://docs.google.com/document/d/1bRZoiuBFnKtdHMuaLA-XYtsdmE0j9GiTS8y4GpB_kJ0/edit)
3. [Presentation](https://uwprod-my.sharepoint.com/:p:/g/personal/rmukherjee28_wisc_edu/EeUXlw-LdeZGlavFvaAIW2cBMunLacGTDcP6zYge57fgqw?e=4%3AjKLeK7&at=9&wdLOR=cA98CA1D5-CAD0-DF49-A064-EC07D7B90F13&PreviousSessionID=b9354234-a437-b792-1eff-d34830abb2fe)
