cmake_minimum_required(VERSION 3.15)
project(UtilityApprox LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(ExistingAlg
    main.cpp
    UtilityApprox/UtilityApprox.cpp
    Others/data_utility.cpp
    Others/operation.cpp
    exp_stats.cpp
)

target_include_directories(ExistingAlg PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

enable_testing()
add_test(
    NAME util_smoke_test
    COMMAND ExistingAlg 1 util ${CMAKE_CURRENT_SOURCE_DIR}/../datasets/e100-10k.txt 3 1 30 1.0
)
