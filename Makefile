CXX ?= g++
CXXFLAGS ?= -std=c++14 -Wall -Wextra -Iinclude
BUILD := build

all:
	@echo "not wired up yet"

clean:
	rm -rf $(BUILD)

.PHONY: all clean
