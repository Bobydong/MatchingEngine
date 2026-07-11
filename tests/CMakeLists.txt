include(GoogleTest)

add_executable(me_tests
  test_smoke.cpp
  test_types.cpp
  test_order_book.cpp
)

target_link_libraries(me_tests PRIVATE
  me_core
  GTest::gtest
  GTest::gtest_main
)

gtest_discover_tests(me_tests)
