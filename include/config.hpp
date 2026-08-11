#pragma once
#include <string>

namespace minict {

struct Limits {
    std::string memory;
    int cpu;
    Limits(const std::string& m = "0", int c = 0) : memory(m), cpu(c) {}
};

struct Config {
    std::string name;
    std::string command;
    std::string rootfs;
    Limits limits;
};

bool simulation_enabled();
std::string state_dir();

}
