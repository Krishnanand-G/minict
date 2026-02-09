#include "minitest.hpp"
#include "config.hpp"
#include "namespace.hpp"
#include "util.hpp"

using namespace minict;

TEST(Namespace, SimulationSucceeds) {
    EXPECT_TRUE(setup_namespaces(true).ok);
}

TEST(Namespace, SimulationListsPid) {
    EXPECT_TRUE(setup_namespaces(true).detail.find("pid") != std::string::npos);
}

TEST(Namespace, SimulationListsMount) {
    EXPECT_TRUE(setup_namespaces(true).detail.find("mount") != std::string::npos);
}

TEST(Namespace, SimulationWritesLog) {
    setup_namespaces(true);
    EXPECT_TRUE(read_file(state_dir() + "/namespaces.log").find("net") != std::string::npos);
}

TEST(Namespace, SimulatedDetailNotEmpty) {
    EXPECT_FALSE(setup_namespaces(true).detail.empty());
}
