#pragma once
#include <string>
#include <vector>

namespace minict {

struct SandboxResult {
    bool ok;
    std::string detail;
    SandboxResult(bool v = true, const std::string& d = "") : ok(v), detail(d) {}
};

// Real mode: pivot_root into the rootfs, mount /proc, drop all capabilities,
// then install a seccomp filter. Sim mode records the steps under state_dir().
SandboxResult setup_sandbox(const std::string& rootfs, bool simulate);

// Linux-only: syscall numbers the seccomp filter rejects with EPERM.
// Empty on non-Linux platforms (and in sim the filter is never installed).
std::vector<int> seccomp_blocklist();

}
