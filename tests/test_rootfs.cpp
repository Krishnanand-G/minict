#include "minitest.hpp"
#include "rootfs.hpp"
#include "util.hpp"

using namespace minict;

TEST(Rootfs, Simulation) {
    RootfsResult r = extract_rootfs("base.tar", "ubuntu-marker", true);
    EXPECT_TRUE(r.ok);
    EXPECT_TRUE(r.path.find("ubuntu-marker") != std::string::npos);
    EXPECT_TRUE(read_file(r.path + "/.minict-rootfs").find("base.tar") != std::string::npos);
}
