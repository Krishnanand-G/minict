#include "runtime.hpp"
#include "rootfs.hpp"
#include "util.hpp"
#include <cstdlib>
#include <iostream>

using namespace minict;

static void usage() {
    std::cout
        << "minict run [--memory 64m] [--cpu 50] [--name demo] [--rootfs name] command\n"
        << "minict ps | kill NAME | stats | pull-rootfs TARBALL NAME\n";
}

int main(int argc, char** argv) {
    if (argc < 2) {
        usage();
        return 1;
    }

    std::string op = argv[1];

    if (op == "ps") {
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
        std::cout << status_json() << "\n";
        return 0;
    }

    if (op == "kill" && argc == 3) {
        bool ok = kill_container(argv[2]);
        std::cout << (ok ? "stopped\n" : "not found\n");
        return ok ? 0 : 1;
    }

    if (op == "pull-rootfs" && argc == 4) {
        RootfsResult x = extract_rootfs(argv[2], argv[3], simulation_enabled());
        std::cout << x.detail << ": " << x.path << "\n";
        return x.ok ? 0 : 1;
    }

    if (op == "run") {
        Config c;
        c.simulate = simulation_enabled();
        int i = 2;
        for (; i < argc; ++i) {
            std::string v = argv[i];
            if (v == "--memory" && i + 1 < argc) c.limits.memory = argv[++i];
            else if (v == "--cpu" && i + 1 < argc) c.limits.cpu = std::atoi(argv[++i]);
            else if (v == "--name" && i + 1 < argc) c.name = argv[++i];
            else if (v == "--rootfs" && i + 1 < argc) c.rootfs = argv[++i];
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

        RunResult x = run_container(c);
        if (x.ok) {
            std::cout << "started " << x.container.name
                      << " in " << x.container.latency_ms << "ms\n";
            return 0;
        }
        std::cout << "error: " << x.detail << "\n";
        return 1;
    }

    usage();
    return 1;
}
