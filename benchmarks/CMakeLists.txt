add_library(me_bench_support
    order_generator.cpp
)
target_include_directories(me_bench_support PUBLIC ${CMAKE_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(me_bench_support PUBLIC me_core)

add_executable(me_bench
    bench_main.cpp
)
add_executable(me_latency
    latency_dist.cpp
)
target_link_libraries(me_latency PRIVATE
    me_core
    me_bench_support
)

target_link_libraries(me_bench PRIVATE
    me_core
    me_bench_support
    benchmark::benchmark
    benchmark::benchmark_main
)