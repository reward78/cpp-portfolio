cmake_minimum_required(VERSION 3.12)

project(
    labwork7
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

enable_testing()

include(FetchContent)

list(APPEND CMAKE_PREFIX_PATH "/opt/homebrew/opt/googletest")

find_package(nlohmann_json CONFIG REQUIRED)
find_package(GTest CONFIG REQUIRED)

find_package(cpr CONFIG QUIET)

if(NOT cpr_FOUND)
    FetchContent_Declare(
        cpr
        GIT_REPOSITORY https://github.com/libcpr/cpr.git
        GIT_TAG 1.11.2
    )
    FetchContent_MakeAvailable(cpr)
endif()

add_library(labwork7_core
    app.cpp
    Path1.cpp
    getcodecity.cpp
    city_code_utils.cpp
    getstationlist.cpp
    GetAPI.cpp
    response_utils.cpp
)

target_link_libraries(labwork7_core PUBLIC
    nlohmann_json::nlohmann_json
    cpr::cpr
)

add_executable(labwork7
    main.cpp
)

target_link_libraries(labwork7 PRIVATE
    labwork7_core
)

add_executable(labwork7_tests
    tests/path1_tests.cpp
)

target_link_libraries(labwork7_tests PRIVATE
    GTest::gtest_main
    labwork7_core
)

add_test(NAME labwork7_tests COMMAND labwork7_tests)

include(tests/coverage.cmake)
