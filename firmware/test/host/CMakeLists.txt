# Testes no host (PC) das partes puras do firmware: sem ESP-IDF, sem dependências.
#   cmake -S firmware/test/host -B build-host
#   cmake --build build-host
#   ctest --test-dir build-host -C Debug --output-on-failure
cmake_minimum_required(VERSION 3.16)
project(solar_boat_telemetry_host_tests CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(FW_MAIN ${CMAKE_CURRENT_SOURCE_DIR}/../../main)

add_executable(test_host
    test_host.cpp
    ${FW_MAIN}/drivers/NmeaParser.cpp
)
target_include_directories(test_host PRIVATE ${FW_MAIN}/drivers ${FW_MAIN}/utils)

enable_testing()
add_test(NAME host_tests COMMAND test_host)
