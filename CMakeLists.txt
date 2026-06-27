cmake_minimum_required(VERSION 3.16)
project(pypilot_cxx VERSION 0.0.1 LANGUAGES CXX)

option(PYPILOT_CXX_BUILD_TESTS "Build pypilot-cxx tests" ON)
option(PYPILOT_CXX_BUILD_EXAMPLES "Build pypilot-cxx examples" ON)

add_library(pypilot_core
    src/pypilot/autopilot.cpp
    src/pypilot/server.cpp
    src/pypilot/client.cpp
    src/pypilot/values.cpp
    src/pypilot/sensors.cpp
    src/pypilot/boatimu.cpp
    src/pypilot/servo.cpp
    src/pypilot/rudder.cpp
    src/pypilot/tacking.cpp
    src/pypilot/nmea.cpp
    src/pypilot/pilots/pilot.cpp
    src/pypilot/pilots/basic.cpp
    src/pypilot/pilots/simple.cpp
    src/pypilot/pilots/gps.cpp
    src/pypilot/pilots/wind.cpp
    src/pypilot/pilots/absolute.cpp
    src/pypilot/pilots/rate.cpp
    src/pypilot/pilots/deadzone.cpp
    src/pypilot/pilots/vmg.cpp
    src/pypilot/pilots/fuzzy.cpp
    src/pypilot/pilots/intellect.cpp
    src/pypilot/pilots/learning.cpp
    src/pypilot/pilots/autotune.cpp
)

target_compile_features(pypilot_core PUBLIC cxx_std_20)
target_include_directories(pypilot_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_compile_definitions(pypilot_core PUBLIC PYPILOT_CXX_VERSION="${PROJECT_VERSION}")

if(PYPILOT_CXX_BUILD_EXAMPLES)
    add_executable(pypilot_autopilot examples/linux_autopilot_main.cpp)
    target_link_libraries(pypilot_autopilot PRIVATE pypilot_core)
endif()

if(PYPILOT_CXX_BUILD_TESTS)
    enable_testing()

    add_executable(test_resolv tests/test_resolv.cpp)
    target_link_libraries(test_resolv PRIVATE pypilot_core)
    add_test(NAME test_resolv COMMAND test_resolv)

    add_executable(test_values tests/test_values.cpp)
    target_link_libraries(test_values PRIVATE pypilot_core)
    add_test(NAME test_values COMMAND test_values)

    add_executable(test_nmea tests/test_nmea.cpp)
    target_link_libraries(test_nmea PRIVATE pypilot_core)
    add_test(NAME test_nmea COMMAND test_nmea)

    add_executable(test_autopilot tests/test_autopilot.cpp)
    target_link_libraries(test_autopilot PRIVATE pypilot_core)
    add_test(NAME test_autopilot COMMAND test_autopilot)

    add_executable(test_servo_protocol tests/test_servo_protocol.cpp)
    target_link_libraries(test_servo_protocol PRIVATE pypilot_core)
    add_test(NAME test_servo_protocol COMMAND test_servo_protocol)
endif()
