#pragma once
#include "config.hpp"
#include <string>

namespace minict {

struct CgroupResult {
    bool ok;
    std::string path;
    std::string detail;
    CgroupResult(bool v = true, const std::string& p = "", const std::string& d = "")
        : ok(v), path(p), detail(d) {}
};

CgroupResult apply_limits(const std::string& name, const Limits& limits, bool simulate);
bool remove_limits(const std::string& name, bool simulate);

}
