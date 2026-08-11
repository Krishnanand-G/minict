#include "runtime.hpp"
#include "namespace.hpp"
#include "cgroup.hpp"
#include "rootfs.hpp"
#include "util.hpp"
#include <cstdlib>
#include <sstream>

#ifdef __linux__
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#endif

namespace minict {

static std::string db() {
    return state_dir() + "/containers.tsv";
}

static std::string row(const Container& c) {
    return c.name + "\t" + c.command + "\t" + c.state + "\t" + c.memory + "\t"
        + std::to_string(c.cpu) + "\t" + std::to_string(c.latency_ms) + "\t"
        + std::to_string(c.pid) + "\t" + c.rootfs + "\n";
}

static Container parse(const std::string& line) {
    std::vector<std::string> p = split(line, '\t');
    Container c;
    c.cpu = 0;
    c.latency_ms = 0;
    c.pid = 0;
    if (p.size() >= 7) {
        c.name = p[0];
        c.command = p[1];
        c.state = p[2];
        c.memory = p[3];
        c.cpu = std::atoi(p[4].c_str());
        c.latency_ms = std::atoll(p[5].c_str());
        c.pid = std::atoll(p[6].c_str());
        if (p.size() > 7) c.rootfs = p[7];
    }
    return c;
}

std::vector<Container> list_containers() {
    std::vector<Container> out;
    std::stringstream ss(read_file(db()));
    std::string line;
    while (std::getline(ss, line)) {
        if (!line.empty()) out.push_back(parse(line));
    }
    return out;
}

static void save(const std::vector<Container>& cs) {
    ensure_dir(state_dir());
    std::string body;
    for (size_t i = 0; i < cs.size(); ++i) body += row(cs[i]);
    write_file(db(), body);
    write_file(state_dir() + "/status.json", status_json());
}

RunResult run_container(const Config& in) {
    RunResult result;
    Config c = in;

    if (c.name.empty()) c.name = "job-" + std::to_string(now_ms());
    if (c.command.empty()) {
        result.detail = "command required";
        return result;
    }

    bool sim = simulation_enabled();
    long long start = now_ms();

    NamespaceResult ns = setup_namespaces(sim);
    if (!ns.ok) {
        result.detail = ns.detail;
        return result;
    }

    CgroupResult cg = apply_limits(c.name, c.limits, sim);
    if (!cg.ok) {
        result.detail = cg.detail;
        return result;
    }

#ifdef __linux__
    if (!c.rootfs.empty() && !sim) {
        if (access(rootfs_path(c.rootfs).c_str(), F_OK) != 0) {
            result.detail = "rootfs not found: " + c.rootfs;
            return result;
        }
    }
#endif

    Container x;
    x.name = c.name;
    x.command = c.command;
    x.state = "running";
    x.memory = c.limits.memory.empty() ? "max" : c.limits.memory;
    x.cpu = c.limits.cpu;
    x.rootfs = c.rootfs;
    x.pid = 0;

#ifdef __linux__
    if (!sim) {
        pid_t pid = fork();
        if (pid < 0) {
            result.detail = "fork failed";
            return result;
        }
        if (pid == 0) {
            if (!c.rootfs.empty()) {
                std::string rp = rootfs_path(c.rootfs);
                if (chroot(rp.c_str()) != 0) _exit(126);
                if (chdir("/") != 0) _exit(126);
            }
            execl("/bin/sh", "sh", "-c", c.command.c_str(), (char*)0);
            _exit(127);
        }
        x.pid = pid;
    }
#endif

    x.latency_ms = now_ms() - start;

    std::vector<Container> all = list_containers();
    all.push_back(x);
    save(all);

    result.ok = true;
    result.container = x;
    result.detail = sim ? "simulated start complete" : "container started";
    return result;
}

bool kill_container(const std::string& name) {
    std::vector<Container> all = list_containers();
    bool found = false;

    for (size_t i = 0; i < all.size(); ++i) {
        if (all[i].name != name) continue;
        all[i].state = "stopped";
        found = true;
#ifdef __linux__
        if (all[i].pid > 0) ::kill((pid_t)all[i].pid, SIGTERM);
#endif
        remove_limits(name, simulation_enabled());
    }

    if (found) save(all);
    return found;
}

std::string status_json() {
    std::vector<Container> all = list_containers();
    std::ostringstream o;
    o << "{\"runtime\":\"minict\",\"simulation\":"
      << (simulation_enabled() ? "true" : "false")
      << ",\"containers\":[";

    for (size_t i = 0; i < all.size(); ++i) {
        if (i) o << ",";
        const Container& c = all[i];
        o << "{\"name\":\"" << json_escape(c.name)
          << "\",\"command\":\"" << json_escape(c.command)
          << "\",\"state\":\"" << c.state
          << "\",\"memory\":\"" << c.memory
          << "\",\"cpu\":" << c.cpu
          << ",\"latency_ms\":" << c.latency_ms << "}";
    }
    o << "]}";
    return o.str();
}

}
