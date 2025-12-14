# Test configuration for simple-rsyncd

# Find Google Test
find_package(GTest QUIET)
if(NOT GTest_FOUND)
    message(STATUS "GTest not found, attempting to download...")
    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.14.0
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
endif()

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
)

# Test executables
add_executable(test_config test_config.cpp)
target_link_libraries(test_config
    simple-rsyncd
    GTest::gtest
    GTest::gtest_main
)

add_executable(test_module test_module.cpp)
target_link_libraries(test_module
    simple-rsyncd
    GTest::gtest
    GTest::gtest_main
)

add_executable(test_protocol test_protocol.cpp)
target_link_libraries(test_protocol
    simple-rsyncd
    GTest::gtest
    GTest::gtest_main
)

add_executable(test_auth test_auth.cpp)
target_link_libraries(test_auth
    simple-rsyncd
    GTest::gtest
    GTest::gtest_main
)

add_executable(test_integration test_integration.cpp)
target_link_libraries(test_integration
    simple-rsyncd
    GTest::gtest
    GTest::gtest_main
)

# Add tests to CTest
add_test(NAME ConfigTests COMMAND test_config)
add_test(NAME ModuleTests COMMAND test_module)
add_test(NAME ProtocolTests COMMAND test_protocol)
add_test(NAME AuthTests COMMAND test_auth)
add_test(NAME IntegrationTests COMMAND test_integration)
