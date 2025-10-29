cmake_minimum_required(VERSION 3.25) # 建议 3.25+，boost-cmake 支持更好
project(GateServer LANGUAGES CXX)
set(CMAKE_PREFIX_PATH "/opt/homebrew/opt/mysql-connector-c++")

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 源文件
set(SOURCES
    AsioIOContextPool.cpp
    ConfigMgr.cpp
    CServer.cpp
    GateServer.cpp
    HttpConnection.cpp
    LogicSystem.cpp
    MysqlDao.cpp
    MysqlMgr.cpp
    RedisMgr.cpp
    StatusGrpcClient.cpp
    VerifyGrpcClient.cpp
    message.pb.cc
    message.grpc.pb.cc
)

set(HEADERS
    AsioIOContextPool.h
    ConfigMgr.h
    CServer.h
    HttpConnection.h
    LogicSystem.h
    MysqlDao.h
    MysqlMgr.h
    RedisMgr.h
    Singleton.h
    StatusGrpcClient.h
    VerifyGrpcClient.h
    const.h
    message.pb.h
    message.grpc.pb.h
)

# ---------------- Boost (boost-cmake) ----------------
include(FetchContent)
set(Boost_EDir "/opt/boost-cmake-1.89.0") # 你 clone 下来的 boost-cmake 路径

FetchContent_Declare(
    Boost
    SOURCE_DIR ${Boost_EDir}
    OVERRIDE_FIND_PACKAGE
)
FetchContent_MakeAvailable(Boost)

#find_package(Boost 1.89.0 CONFIG REQUIRED COMPONENTS filesystem asio beast)

# ---------------- gRPC & Protobuf ----------------
find_package(Protobuf CONFIG REQUIRED)
find_package(gRPC CONFIG REQUIRED)

# ---------------- JsonCpp ----------------
find_package(jsoncpp CONFIG REQUIRED)

# ---------------- MySQL Connector/C++ ----------------
find_package(mysql-concpp CONFIG REQUIRED)

# ---------------- hiredis ----------------
set(HIREDIS_HOME "/opt/homebrew")
include_directories(${HIREDIS_HOME}/include)
link_directories(${HIREDIS_HOME}/lib)

# ---------------- 可执行文件 ----------------
add_executable(gateserver ${SOURCES} ${HEADERS})

configure_file(${CMAKE_SOURCE_DIR}/config.ini ${CMAKE_BINARY_DIR}/config.ini COPYONLY)

# ---------------- 链接库 ----------------
target_link_libraries(gateserver
    PRIVATE
        gRPC::grpc++
        protobuf::libprotobuf
        jsoncpp_lib
        mysql::concpp
        Boost::filesystem
        Boost::asio
        Boost::beast
        Boost::property_tree
        hiredis
)
