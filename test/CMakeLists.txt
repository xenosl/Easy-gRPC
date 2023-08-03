add_executable(gRPC-Test)

file(GLOB_RECURSE SOURCES EasyGRPC/*.cpp)
target_sources(gRPC-Test
        PRIVATE ${SOURCES})

find_package(GTest)
target_link_libraries(gRPC-Test
        PRIVATE gRPC GTest::gtest_main)
