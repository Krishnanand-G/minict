#include "minitest.hpp"
#include "config.hpp"
#include "util.hpp"

using namespace minict;

TEST(Util, TrimSpaces) {
    EXPECT_EQ(trim("  hello \n"), "hello");
}

TEST(Util, TrimEmpty) {
    EXPECT_EQ(trim(" \t"), "");
}

TEST(Util, SplitTwo) {
    EXPECT_EQ(split("a,b", ',').size(), (size_t)2);
}

TEST(Util, SplitThree) {
    EXPECT_EQ(split("a:b:c", ':')[2], "c");
}

TEST(Util, EnsureDirectory) {
    EXPECT_TRUE(ensure_dir(".minict/test-util"));
}

TEST(Util, WriteReadFile) {
    EXPECT_TRUE(write_file(".minict/test-util/value", "ok"));
    EXPECT_EQ(read_file(".minict/test-util/value"), "ok");
}

TEST(Util, JsonEscapesQuotes) {
    EXPECT_EQ(json_escape("a\"b"), "a\\\"b");
}

TEST(Limits, Defaults) {
    Limits x;
    EXPECT_EQ(x.memory, "0");
    EXPECT_EQ(x.cpu, 0);
}

TEST(Limits, Values) {
    Limits x("64m", 50);
    EXPECT_EQ(x.memory, "64m");
    EXPECT_EQ(x.cpu, 50);
}
