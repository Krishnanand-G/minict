#pragma once
#include "config.hpp"
#include <string>
#include <vector>

namespace minict {

struct Container {
    std::string name;
    std::string command;
    std::string state;
    std::string memory;
    std::string rootfs;
    int cpu;
    long long latency_ms;
    long long pid;
};

struct RunResult {
    bool ok;
    Container container;
    std::string detail;
    RunResult() : ok(false) {}
};

RunResult run_container(const Config& config);
std::vector<Container> list_containers();
bool kill_container(const std::string& name);
std::string status_json();

}
