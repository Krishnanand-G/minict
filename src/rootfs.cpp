#include "rootfs.hpp"
#include "config.hpp"
#include "util.hpp"
#include <cstdlib>
#include <cctype>

namespace minict {

static bool valid_name(const std::string& name) {
    if (name.empty() || name == "." || name == "..") return false;
    for (size_t i = 0; i < name.size(); ++i) {
        unsigned char c = static_cast<unsigned char>(name[i]);
        if (!(std::isalnum(c) || name[i] == '-' || name[i] == '_' || name[i] == '.')) return false;
    }
    return true;
}

std::string rootfs_path(const std::string& name) {
    return state_dir() + "/rootfs/" + name;
}

RootfsResult extract_rootfs(const std::string& tarball, const std::string& name, bool sim) {
    if (!valid_name(name)) return RootfsResult(false, "", "invalid rootfs name");
    std::string path = rootfs_path(name);
    ensure_dir(state_dir());
    ensure_dir(state_dir() + "/rootfs");
    ensure_dir(path);

    if (sim) {
        // don't actually unpack on windows — just leave a breadcrumb
        bool ok = write_file(path + "/.minict-rootfs", "simulated from: " + tarball + "\n");
        return RootfsResult(ok, path, ok ? "rootfs marker created" : "unable to write marker");
    }

#ifdef __linux__
    bool ok = run_process({"tar", "-xf", tarball, "-C", path});
    return RootfsResult(ok, path, ok ? "rootfs extracted" : "tar extraction failed");
#else
    return RootfsResult(false, path, "rootfs extraction requires Linux or MINICT_SIM=1");
#endif
}

}
