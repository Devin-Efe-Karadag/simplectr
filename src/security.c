#include "security.h"

#include <errno.h>
#include <linux/sched.h>
#include <seccomp.h>
#include <sys/capability.h>
#include <sys/prctl.h>

int security_apply(void) {
    /* Rootful, deliberately no capabilities, including CAP_NET_RAW. BusyBox ping uses datagram
     * ICMP. */
    if (prctl(PR_CAP_AMBIENT, PR_CAP_AMBIENT_CLEAR_ALL, 0, 0, 0))
        return -1;
    for (int cap = 0; cap <= CAP_LAST_CAP; cap++)
        if (prctl(PR_CAPBSET_DROP, cap, 0, 0, 0))
            return -1;
    cap_t caps = cap_init();

    if (!caps)
        return -1;
    int rc = cap_set_proc(caps);
    cap_free(caps);

    if (rc)
        return -1;
    if (prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0))
        return -1;
    scmp_filter_ctx ctx = seccomp_init(SCMP_ACT_ERRNO(EPERM));

    if (!ctx)
        return -1;
    const char *allow[] = {"read",
                           "write",
                           "readv",
                           "writev",
                           "pread64",
                           "pwrite64",
                           "close",
                           "close_range",
                           "openat",
                           "openat2",
                           "newfstatat",
                           "fstat",
                           "statx",
                           "lseek",
                           "access",
                           "faccessat",
                           "faccessat2",
                           "mmap",
                           "mprotect",
                           "munmap",
                           "mremap",
                           "madvise",
                           "brk",
                           "msync",
                           "mincore",
                           "rt_sigaction",
                           "rt_sigprocmask",
                           "rt_sigreturn",
                           "rt_sigsuspend",
                           "rt_sigpending",
                           "rt_sigtimedwait",
                           "sigaltstack",
                           "ioctl",
                           "fcntl",
                           "flock",
                           "dup",
                           "dup3",
                           "pipe2",
                           "poll",
                           "ppoll",
                           "pselect6",
                           "epoll_create1",
                           "epoll_ctl",
                           "epoll_pwait",
                           "epoll_pwait2",
                           "getpid",
                           "getppid",
                           "gettid",
                           "getuid",
                           "geteuid",
                           "getgid",
                           "getegid",
                           "getgroups",
                           "set_tid_address",
                           "set_robust_list",
                           "get_robust_list",
                           "rseq",
                           "futex",
                           "futex_waitv",
                           "execve",
                           "exit",
                           "exit_group",
                           "wait4",
                           "waitid",
                           "kill",
                           "tkill",
                           "tgkill",
                           "uname",
                           "getcwd",
                           "chdir",
                           "fchdir",
                           "getdents64",
                           "readlinkat",
                           "mkdirat",
                           "unlinkat",
                           "renameat",
                           "renameat2",
                           "linkat",
                           "symlinkat",
                           "fchmod",
                           "fchmodat",
                           "fchmodat2",
                           "fchown",
                           "fchownat",
                           "umask",
                           "utimensat",
                           "getxattr",
                           "lgetxattr",
                           "fgetxattr",
                           "listxattr",
                           "llistxattr",
                           "flistxattr",
                           "setxattr",
                           "lsetxattr",
                           "fsetxattr",
                           "removexattr",
                           "lremovexattr",
                           "fremovexattr",
                           "truncate",
                           "ftruncate",
                           "fallocate",
                           "fsync",
                           "fdatasync",
                           "sync",
                           "syncfs",
                           "sync_file_range",
                           "statfs",
                           "fstatfs",
                           "clock_gettime",
                           "clock_getres",
                           "clock_nanosleep",
                           "nanosleep",
                           "gettimeofday",
                           "times",
                           "getrusage",
                           "getrlimit",
                           "prlimit64",
                           "getrandom",
                           "sysinfo",
                           "sched_yield",
                           "sched_getaffinity",
                           "sched_getparam",
                           "sched_getscheduler",
                           "socket",
                           "socketpair",
                           "connect",
                           "bind",
                           "listen",
                           "accept",
                           "accept4",
                           "getsockname",
                           "getpeername",
                           "sendto",
                           "recvfrom",
                           "sendmsg",
                           "recvmsg",
                           "sendmmsg",
                           "recvmmsg",
                           "getsockopt",
                           "setsockopt",
                           "shutdown",
                           "sendfile",
                           "splice",
                           "tee",
                           "vmsplice",
                           "copy_file_range",
                           "eventfd2",
                           "timerfd_create",
                           "timerfd_settime",
                           "timerfd_gettime",
                           "getpgid",
                           "getpgrp",
                           "getsid",
                           "setpgid",
                           "setsid",
                           "restart_syscall",
                           "capget",
                           "prctl"};
    for (unsigned i = 0; i < sizeof allow / sizeof allow[0]; i++) {
        int nr = seccomp_syscall_resolve_name(allow[i]);

        if (nr == __NR_SCMP_ERROR)
            continue;
        if (seccomp_rule_add(ctx, SCMP_ACT_ALLOW, nr, 0))
            goto fail;
    }
    /* clone is allowed only without namespace flags. clone3 cannot be safely inspected; ENOSYS
     * enables libc fallback. */
    unsigned long mask = CLONE_NEWUSER | CLONE_NEWNS | CLONE_NEWUTS | CLONE_NEWIPC | CLONE_NEWPID |
                         CLONE_NEWNET | CLONE_NEWCGROUP;
    if (seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(clone), 1,
                         SCMP_A0(SCMP_CMP_MASKED_EQ, mask, 0)) ||
        seccomp_rule_add(ctx, SCMP_ACT_ERRNO(ENOSYS), SCMP_SYS(clone3), 0) || seccomp_load(ctx))

        goto fail;
    seccomp_release(ctx);

    return 0;
fail:
    seccomp_release(ctx);
    errno = EPERM;

    return -1;
}
