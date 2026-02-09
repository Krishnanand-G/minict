#include "minitest.hpp"
#include "config.hpp"
#include "runtime.hpp"
#include "util.hpp"

using namespace minict;

TEST(CliParse, DefaultLimits) {
    Limits x;
    EXPECT_EQ(x.memory, "0");
}

TEST(CliParse, MemoryLimit) {
    Limits x("64m", 0);
    EXPECT_EQ(x.memory, "64m");
}

TEST(CliParse, CpuLimit) {
    Limits x("0", 50);
    EXPECT_EQ(x.cpu, 50);
}

TEST(CliParse, EmptyConfigSimFalse) {
    Config c;
    EXPECT_FALSE(c.simulate);
}

TEST(CliParse, MissingCommandFails) {
    Config c;
    c.simulate = true;
    RunResult r = run_container(c);
    EXPECT_FALSE(r.ok);
}

TEST(CliParse, GeneratedName) {
    Config c;
    c.simulate = true;
    c.command = "echo hi";
    RunResult r = run_container(c);
    EXPECT_TRUE(r.ok);
    EXPECT_TRUE(r.container.name.find("job-") == 0);
}

TEST(CliParse, JsonHasContainers) {
    EXPECT_TRUE(status_json().find("containers") != std::string::npos);
}

int main() {
    return minitest::RUN_ALL_TESTS();
}
