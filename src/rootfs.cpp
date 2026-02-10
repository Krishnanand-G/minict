#include "rootfs.hpp"
#include "config.hpp"
#include "util.hpp"
#include <cstdlib>

namespace minict {

std::string rootfs_path(const std::string& name) {
    return state_dir() + "/rootfs/" + name;
}

RootfsResult extract_rootfs(const std::string& tarball, const std::string& name, bool sim) {
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
    std::string cmd = "tar -xf '" + tarball + "' -C '" + path + "'";
    bool ok = std::system(cmd.c_str()) == 0;
    return RootfsResult(ok, path, ok ? "rootfs extracted" : "tar extraction failed");
#else
    return RootfsResult(false, path, "rootfs extraction requires Linux or MINICT_SIM=1");
#endif
}

}
