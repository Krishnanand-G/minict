#include "minitest.hpp"
#include "ipc.hpp"

using namespace minict;

TEST(IpcJson, GetString) {
    std::string j = "{\"op\":\"run\",\"name\":\"demo\"}";
    EXPECT_EQ(json_get_string(j, "op"), "run");
    EXPECT_EQ(json_get_string(j, "name"), "demo");
}

TEST(IpcJson, GetInt) {
    std::string j = "{\"cpu\":50}";
    EXPECT_EQ(json_get_int(j, "cpu", 0), 50);
}

TEST(IpcJson, GetBool) {
    std::string j = "{\"ok\":true}";
    EXPECT_TRUE(json_get_bool(j, "ok", false));
}

TEST(IpcJson, ReplyShape) {
    std::string r = ipc_reply(true, "ok", "body text");
    EXPECT_TRUE(r.find("\"ok\":true") != std::string::npos);
    EXPECT_TRUE(r.find("body text") != std::string::npos);
}


TEST(IpcJson, UnescapesNewlines) {
    EXPECT_TRUE(json_get_string("{\"body\":\"one\\ntwo\"}", "body") == "one\ntwo");
}
