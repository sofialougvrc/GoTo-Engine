CXX ?= c++
CXXFLAGS ?= -std=c++20 -Wall -Wextra -Wpedantic -Iinclude
BUILD_DIR := work/build
ENGINE_SOURCES := src/catalog.cpp src/coord_transform.cpp src/gpio_interface.cpp src/mount.cpp src/time_utils.cpp src/tracking_loop.cpp
SIDEREAL_TEST_BIN := $(BUILD_DIR)/sidereal_time_tests
ENGINE_TEST_BIN := $(BUILD_DIR)/engine_tests
GPIO_TEST_BIN := $(BUILD_DIR)/gpio_tests
CLI_BIN := $(BUILD_DIR)/goto-engine

.PHONY: all test clean

all: $(SIDEREAL_TEST_BIN) $(ENGINE_TEST_BIN) $(GPIO_TEST_BIN) $(CLI_BIN)

$(SIDEREAL_TEST_BIN): $(ENGINE_SOURCES) tests/sidereal_time_tests.cpp include/gte/time_utils.hpp
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(ENGINE_SOURCES) tests/sidereal_time_tests.cpp -o $(SIDEREAL_TEST_BIN)

$(ENGINE_TEST_BIN): $(ENGINE_SOURCES) tests/engine_tests.cpp include/gte/*.hpp
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(ENGINE_SOURCES) tests/engine_tests.cpp -o $(ENGINE_TEST_BIN)

$(GPIO_TEST_BIN): $(ENGINE_SOURCES) tests/gpio_tests.cpp include/gte/*.hpp
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(ENGINE_SOURCES) tests/gpio_tests.cpp -o $(GPIO_TEST_BIN)

$(CLI_BIN): $(ENGINE_SOURCES) src/goto_engine_cli.cpp include/gte/*.hpp
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(ENGINE_SOURCES) src/goto_engine_cli.cpp -o $(CLI_BIN)

test: $(SIDEREAL_TEST_BIN) $(ENGINE_TEST_BIN) $(GPIO_TEST_BIN)
	$(SIDEREAL_TEST_BIN)
	$(ENGINE_TEST_BIN)
	$(GPIO_TEST_BIN)

clean:
	rm -rf $(BUILD_DIR)
