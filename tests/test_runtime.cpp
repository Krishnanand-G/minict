#include "minitest.hpp"
#include "runtime.hpp"
#include "util.hpp"

using namespace minict;

static Config sample(const std::string& name) {
    Config c;
    c.name = name;
    c.command = "echo hello";
    c.limits = Limits("64m", 50);
    return c;
}

TEST(Runtime, RunSimulation) {
    EXPECT_TRUE(run_container(sample("rt-one")).ok);
}

TEST(Runtime, HasName) {
    EXPECT_EQ(run_container(sample("rt-two")).container.name, "rt-two");
}

TEST(Runtime, RunningState) {
    EXPECT_EQ(run_container(sample("rt-three")).container.state, "running");
}

TEST(Runtime, RecordsLimit) {
    EXPECT_EQ(run_container(sample("rt-four")).container.memory, "64m");
}

TEST(Runtime, RecordsCpu) {
    EXPECT_EQ(run_container(sample("rt-five")).container.cpu, 50);
}

TEST(Runtime, ListContainsRun) {
    run_container(sample("rt-six"));
    bool found = false;
    std::vector<Container> all = list_containers();
    for (size_t i = 0; i < all.size(); ++i) {
        if (all[i].name == "rt-six") found = true;
    }
    EXPECT_TRUE(found);
}

TEST(Runtime, StatusJson) {
    run_container(sample("rt-seven"));
    EXPECT_TRUE(status_json().find("rt-seven") != std::string::npos);
}

TEST(Runtime, KillTransitionsState) {
    run_container(sample("rt-eight"));
    EXPECT_TRUE(kill_container("rt-eight"));
    std::vector<Container> all = list_containers();
    bool stopped = false;
    for (size_t i = 0; i < all.size(); ++i) {
        if (all[i].name == "rt-eight" && all[i].state == "stopped") stopped = true;
    }
    EXPECT_TRUE(stopped);
}
