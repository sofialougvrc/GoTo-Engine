cmake_minimum_required(VERSION 3.16)

project(goto_telescope_engine LANGUAGES CXX)

add_library(gte_engine
    src/catalog.cpp
    src/coord_transform.cpp
    src/gpio_interface.cpp
    src/mount.cpp
    src/stepper_axis.cpp
    src/stepper_mount.cpp
    src/time_utils.cpp
    src/tmc2209_config.cpp
    src/tracking_loop.cpp
    src/uart_interface.cpp
)

target_include_directories(gte_engine
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(gte_engine PUBLIC cxx_std_20)

add_executable(goto-engine
    src/goto_engine_cli.cpp
)

target_link_libraries(goto-engine
    PRIVATE
        gte_engine
)

enable_testing()

add_executable(sidereal_time_tests
    tests/sidereal_time_tests.cpp
)

target_link_libraries(sidereal_time_tests
    PRIVATE
        gte_engine
)

add_test(NAME sidereal_time_tests COMMAND sidereal_time_tests)
set_tests_properties(sidereal_time_tests
    PROPERTIES
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

add_executable(engine_tests
    tests/engine_tests.cpp
)

target_link_libraries(engine_tests
    PRIVATE
        gte_engine
)

add_test(NAME engine_tests COMMAND engine_tests)
set_tests_properties(engine_tests
    PROPERTIES
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

add_executable(gpio_tests
    tests/gpio_tests.cpp
)

target_link_libraries(gpio_tests
    PRIVATE
        gte_engine
)

add_test(NAME gpio_tests COMMAND gpio_tests)
set_tests_properties(gpio_tests
    PROPERTIES
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

add_executable(stepper_tests
    tests/stepper_tests.cpp
)

target_link_libraries(stepper_tests
    PRIVATE
        gte_engine
)

add_test(NAME stepper_tests COMMAND stepper_tests)
set_tests_properties(stepper_tests
    PROPERTIES
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

add_executable(tmc2209_config_tests
    tests/tmc2209_config_tests.cpp
)

target_link_libraries(tmc2209_config_tests
    PRIVATE
        gte_engine
)

add_test(NAME tmc2209_config_tests COMMAND tmc2209_config_tests)
set_tests_properties(tmc2209_config_tests
    PROPERTIES
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
