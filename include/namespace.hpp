#pragma once
#include <string>

namespace minict {

struct NamespaceResult {
    bool ok;
    std::string detail;
    NamespaceResult(bool v = true, const std::string& d = "") : ok(v), detail(d) {}
};

NamespaceResult setup_namespaces(bool simulate);

}
