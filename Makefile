CXX ?= g++
CXXFLAGS ?= -std=c++14 -Wall -Wextra -Iinclude -Ithird_party/minitest
BUILD := build
CORE := src/util.cpp src/namespace.cpp src/cgroup.cpp src/rootfs.cpp src/runtime.cpp
TESTS := tests/test_util.cpp tests/test_cli_parse.cpp

all: $(BUILD)/minict
$(BUILD):
	mkdir -p $(BUILD)
$(BUILD)/minict: $(CORE) src/main.cpp | $(BUILD)
	$(CXX) $(CXXFLAGS) $(CORE) src/main.cpp -o $@
$(BUILD)/minict_tests: $(CORE) third_party/minitest/minitest.cpp $(TESTS) | $(BUILD)
	$(CXX) $(CXXFLAGS) $(CORE) third_party/minitest/minitest.cpp $(TESTS) -o $@
test: $(BUILD)/minict_tests
	MINICT_SIM=1 ./$(BUILD)/minict_tests
clean:
	rm -rf $(BUILD) .minict
.PHONY: all test clean
