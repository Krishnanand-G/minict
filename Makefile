CXX ?= g++
CXXFLAGS ?= -std=c++14 -Wall -Wextra -Iinclude -Ithird_party/minitest
BUILD := build
CORE := src/util.cpp

all: $(BUILD)/util.o
$(BUILD):
	mkdir -p $(BUILD)
$(BUILD)/util.o: src/util.cpp include/util.hpp | $(BUILD)
	$(CXX) $(CXXFLAGS) -c src/util.cpp -o $@
clean:
	rm -rf $(BUILD) .minict
.PHONY: all clean
