cmake_minimum_required(VERSION 3.2)
project(rpmsg-netlink-client)

SET (CMAKE_C_COMPILER_WORKS 1)
SET (CMAKE_CXX_COMPILER_WORKS 1)

add_executable(rpmsg-netlink-client
    main.cpp
    netlink_client.cpp
    netlink_tester.cpp
)