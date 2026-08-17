#include "runtime.hpp"
#include "rootfs.hpp"
#include "oci.hpp"
#include "daemon.hpp"
#include "ipc.hpp"
#include "util.hpp"
#include <cstdlib>
#include <iostream>
#include <sstream>

using namespace minict;

static void usage() {
    std::cout
        << "minict run [--memory 64m] [--cpu 50] [--name demo] [--image name] command\n"
        << "minict ps | kill NAME | stats | daemon | load-oci PATH NAME | pull-rootfs TARBALL NAME\n";
}

static bool ipc_ok(const std::string& resp, std::string& detail, std::string& body) {
    if (resp.empty()) return false;
    bool ok = json_get_bool(resp, "ok", false);
    detail = json_get_string(resp, "detail");
    body = json_get_string(resp, "body");
    return ok;
}

static bool try_ipc(const std::string& req, std::string& detail, std::string& body) {
    if (!socket_exists()) return false;
    std::string resp = ipc_call(req);
    if (resp.empty()) return false;
    bool ok = ipc_ok(resp, detail, body);
    return ok;
}

static std::string run_ipc_json(const Config& c) {
    std::ostringstream o;
    o << "{\"op\":\"run\",\"command\":\"" << json_escape(c.command) << "\"";
    if (!c.name.empty()) o << ",\"name\":\"" << json_escape(c.name) << "\"";
    if (!c.limits.memory.empty()) o << ",\"memory\":\"" << json_escape(c.limits.memory) << "\"";
    if (c.limits.cpu > 0) o << ",\"cpu\":" << c.limits.cpu;
    if (!c.rootfs.empty()) o << ",\"image\":\"" << json_escape(c.rootfs) << "\"";
    o << "}";
    return o.str();
}

int main(int argc, char** argv) {
    if (argc < 2) {
        usage();
        return 1;
    }

    std::string op = argv[1];

    if (op == "daemon") {
        return run_daemon();
    }

    if (op == "ps") {
        std::string detail, body;
        if (try_ipc("{\"op\":\"ps\"}", detail, body)) {
            std::cout << body;
            return 0;
        }
        std::vector<Container> all = list_containers();
        std::cout << "NAME\tSTATE\tMEMORY\tCPU\tLATENCY_MS\tCOMMAND\n";
        for (size_t i = 0; i < all.size(); ++i) {
            const Container& c = all[i];
            std::cout << c.name << "\t" << c.state << "\t" << c.memory << "\t"
                      << c.cpu << "\t" << c.latency_ms << "\t" << c.command << "\n";
        }
        return 0;
    }

    if (op == "stats") {
        std::string detail, body;
        if (try_ipc("{\"op\":\"stats\"}", detail, body)) {
            std::cout << body << "\n";
            return 0;
        }
        std::cout << status_json() << "\n";
        return 0;
    }

    if (op == "kill" && argc == 3) {
        std::string req = std::string("{\"op\":\"kill\",\"name\":\"") + json_escape(argv[2]) + "\"}";
        std::string detail, body;
        if (socket_exists()) {
            std::string resp = ipc_call(req);
            if (!resp.empty()) {
                bool ok = ipc_ok(resp, detail, body);
                std::cout << (ok ? "stopped\n" : "not found\n");
                return ok ? 0 : 1;
            }
        }
        bool ok = kill_container(argv[2]);
        std::cout << (ok ? "stopped\n" : "not found\n");
        return ok ? 0 : 1;
    }

    if (op == "pull-rootfs" && argc == 4) {
        std::ostringstream req;
        req << "{\"op\":\"pull-rootfs\",\"tarball\":\"" << json_escape(argv[2])
            << "\",\"name\":\"" << json_escape(argv[3]) << "\"}";
        std::string detail, body;
        if (socket_exists()) {
            std::string resp = ipc_call(req.str());
            if (!resp.empty()) {
                bool ok = ipc_ok(resp, detail, body);
                if (!body.empty()) std::cout << body << "\n";
                else std::cout << detail << "\n";
                return ok ? 0 : 1;
            }
        }
        RootfsResult x = extract_rootfs(argv[2], argv[3], simulation_enabled());
        std::cout << x.detail << ": " << x.path << "\n";
        return x.ok ? 0 : 1;
    }

    if (op == "load-oci" && argc == 4) {
        std::ostringstream req;
        req << "{\"op\":\"load-oci\",\"path\":\"" << json_escape(argv[2])
            << "\",\"name\":\"" << json_escape(argv[3]) << "\"}";
        std::string detail, body;
        if (socket_exists()) {
            std::string resp = ipc_call(req.str());
            if (!resp.empty()) {
                bool ok = ipc_ok(resp, detail, body);
                if (!body.empty()) std::cout << body << "\n";
                else std::cout << detail << "\n";
                return ok ? 0 : 1;
            }
        }
        OciResult x = load_oci(argv[2], argv[3], simulation_enabled());
        std::cout << x.detail << ": " << x.path << "\n";
        return x.ok ? 0 : 1;
    }

    if (op == "run") {
        Config c;
        int i = 2;
        for (; i < argc; ++i) {
            std::string v = argv[i];
            if (v == "--memory" && i + 1 < argc) c.limits.memory = argv[++i];
            else if (v == "--cpu" && i + 1 < argc) c.limits.cpu = std::atoi(argv[++i]);
            else if (v == "--name" && i + 1 < argc) c.name = argv[++i];
            else if (v == "--image" && i + 1 < argc) c.rootfs = argv[++i];
            else break;
        }
        for (; i < argc; ++i) {
            if (!c.command.empty()) c.command += " ";
            c.command += argv[i];
        }
        if (c.command.empty()) {
            std::cerr << "error: missing command\n";
            return 1;
        }

        if (socket_exists()) {
            std::string detail, body;
            std::string resp = ipc_call(run_ipc_json(c));
            if (!resp.empty() && ipc_ok(resp, detail, body)) {
                std::cout << body << "\n";
                return 0;
            }
            if (!resp.empty()) {
                std::cout << "error: " << detail << "\n";
                return 1;
            }
        }

        // in-process mode: stay attached to the container (docker run semantics)
        c.wait_exit = true;
        RunResult x = run_container(c);
        if (x.ok) {
            std::cout << x.container.name << ": " << x.detail
                      << " (start latency " << x.container.latency_ms << "ms)\n";
            return 0;
        }
        std::cout << "error: " << x.detail << "\n";
        return 1;
    }

    usage();
    return 1;
}
