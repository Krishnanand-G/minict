#include "minitest.hpp"
#include "config.hpp"
#include "rootfs.hpp"
#include "util.hpp"

using namespace minict;

TEST(Rootfs, SimulationExtracts) {
    EXPECT_TRUE(extract_rootfs("ubuntu.tar", "ubuntu-a", true).ok);
}

TEST(Rootfs, ReturnsPath) {
    EXPECT_TRUE(extract_rootfs("ubuntu.tar", "ubuntu-b", true).path.find("ubuntu-b") != std::string::npos);
}

TEST(Rootfs, CreatesMarker) {
    extract_rootfs("base.tar", "ubuntu-marker", true);
    EXPECT_TRUE(read_file(state_dir() + "/rootfs/ubuntu-marker/.minict-rootfs").find("base.tar") != std::string::npos);
}

TEST(Rootfs, MarkerSaysSimulated) {
    extract_rootfs("x", "ubuntu-sim", true);
    EXPECT_TRUE(read_file(state_dir() + "/rootfs/ubuntu-sim/.minict-rootfs").find("simulated") != std::string::npos);
}

TEST(Rootfs, DifferentNames) {
    RootfsResult a = extract_rootfs("a", "r-one", true);
    RootfsResult b = extract_rootfs("b", "r-two", true);
    EXPECT_FALSE(a.path == b.path);
}
