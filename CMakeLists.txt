cmake_minimum_required(VERSION 3.16)
project(quick_sort LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
    parlaylib
    GIT_REPOSITORY https://github.com/cmuparlay/parlaylib.git
    GIT_TAG 51017699dcc421f80479cdb238d3092233ad0d26
)
FetchContent_MakeAvailable(parlaylib)

add_compile_options(-Wall -Wextra -Wpedantic)
option(ENABLE_SANITIZERS "Enable ASan and UBSan" OFF)
if(ENABLE_SANITIZERS)
    add_compile_options(
        -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address,undefined)
endif()

add_library(quick_sort sort/sequential_sort.cpp sort/parallel_sort.cpp)
target_include_directories(quick_sort PUBLIC sort)
target_link_libraries(quick_sort PRIVATE parlay)

add_executable(correctness_tests tests/correctness.cpp)
target_link_libraries(correctness_tests PRIVATE quick_sort)

add_executable(benchmark benchmarks/benchmark.cpp)
target_link_libraries(benchmark PRIVATE quick_sort)

enable_testing()
add_test(NAME correctness_seq COMMAND correctness_tests seq)
add_test(NAME correctness_par_1 COMMAND correctness_tests par)
add_test(NAME correctness_par_4 COMMAND correctness_tests par)
set_tests_properties(correctness_seq correctness_par_1 PROPERTIES ENVIRONMENT "PARLAY_NUM_THREADS=1")
set_tests_properties(correctness_par_4 PROPERTIES ENVIRONMENT "PARLAY_NUM_THREADS=4")
