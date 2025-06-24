find_package(Boost REQUIRED COMPONENTS filesystem)

add_executable(test_utils
    sdr/test_utils.cpp
    ../sdr/utils.cpp
)

add_executable(test_pseudorandom_phase
    sdr/test_pseudorandom_phase.cpp
    ../sdr/pseudorandom_phase.cpp
)

target_include_directories(test_utils PRIVATE ../sdr)
target_link_libraries(test_utils
    gtest_main
    Boost::filesystem
)

target_include_directories(test_pseudorandom_phase PRIVATE ../sdr)
target_link_libraries(test_pseudorandom_phase
    gtest_main
    Boost::filesystem
)

include(GoogleTest)
gtest_discover_tests(test_utils)
gtest_discover_tests(test_pseudorandom_phase)
