#include "minitest.hpp"
#include "oci.hpp"
#include "util.hpp"

using namespace minict;

TEST(OciLoad, SimMarker) {
    OciResult r = load_oci("tests/fixtures/oci-tiny", "oci-test-one", true);
    EXPECT_TRUE(r.ok);
    EXPECT_TRUE(read_file(r.path + "/.minict-rootfs").find("bbbbbbbb") != std::string::npos);
}

TEST(OciLoad, SimPath) {
    OciResult r = load_oci("tests/fixtures/oci-tiny", "oci-test-two", true);
    EXPECT_TRUE(r.path.find("oci-test-two") != std::string::npos);
}


TEST(OciLoad, RejectsUnsafeNames) {
    EXPECT_TRUE(!load_oci("tests/fixtures/oci-tiny", "../escape", true).ok);
    EXPECT_TRUE(!load_oci("tests/fixtures/oci-tiny", "nested/name", true).ok);
}
