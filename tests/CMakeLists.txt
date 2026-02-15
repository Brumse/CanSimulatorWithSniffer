find_package(GTest REQUIRED)
include(GoogleTest)

file(GLOB TEST_SOURCES "*.cpp")
add_executable(unit_tests ${TEST_SOURCES})
target_link_libraries(unit_tests
    PRIVATE
    cansim_lib         
    GTest::gtest
    GTest::gtest_main 
)
target_include_directories(unit_tests PRIVATE ${gtest_SOURCE_DIR}/include)