#pragma once
#include <string>

namespace minict {

struct RootfsResult {
    bool ok;
    std::string path;
    std::string detail;
    RootfsResult(bool v = true, const std::string& p = "", const std::string& d = "")
        : ok(v), path(p), detail(d) {}
};

RootfsResult extract_rootfs(const std::string& tarball, const std::string& name, bool simulate);

}
