#include "cgroup.hpp"
#include "util.hpp"
#include <cstdlib>
#include <cerrno>
#ifdef __linux__
#include <unistd.h>
#else
#include <direct.h>
#endif

namespace minict {

static std::string cg_path(const std::string& name, bool sim) {
    if (sim) return state_dir() + "/cgroups/" + name;
    return "/sys/fs/cgroup/minict/" + name;
}

CgroupResult apply_limits(const std::string& name, const Limits& limits, bool sim) {
    std::string path = cg_path(name, sim);

    std::string mem = limits.memory.empty() ? "max" : limits.memory;
    // cpu.max is "quota period" in usec. 50% of a 100ms period => 50000 100000
    std::string cpu = limits.cpu > 0
        ? std::to_string(limits.cpu * 1000) + " 100000"
        : "max 100000";

    if (sim) {
        ensure_dir(state_dir());
        ensure_dir(state_dir() + "/cgroups");
        ensure_dir(path);
        bool ok = write_file(path + "/memory.max", mem) && write_file(path + "/cpu.max", cpu);
        return CgroupResult(ok, path, ok ? "simulated cgroup v2" : "unable to persist simulated limits");
    }

#ifdef __linux__
    if (!ensure_dir("/sys/fs/cgroup/minict") || !ensure_dir(path)) {
        return CgroupResult(false, path, "cannot create cgroup; root privileges may be required");
    }
    // a fresh cgroup has no controllers enabled in its subtree_control, so its
    // children would get no memory.max / cpu.max files at all. Enable the ones
    // we use (idempotent — same as what systemd/runc do).
    write_file("/sys/fs/cgroup/minict/cgroup.subtree_control", "+memory +cpu\n");
    bool ok = write_file(path + "/memory.max", mem) && write_file(path + "/cpu.max", cpu);
    return CgroupResult(ok, path, ok ? "cgroup v2 limits applied" : "unable to write cgroup limits");
#else
    return CgroupResult(false, path, "cgroups require Linux or MINICT_SIM=1");
#endif
}

bool attach_pid(const std::string& name, long pid, bool simulate) {
    std::string p = cg_path(name, simulate) + "/cgroup.procs";
    return write_file(p, std::to_string(pid) + "\n");
}

bool remove_limits(const std::string& name, bool sim) {
    std::string path = cg_path(name, sim);
#ifdef _WIN32
    return _rmdir(path.c_str()) == 0 || errno == ENOENT;
#else
    return ::rmdir(path.c_str()) == 0 || errno == ENOENT;
#endif
}

}
