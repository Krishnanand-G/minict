CXX ?= g++
CXXFLAGS ?= -std=c++14 -Wall -Wextra -Iinclude -Ithird_party/minitest
BUILD := build
CORE := src/util.cpp src/namespace.cpp src/cgroup.cpp src/rootfs.cpp src/runtime.cpp

all: $(BUILD)/minict
$(BUILD):
	mkdir -p $(BUILD)
$(BUILD)/minict: $(CORE) src/main.cpp | $(BUILD)
	$(CXX) $(CXXFLAGS) $(CORE) src/main.cpp -o $@
clean:
	rm -rf $(BUILD) .minict
.PHONY: all clean
