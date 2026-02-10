#pragma once
#include <string>

namespace minict {

struct OciResult {
    bool ok;
    std::string path;
    std::string detail;
    OciResult(bool v = true, const std::string& p = "", const std::string& d = "")
        : ok(v), path(p), detail(d) {}
};

OciResult load_oci(const std::string& path, const std::string& name, bool simulate);

}
