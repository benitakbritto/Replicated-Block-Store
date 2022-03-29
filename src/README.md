# CS-739-P3

### gRPC Installation:
Follow these steps to install gRPC lib using cmake: https://grpc.io/docs/languages/cpp/quickstart/#setup. 
:warning: make sure to limit the processes by passing number(e.g. 4) during `make -j` command.

for example, instead of `make -j` use `make -j 4`

### Build
#### Config Setup:
0. export MY_INSTALL_DIR=$HOME/.local
1. mkdir -p cmake/build
2. pushd cmake/build
3. cmake -DCMAKE_PREFIX_PATH=$MY_INSTALL_DIR ../..
4. make -j 4
  

### Execute


### Clean
execute `make clean` from `cmake/build` directory.


### Performance Graphs
1. 4k aligned reads(same addr) - time vs num ops - shows server scalability
2. 4k aligned writes(same addr)- time vs num ops - shows server scalability
3. Non 4k aligned reads(same addr) - time vs num ops - shows server scalability
4. Non 4k aligned writes(same addr) - time vs num ops - shows server scalability

- Combine 1,2,3,4 graphs.
the above shows the *minimum-guaranteed* performance from our system. Because for read we are accessing the same memory/file. 

