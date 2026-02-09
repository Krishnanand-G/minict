#include "minitest.hpp"
#include "cgroup.hpp"
#include "util.hpp"

using namespace minict;

TEST(Cgroup, SimulationApplies) {
    EXPECT_TRUE(apply_limits("cg-a", Limits("64m", 50), true).ok);
}

TEST(Cgroup, WritesMemory) {
    apply_limits("cg-memory", Limits("128m", 0), true);
    EXPECT_EQ(read_file(state_dir() + "/cgroups/cg-memory/memory.max"), "128m");
}

TEST(Cgroup, WritesCpu) {
    apply_limits("cg-cpu", Limits("0", 50), true);
    EXPECT_EQ(read_file(state_dir() + "/cgroups/cg-cpu/cpu.max"), "50000 100000");
}

TEST(Cgroup, UnlimitedMemory) {
    apply_limits("cg-max", Limits("", 0), true);
    EXPECT_EQ(read_file(state_dir() + "/cgroups/cg-max/memory.max"), "max");
}

TEST(Cgroup, UnlimitedCpu) {
    apply_limits("cg-cpu-max", Limits("0", 0), true);
    EXPECT_EQ(read_file(state_dir() + "/cgroups/cg-cpu-max/cpu.max"), "max 100000");
}

TEST(Cgroup, ReturnsPath) {
    EXPECT_TRUE(apply_limits("cg-path", Limits(), true).path.find("cg-path") != std::string::npos);
}
