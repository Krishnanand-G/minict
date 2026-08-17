#include "minitest.hpp"
#include "sandbox.hpp"
#include "config.hpp"
#include "util.hpp"

using namespace minict;

TEST(Sandbox, SimulationRecordsSteps) {
    SandboxResult r = setup_sandbox("placeholder", true);
    EXPECT_TRUE(r.ok);
    EXPECT_TRUE(read_file(state_dir() + "/sandbox.log").find("seccomp") != std::string::npos);
    EXPECT_TRUE(read_file(state_dir() + "/sandbox.log").find("pivot_root") != std::string::npos);
}

#ifdef __linux__
TEST(Sandbox, BlocklistCoversEscapePaths) {
    std::vector<int> bl = seccomp_blocklist();
    EXPECT_TRUE(bl.size() > 10);
    bool has_mount = false;
    bool has_unshare = false;
    bool has_setns = false;
    bool has_chroot = false;
    for (size_t i = 0; i < bl.size(); ++i) {
        if (bl[i] == 165) has_mount = true;
        if (bl[i] == 272) has_unshare = true;
        if (bl[i] == 308) has_setns = true;
        if (bl[i] == 161) has_chroot = true;
    }
    EXPECT_TRUE(has_mount);
    EXPECT_TRUE(has_unshare);
    EXPECT_TRUE(has_setns);
    EXPECT_TRUE(has_chroot);
}

TEST(Sandbox, BlocklistRejectsOnlyLinuxSyscalls) {
    std::vector<int> bl = seccomp_blocklist();
    for (size_t i = 0; i < bl.size(); ++i) {
        EXPECT_TRUE(bl[i] >= 0 && bl[i] <= 450);
    }
}
#endif
