#include "daemon.hpp"
#include "ipc.hpp"
#include "runtime.hpp"
#include "rootfs.hpp"
#include "oci.hpp"
#include "config.hpp"
#include "util.hpp"
#include <iostream>
#include <sstream>

#ifdef __linux__
#include <unistd.h>
#include <sys/socket.h>
#endif

namespace minict {

#ifdef __linux__
static std::string handle_request(const std::string& req) {
    std::string op = json_get_string(req, "op");

    if (op == "ps") {
        std::ostringstream body;
        std::vector<Container> all = list_containers();
        body << "NAME\tSTATE\tMEMORY\tCPU\tLATENCY_MS\tCOMMAND\n";
        for (size_t i = 0; i < all.size(); ++i) {
            const Container& c = all[i];
            body << c.name << "\t" << c.state << "\t" << c.memory << "\t"
                 << c.cpu << "\t" << c.latency_ms << "\t" << c.command << "\n";
        }
        return ipc_reply(true, "ok", body.str());
    }

    if (op == "stats") {
        return ipc_reply(true, "ok", status_json());
    }

    if (op == "kill") {
        std::string name = json_get_string(req, "name");
        bool ok = kill_container(name);
        return ipc_reply(ok, ok ? "stopped" : "not found", "");
    }

    if (op == "pull-rootfs") {
        std::string tarball = json_get_string(req, "tarball");
        std::string name = json_get_string(req, "name");
        RootfsResult x = extract_rootfs(tarball, name, simulation_enabled());
        std::string body = x.detail + ": " + x.path;
        return ipc_reply(x.ok, x.detail, body);
    }

    if (op == "load-oci") {
        std::string path = json_get_string(req, "path");
        std::string name = json_get_string(req, "name");
        OciResult x = load_oci(path, name, simulation_enabled());
        std::string body = x.detail + ": " + x.path;
        return ipc_reply(x.ok, x.detail, body);
    }

    if (op == "run") {
        Config c;
        c.name = json_get_string(req, "name");
        c.command = json_get_string(req, "command");
        c.rootfs = json_get_string(req, "image");
        c.limits.memory = json_get_string(req, "memory");
        c.limits.cpu = json_get_int(req, "cpu", 0);

        RunResult x = run_container(c);
        if (x.ok) {
            std::ostringstream body;
            body << "started " << x.container.name << " in " << x.container.latency_ms << "ms";
            return ipc_reply(true, x.detail, body.str());
        }
        return ipc_reply(false, x.detail, "");
    }

    return ipc_reply(false, "unknown op", "");
}

#endif

int run_daemon() {
#ifdef __linux__
    int listen_fd = ipc_listen_socket();
    if (listen_fd < 0) {
        std::cerr << "error: could not bind " << socket_path() << "\n";
        return 1;
    }

    std::cerr << "minict daemon on " << socket_path() << "\n";

    while (true) {
        int client = accept(listen_fd, 0, 0);
        if (client < 0) continue;

        std::string req;
        ipc_read_line(client, req);
        std::string resp = handle_request(req);
        ipc_write_line(client, resp);
        close(client);
    }
#else
    std::cerr << "error: daemon needs Linux or WSL2\n";
    return 1;
#endif
}

}
