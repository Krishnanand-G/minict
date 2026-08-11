#include "minitest.hpp"
#include "config.hpp"
#include "namespace.hpp"
#include "util.hpp"

using namespace minict;

TEST(Namespace, Simulation) {
    NamespaceResult r = setup_namespaces(true);
    EXPECT_TRUE(r.ok);
    EXPECT_TRUE(r.detail.find("pid") != std::string::npos);
    EXPECT_TRUE(read_file(state_dir() + "/namespaces.log").find("net") != std::string::npos);
}
