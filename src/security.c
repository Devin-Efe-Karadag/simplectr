#include "security.h"

#include <errno.h>
#include <linux/sched.h>
#include <sys/capability.h>
int security_apply(void) {
     * ICMP. */
        return -1;
        if (prctl(PR_CAPBSET_DROP, cap, 0, 0, 0))
            return -1;
    if (!caps)
    int rc = cap_set_proc(caps);
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
                           "openat",
                           "newfstatat",
                           "statx",
                           "access",
                           "faccessat2",
                           "mprotect",
                           "mremap",
                           "brk",
                           "mincore",
                           "rt_sigprocmask",
                           "rt_sigsuspend",
                           "rt_sigtimedwait",
                           "ioctl",
                           "flock",
                           "dup3",
                           "poll",
                           "pselect6",
                           "epoll_ctl",
                           "epoll_pwait2",
                           "getppid",
                           "getuid",
                           "getgid",
                           "getgroups",
                           "set_robust_list",
                           "rseq",
                           "futex_waitv",
                           "exit",
                           "wait4",
                           "kill",
                           "tgkill",
                           "getcwd",
                           "fchdir",
                           "readlinkat",
                           "unlinkat",
                           "renameat2",
                           "fchmod",
                           "fchmodat2",
                           "fchownat",
                           "utimensat",
                           "getxattr",
                           "fgetxattr",
                           "llistxattr",
                           "setxattr",
                           "fsetxattr",
                           "lremovexattr",
                           "truncate",
                           "fallocate",
                           "fdatasync",
                           "syncfs",
                           "statfs",
                           "clock_gettime",
                           "clock_nanosleep",
                           "gettimeofday",
                           "getrusage",
                           "prlimit64",
                           "sysinfo",
                           "sched_getaffinity",
                           "sched_getscheduler",
                           "socketpair",
                           "bind",
                           "accept",
                           "getsockname",
                           "sendto",
                           "sendmsg",
                           "sendmmsg",
                           "getsockopt",
                           "shutdown",
                           "splice",
                           "vmsplice",
                           "eventfd2",
                           "timerfd_settime",
                           "getpgid",
                           "getsid",
                           "setsid",
                           "capget",
    for (unsigned i = 0; i < sizeof allow / sizeof allow[0]; i++) {
        if (nr == __NR_SCMP_ERROR)
        if (seccomp_rule_add(ctx, SCMP_ACT_ALLOW, nr, 0))
    }
     * enables libc fallback. */
                         CLONE_NEWNET | CLONE_NEWCGROUP;
                         SCMP_A0(SCMP_CMP_MASKED_EQ, mask, 0)) ||
        goto fail;
    return 0;
    seccomp_release(ctx);
    return -1;
