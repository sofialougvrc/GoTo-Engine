CXX ?= c++
CXXFLAGS ?= -std=c++20 -Wall -Wextra -Wpedantic -Iinclude
BUILD_DIR := work/build
ENGINE_SOURCES := src/catalog.cpp src/coord_transform.cpp src/gpio_interface.cpp src/mount.cpp src/step_calibration.cpp src/stepper_axis.cpp src/stepper_mount.cpp src/time_utils.cpp src/tmc2209_config.cpp src/tracking_loop.cpp src/uart_interface.cpp
SIDEREAL_TEST_BIN := $(BUILD_DIR)/sidereal_time_tests
ENGINE_TEST_BIN := $(BUILD_DIR)/engine_tests
GPIO_TEST_BIN := $(BUILD_DIR)/gpio_tests
STEP_CALIBRATION_TEST_BIN := $(BUILD_DIR)/step_calibration_tests
STEPPER_TEST_BIN := $(BUILD_DIR)/stepper_tests
TMC2209_CONFIG_TEST_BIN := $(BUILD_DIR)/tmc2209_config_tests
CLI_BIN := $(BUILD_DIR)/goto-engine

.PHONY: all test clean

all: $(SIDEREAL_TEST_BIN) $(ENGINE_TEST_BIN) $(GPIO_TEST_BIN) $(STEP_CALIBRATION_TEST_BIN) $(STEPPER_TEST_BIN) $(TMC2209_CONFIG_TEST_BIN) $(CLI_BIN)

$(SIDEREAL_TEST_BIN): $(ENGINE_SOURCES) tests/sidereal_time_tests.cpp include/gte/time_utils.hpp
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(ENGINE_SOURCES) tests/sidereal_time_tests.cpp -o $(SIDEREAL_TEST_BIN)

$(ENGINE_TEST_BIN): $(ENGINE_SOURCES) tests/engine_tests.cpp include/gte/*.hpp
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(ENGINE_SOURCES) tests/engine_tests.cpp -o $(ENGINE_TEST_BIN)

$(GPIO_TEST_BIN): $(ENGINE_SOURCES) tests/gpio_tests.cpp include/gte/*.hpp
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(ENGINE_SOURCES) tests/gpio_tests.cpp -o $(GPIO_TEST_BIN)

$(STEP_CALIBRATION_TEST_BIN): $(ENGINE_SOURCES) tests/step_calibration_tests.cpp include/gte/*.hpp
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(ENGINE_SOURCES) tests/step_calibration_tests.cpp -o $(STEP_CALIBRATION_TEST_BIN)

$(STEPPER_TEST_BIN): $(ENGINE_SOURCES) tests/stepper_tests.cpp include/gte/*.hpp
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(ENGINE_SOURCES) tests/stepper_tests.cpp -o $(STEPPER_TEST_BIN)

$(TMC2209_CONFIG_TEST_BIN): $(ENGINE_SOURCES) tests/tmc2209_config_tests.cpp include/gte/*.hpp
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(ENGINE_SOURCES) tests/tmc2209_config_tests.cpp -o $(TMC2209_CONFIG_TEST_BIN)

$(CLI_BIN): $(ENGINE_SOURCES) src/goto_engine_cli.cpp include/gte/*.hpp
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(ENGINE_SOURCES) src/goto_engine_cli.cpp -o $(CLI_BIN)

test: $(SIDEREAL_TEST_BIN) $(ENGINE_TEST_BIN) $(GPIO_TEST_BIN) $(STEP_CALIBRATION_TEST_BIN) $(STEPPER_TEST_BIN) $(TMC2209_CONFIG_TEST_BIN)
	$(SIDEREAL_TEST_BIN)
	$(ENGINE_TEST_BIN)
	$(GPIO_TEST_BIN)
	$(STEP_CALIBRATION_TEST_BIN)
	$(STEPPER_TEST_BIN)
	$(TMC2209_CONFIG_TEST_BIN)

clean:
	rm -rf $(BUILD_DIR)
