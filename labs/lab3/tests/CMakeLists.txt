include(FetchContent)

FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/releases/download/v1.16.0/googletest-1.16.0.tar.gz
  DOWNLOAD_EXTRACT_TIMESTAMP ON
)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(argparser_tests argparser_tests.cpp)
target_link_libraries(argparser_tests PRIVATE argparser gtest_main)

target_include_directories(argparser_tests PUBLIC ${PROJECT_SOURCE_DIR})

include(GoogleTest)
gtest_discover_tests(argparser_tests)