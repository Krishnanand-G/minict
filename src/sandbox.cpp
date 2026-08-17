#include "sandbox.hpp"
#include "config.hpp"
#include "util.hpp"

#include <cstring>

#ifdef __linux__
#include <sys/mount.h>
#include <sys/prctl.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <linux/filter.h>
#include <linux/seccomp.h>
#include <linux/audit.h>
#include <linux/capability.h>
#include <linux/securebits.h>
#include <unistd.h>
#include <cerrno>
#include <vector>
#endif

namespace minict {

#ifdef __linux__
// covers every capability through current kernels (CAP_LAST_CAP ~ 40)
static const int kMaxCap = 40;
// x86_64 syscall numbers. The filter is built for the architecture it is
// installed on; AUDIT_ARCH_X86_64 = EM_X86_64 | __AUDIT_ARCH_64BIT | __AUDIT_ARCH_LE.
static const int kCapSetpcap = 8;
#endif

static const int kSyscallMount       = 165;
static const int kSyscallUnshare     = 272;
static const int kSyscallSetns       = 308;

std::vector<int> seccomp_blocklist() {
    std::vector<int> out;
#ifdef __linux__
    static const int kBlocked[] = {
        // filesystem escape
        kSyscallMount, 166 /*umount2*/, 155 /*pivot_root*/, 161 /*chroot*/,
        133 /*mknod*/, 303 /*name_to_handle_at*/, 304 /*open_by_handle_at*/,
        // namespace / sandbox escape
        kSyscallUnshare, kSyscallSetns,
        // process injection / introspection
        101 /*ptrace*/, 310 /*process_vm_readv*/, 311 /*process_vm_writev*/,
        298 /*perf_event_open*/, 321 /*bpf*/, 323 /*userfaultfd*/,
        // kernel module / device control
        175 /*init_module*/, 313 /*finit_module*/, 176 /*delete_module*/,
        167 /*swapon*/, 168 /*swapoff*/, 174 /*create_module*/,
        // reboot / kexec
        169 /*reboot*/, 246 /*kexec_load*/, 320 /*kexec_file_load*/,
        // time / audit
        163 /*acct*/, 164 /*settimeofday*/, 227 /*clock_settime*/,
        305 /*clock_adjtime*/, 159 /*adjtimex*/, 103 /*syslog*/,
        // misc
        111 /*vhangup*/, 179 /*quotactl*/,
    };
    for (size_t i = 0; i < sizeof(kBlocked) / sizeof(kBlocked[0]); ++i) {
        out.push_back(kBlocked[i]);
    }
#endif
    return out;
}

#ifdef __linux__
static bool drop_capabilities() {
    // 1. clear the bounding set. PR_CAPBSET_DROP itself needs CAP_SETPCAP in the
    //    effective set, so drop everything else first, set securebits while we
    //    still have CAP_SETPCAP, and drop CAP_SETPCAP last.
    for (int cap = 0; cap <= kMaxCap; ++cap) {
        if (cap == kCapSetpcap) continue;
        if (prctl(PR_CAPBSET_DROP, cap, 0, 0, 0) != 0 && errno != EINVAL) return false;
    }
    // never re-acquire capabilities on exec (setuid binaries, file caps)
    if (prctl(PR_SET_SECUREBITS,
              SECBIT_NOROOT | SECBIT_NOROOT_LOCKED |
              SECBIT_NO_SETUID_FIXUP | SECBIT_NO_SETUID_FIXUP_LOCKED) != 0) return false;
    if (prctl(PR_CAPBSET_DROP, kCapSetpcap, 0, 0, 0) != 0 && errno != EINVAL) return false;

    // 2. empty the effective / permitted / inheritable sets via raw capset.
    struct cap_header { uint32_t version; int pid; };
    struct cap_data   { uint32_t effective; uint32_t permitted; uint32_t inheritable; };
    cap_header hdr;
    cap_data data[2];
    std::memset(&hdr, 0, sizeof(hdr));
    std::memset(data, 0, sizeof(data));
    hdr.version = _LINUX_CAPABILITY_VERSION_3;
    hdr.pid = 0;
    if (syscall(SYS_capset, &hdr, data) != 0) return false;
    return true;
}

static bool install_seccomp() {
    std::vector<sock_filter> prog;
    // arch must be x86_64, otherwise kill. BPF_JEQ jumps pc+1+jt on match and
    // pc+1+jf on mismatch: on match skip the kill (jt=1), on mismatch take it.
    prog.push_back({ BPF_LD | BPF_W | BPF_ABS, 0, 0, 4 });                  // load arch word
    prog.push_back({ BPF_JMP | BPF_JEQ | BPF_K, 1, 0, AUDIT_ARCH_X86_64 }); // match? skip kill
    prog.push_back({ BPF_RET | BPF_K, 0, 0, SECCOMP_RET_KILL_PROCESS });
    // load syscall number, reject each blocked syscall with EPERM
    prog.push_back({ BPF_LD | BPF_W | BPF_ABS, 0, 0, 0 });
    std::vector<int> blocked = seccomp_blocklist();
    for (size_t i = 0; i < blocked.size(); ++i) {
        prog.push_back({ BPF_JMP | BPF_JEQ | BPF_K, 0, 1, (uint32_t)blocked[i] });
        prog.push_back({ BPF_RET | BPF_K, 0, 0, SECCOMP_RET_ERRNO | EPERM });
    }
    prog.push_back({ BPF_RET | BPF_K, 0, 0, SECCOMP_RET_ALLOW });

    sock_fprog fprog;
    fprog.len = (unsigned short)prog.size();
    fprog.filter = prog.data();

    if (prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0) != 0) return false;
    if (syscall(SYS_seccomp, SECCOMP_SET_MODE_FILTER, 0, &fprog) != 0) return false;
    return true;
}

static bool pivot_root_into(const std::string& rootfs) {
    // our / is shared (systemd); without this, the bind mount below would
    // propagate back into the host's mount table. Make everything private.
    if (mount(NULL, "/", NULL, MS_REC | MS_SLAVE, NULL) != 0) return false;
    // pivot_root requires the new root to be a mount point
    if (mount(rootfs.c_str(), rootfs.c_str(), NULL, MS_BIND | MS_REC, NULL) != 0) return false;
    if (chdir(rootfs.c_str()) != 0) return false;
    std::string old = rootfs + "/.minict-oldroot";
    mkdir(old.c_str(), 0755);
    if (syscall(SYS_pivot_root, ".", "./.minict-oldroot") != 0) return false;
    if (chdir("/") != 0) return false;
    if (umount2("/.minict-oldroot", MNT_DETACH) != 0) return false;
    rmdir("/.minict-oldroot");
    return true;
}
#endif

SandboxResult setup_sandbox(const std::string& rootfs, bool simulate) {
    if (simulate) {
        ensure_dir(state_dir());
        write_file(state_dir() + "/sandbox.log",
                   "simulated pivot_root,cap-drop,seccomp,proc-mount\n");
        return SandboxResult(true, "simulated sandbox hardening");
    }

#ifdef __linux__
    if (!rootfs.empty()) {
        if (!pivot_root_into(rootfs)) {
            return SandboxResult(false, "pivot_root failed: " + std::string(std::strerror(errno)));
        }
        // give the container its own proc mount, not the host's
        mkdir("/proc", 0555);
        if (mount("proc", "/proc", "proc", 0, NULL) != 0) {
            return SandboxResult(false, "proc mount failed: " + std::string(std::strerror(errno)));
        }
    }

    if (!drop_capabilities()) {
        return SandboxResult(false, "capability drop failed: " + std::string(std::strerror(errno)));
    }
    if (!install_seccomp()) {
        return SandboxResult(false, "seccomp install failed: " + std::string(std::strerror(errno)));
    }
    return SandboxResult(true, "pivot_root, proc, cap-drop, seccomp applied");
#else
    return SandboxResult(false, "sandbox hardening requires Linux or MINICT_SIM=1");
#endif
}

}
