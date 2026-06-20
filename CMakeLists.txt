cmake_minimum_required(VERSION 3.16)
project(boostApp VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(NV_NAMESPACE "boostapp" CACHE STRING "Root C++ namespace")

# Third-party libraries install root (see build.sh).
set(BOOSTAPP_LIB_DIR "${CMAKE_SOURCE_DIR}/lib"
    CACHE PATH "Third-party libraries install root")
set(BOOSTAPP_BOOST_ROOT "${BOOSTAPP_LIB_DIR}/boost"
    CACHE PATH "Boost install prefix from source build")
set(Boost_NO_SYSTEM_PATHS ON CACHE BOOL "Do not use system Boost")
set(Boost_ROOT "${BOOSTAPP_BOOST_ROOT}")

if(NOT EXISTS "${Boost_ROOT}/include/boost/version.hpp")
    message(FATAL_ERROR
        "Boost not found at ${Boost_ROOT}\n"
        "Run: ./build.sh boost   (download, compile and install Boost)")
endif()

list(PREPEND CMAKE_PREFIX_PATH "${Boost_ROOT}")

find_package(Threads REQUIRED)
find_package(Boost 1.84 REQUIRED CONFIG)

message(STATUS "Using Boost ${Boost_VERSION} from ${Boost_ROOT}")

add_library(boostapp_core STATIC
    src/core/config.cpp
    src/core/logger.cpp
    src/core/application.cpp
    src/infra/ipc/message_codec.cpp
    src/infra/ipc/message_queue_bus.cpp
    src/service/board_service.cpp
)

target_include_directories(boostapp_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${Boost_INCLUDE_DIRS}
)

target_compile_definitions(boostapp_core PUBLIC
    BOOST_ASIO_NO_DEPRECATED
    BOOST_ALL_NO_LIB
    NV_NAMESPACE=${NV_NAMESPACE}
)

target_link_libraries(boostapp_core PUBLIC
    Threads::Threads
    rt
    stdc++fs
)

function(add_boardcomm_target name source)
    add_executable(${name} ${source})
    target_link_libraries(${name} PRIVATE boostapp_core)
endfunction()

add_boardcomm_target(boardcommd src/app/boardcommd/main.cpp)
add_boardcomm_target(boardcomm_pub src/app/boardcomm_pub/main.cpp)
add_boardcomm_target(boardcomm_sub src/app/boardcomm_sub/main.cpp)

install(TARGETS boardcommd boardcomm_pub boardcomm_sub
    RUNTIME DESTINATION bin
)

install(FILES config/boardcomm.conf
    DESTINATION etc/boardcomm
)
