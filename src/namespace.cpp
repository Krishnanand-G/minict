#include "namespace.hpp"
#include "config.hpp"
#include "util.hpp"

#ifdef __linux__
#include <sched.h>
#include <errno.h>
#include <cstring>
#endif

namespace minict {

NamespaceResult setup_namespaces(bool simulate) {
    if (simulate) {
        ensure_dir(state_dir());
        write_file(state_dir() + "/namespaces.log", "pid,mount,uts,ipc,net\n");
        return NamespaceResult(true, "simulated pid,mount,uts,ipc,net");
    }

#ifdef __linux__
    // needs CAP_SYS_ADMIN; fails loudly if we don't have it
    int flags = CLONE_NEWPID | CLONE_NEWNS | CLONE_NEWUTS | CLONE_NEWIPC | CLONE_NEWNET;
    if (unshare(flags) == 0) {
        return NamespaceResult(true, "pid,mount,uts,ipc,net");
    }
    return NamespaceResult(false, std::strerror(errno));
#else
    return NamespaceResult(false, "namespaces require Linux or MINICT_SIM=1");
#endif
}

}
