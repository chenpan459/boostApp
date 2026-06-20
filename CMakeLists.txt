cmake_minimum_required(VERSION 3.16)
project(boostApp VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(NVCOMM_BUILD_TESTS "Build unit tests" ON)
option(NVCOMM_SANITIZE "Enable ASan/UBSan" OFF)

set(NV_NAMESPACE "nvcomm" CACHE STRING "Root C++ namespace")

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

add_library(nvcomm_core STATIC
    src/core/config.cpp
    src/core/logger.cpp
    src/core/application.cpp
    src/core/json_util.cpp
    src/core/http_util.cpp
    src/core/request_id.cpp
    src/platform/paths.cpp
    src/platform/cpu_affinity.cpp
    src/platform/watchdog.cpp
    src/platform/systemd_notify.cpp
    src/infra/persistence/file_event_store.cpp
    src/infra/net/io_context_pool.cpp
    src/infra/net/http_server.cpp
    src/service/gateway/router.cpp
    src/service/gateway/gateway.cpp
    src/service/gateway/middleware/request_id_middleware.cpp
    src/service/gateway/middleware/auth_middleware.cpp
    src/service/gateway/middleware/rate_limit_middleware.cpp
    src/service/gateway/middleware/access_log_middleware.cpp
    src/service/gateway/middleware/body_limit_middleware.cpp
    src/service/gateway/handlers/health_handler.cpp
    src/service/gateway/handlers/packet_handler.cpp
    src/service/gateway_service.cpp
)

target_include_directories(nvcomm_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${Boost_INCLUDE_DIRS}
)

target_compile_definitions(nvcomm_core PUBLIC
    BOOST_ASIO_NO_DEPRECATED
    BOOST_ALL_NO_LIB
    NV_NAMESPACE=${NV_NAMESPACE}
)

target_compile_options(nvcomm_core PRIVATE
    -Wall -Wextra -Wpedantic
)

if(NVCOMM_SANITIZE)
    target_compile_options(nvcomm_core PRIVATE -fsanitize=address,undefined)
    target_link_options(nvcomm_core PUBLIC -fsanitize=address,undefined)
endif()

target_link_libraries(nvcomm_core PUBLIC
    Threads::Threads
    rt
    stdc++fs
)

add_executable(nvcommd src/app/nvcommd/main.cpp)
target_link_libraries(nvcommd PRIVATE nvcomm_core)

if(NVCOMM_BUILD_TESTS)
    enable_testing()
    add_executable(nvcomm_tests
        tests/test_main.cpp
        tests/test_router.cpp
        tests/test_util.cpp
        tests/test_middleware.cpp
    )
    target_include_directories(nvcomm_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/tests
    )
    target_link_libraries(nvcomm_tests PRIVATE nvcomm_core)
    add_test(NAME nvcomm_tests COMMAND nvcomm_tests)
endif()

install(TARGETS nvcommd
    RUNTIME DESTINATION bin
)

install(FILES config/nvcomm.conf
    DESTINATION etc/nvcomm
)

install(FILES config/systemd/nvcommd.service
    DESTINATION lib/systemd/system
)

install(FILES config/nginx/nvcomm.conf
    DESTINATION etc/nginx/conf.d
)
