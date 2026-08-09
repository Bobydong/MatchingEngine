cmake_minimum_required(VERSION 3.20)
project(matching_engine CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

add_compile_options(-Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wnon-virtual-dtor)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
  add_link_options(-fsanitize=address,undefined)
endif()

# Core library — add new .cpp files here as you create them
add_library(me_core
  src/orderbook.cpp
)
target_include_directories(me_core PUBLIC include)

# ---------------------------------------------------------------------------
# Tests and benchmarks — DISABLED during the rewrite.
#
# The tests/ and benchmarks/ directories were removed in 8fca1e8. Until they
# exist again, add_subdirectory() aborts configure, which also means no
# compile_commands.json gets written and clangd has nothing to work from.
#
# The FetchContent blocks are commented out too: they clone googletest and
# googlebenchmark on every fresh configure, which is pure wait time while
# there is nothing to link them against.
#
# TO RESTORE: recreate tests/CMakeLists.txt and benchmarks/CMakeLists.txt,
# then uncomment everything below.
# ---------------------------------------------------------------------------

# include(FetchContent)
# FetchContent_Declare(googletest
#   GIT_REPOSITORY https://github.com/google/googletest.git
#   GIT_TAG        v1.14.0
# )
# # Prevent overriding the parent project's compiler/linker settings on Windows
# set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
# FetchContent_MakeAvailable(googletest)
#
# set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
# FetchContent_Declare(googlebenchmark
#     GIT_REPOSITORY https://github.com/google/benchmark.git
#     GIT_TAG        v1.8.4
# )
# FetchContent_MakeAvailable(googlebenchmark)
#
# enable_testing()
# add_subdirectory(tests)
#
# # Benchmarks — wired up in Phase 1
# add_subdirectory(benchmarks)
