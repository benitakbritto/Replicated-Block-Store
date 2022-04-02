cmake_minimum_required(VERSION 3.5.1)

project(Performance C CXX)

include(${CMAKE_CURRENT_SOURCE_DIR}/../common.cmake)

# # Proto files
# Proto file 1
get_filename_component(hw_proto "../src/protos/blockstorage.proto" ABSOLUTE)
get_filename_component(hw_proto_path "${hw_proto}" PATH)
# # Proto file 2
# get_filename_component(comm_proto "../src/protos/servercomm.proto" ABSOLUTE)
# get_filename_component(comm_proto_path "${comm_proto}" PATH)

# get_filename_component(lb_proto "../src/protos/lb.proto" ABSOLUTE)
# get_filename_component(lb_proto_path "${lb_proto}" PATH)

# Generated sources
# Proto file 1
set(hw_proto_srcs "${CMAKE_CURRENT_BINARY_DIR}/blockstorage.pb.cc")
set(hw_proto_hdrs "${CMAKE_CURRENT_BINARY_DIR}/blockstorage.pb.h")
set(hw_grpc_srcs "${CMAKE_CURRENT_BINARY_DIR}/blockstorage.grpc.pb.cc")
set(hw_grpc_hdrs "${CMAKE_CURRENT_BINARY_DIR}/blockstorage.grpc.pb.h")
add_custom_command(
      OUTPUT "${hw_proto_srcs}" "${hw_proto_hdrs}" "${hw_grpc_srcs}" "${hw_grpc_hdrs}"
      COMMAND ${_PROTOBUF_PROTOC}
      ARGS --grpc_out "${CMAKE_CURRENT_BINARY_DIR}"
        --cpp_out "${CMAKE_CURRENT_BINARY_DIR}"
        -I "${hw_proto_path}"
        --plugin=protoc-gen-grpc="${_GRPC_CPP_PLUGIN_EXECUTABLE}"
        "${hw_proto}"
      DEPENDS "${hw_proto}")

# # Proto file 2
# set(comm_proto_srcs "${CMAKE_CURRENT_BINARY_DIR}/servercomm.pb.cc")
# set(comm_proto_hdrs "${CMAKE_CURRENT_BINARY_DIR}/servercomm.pb.h")
# set(comm_grpc_srcs "${CMAKE_CURRENT_BINARY_DIR}/servercomm.grpc.pb.cc")
# set(comm_grpc_hdrs "${CMAKE_CURRENT_BINARY_DIR}/servercomm.grpc.pb.h")
# add_custom_command(
#       OUTPUT "${comm_proto_srcs}" "${comm_proto_hdrs}" "${comm_grpc_srcs}" "${comm_grpc_hdrs}"
#       COMMAND ${_PROTOBUF_PROTOC}
#       ARGS --grpc_out "${CMAKE_CURRENT_BINARY_DIR}"
#         --cpp_out "${CMAKE_CURRENT_BINARY_DIR}"
#         -I "${comm_proto_path}"
#         --plugin=protoc-gen-grpc="${_GRPC_CPP_PLUGIN_EXECUTABLE}"
#         "${comm_proto}"
#       DEPENDS "${comm_proto}")

# # Proto file 3
# set(lb_proto_srcs "${CMAKE_CURRENT_BINARY_DIR}/lb.pb.cc")
# set(lb_proto_hdrs "${CMAKE_CURRENT_BINARY_DIR}/lb.pb.h")
# set(lb_grpc_srcs "${CMAKE_CURRENT_BINARY_DIR}/lb.grpc.pb.cc")
# set(lb_grpc_hdrs "${CMAKE_CURRENT_BINARY_DIR}/lb.grpc.pb.h")
# add_custom_command(
#       OUTPUT "${lb_proto_srcs}" "${lb_proto_hdrs}" "${lb_grpc_srcs}" "${lb_grpc_hdrs}"
#       COMMAND ${_PROTOBUF_PROTOC}
#       ARGS --grpc_out "${CMAKE_CURRENT_BINARY_DIR}"
#         --cpp_out "${CMAKE_CURRENT_BINARY_DIR}"
#         -I "${lb_proto_path}"
#         --plugin=protoc-gen-grpc="${_GRPC_CPP_PLUGIN_EXECUTABLE}"
#         "${lb_proto}"
#       DEPENDS "${lb_proto}")

# Include generated *.pb.h files
include_directories("${CMAKE_CURRENT_BINARY_DIR}")

# hw_grpc_proto
# Proto file 1
add_library(hw_grpc_proto
  ${hw_grpc_srcs}
  ${hw_grpc_hdrs}
  ${hw_proto_srcs}
  ${hw_proto_hdrs})
target_link_libraries(hw_grpc_proto
  ${_REFLECTION}
  ${_GRPC_GRPCPP}
  ${_PROTOBUF_LIBPROTOBUF})

# # Proto file 2
# add_library(comm_grpc_proto
#   ${comm_grpc_srcs}
#   ${comm_grpc_hdrs}
#   ${comm_proto_srcs}
#   ${comm_proto_hdrs})
# target_link_libraries(comm_grpc_proto
#   ${_REFLECTION}
#   ${_GRPC_GRPCPP}
#   ${_PROTOBUF_LIBPROTOBUF})

# # Proto file 3
# add_library(lb_grpc_proto
#   ${lb_grpc_srcs}
#   ${lb_grpc_hdrs}
#   ${lb_proto_srcs}
#   ${lb_proto_hdrs})
# target_link_libraries(lb_grpc_proto
#   ${_REFLECTION}
#   ${_GRPC_GRPCPP}
#   ${_PROTOBUF_LIBPROTOBUF})


# # TODO Add comm proto to _target only

# # Targets blockstorage_[async_](client|server)
# # foreach(_target
# #   load_balancer)
# #   add_library(lb "load_balancer.cc")
# #   add_executable(${_target} "${_target}.cc")
# #   target_link_libraries(${_target}
# #     lb
# #     hw_grpc_proto
# #     ${_REFLECTION}
# #     ${_GRPC_GRPCPP}
# #     ${_PROTOBUF_LIBPROTOBUF})
# # endforeach()

add_library(client_lib ${CMAKE_CURRENT_SOURCE_DIR}/../src/client.cc)
target_link_libraries(client_lib 
  hw_grpc_proto
)

# foreach(_target
#   blockstorage_client load_balancer)
#   add_executable(${_target} "${_target}.cc")
#   target_link_libraries(${_target}
#     hw_grpc_proto
#     lb_grpc_proto
#     client_lib
#     ${_REFLECTION}
#     ${_GRPC_GRPCPP}
#     ${_PROTOBUF_LIBPROTOBUF})
# endforeach()


# add_library(cr "util/crash_recovery.cc")
# target_link_libraries(cr 
#   hw_grpc_proto
#   comm_grpc_proto
# )

# foreach(_target
#   blockstorage_server )
#   add_library(atl "util/address_translation.cc")
#   add_library(wal "util/wal.cc")
#   add_library(txn "util/txn.cc")
#   add_library(kv "util/kv_store.cc")
#   add_executable(${_target} "${_target}.cc")
#   target_link_libraries(${_target}
#     atl
#     wal
#     txn
#     kv
#     cr
#     hw_grpc_proto
#     comm_grpc_proto
#     lb_grpc_proto
#     ${_REFLECTION}
#     ${_GRPC_GRPCPP}
#     ${_PROTOBUF_LIBPROTOBUF})
# endforeach()

foreach(_target
  rw_perf )
  add_executable(${_target} "${_target}.cc")
  target_link_libraries(${_target}
    pthread
    hw_grpc_proto
    ${_REFLECTION}
    ${_GRPC_GRPCPP}
    ${_PROTOBUF_LIBPROTOBUF})
endforeach()
