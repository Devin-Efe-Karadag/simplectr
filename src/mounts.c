#include "mounts.h"

#include "util.h"

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/sysmacros.h>
#include <unistd.h>

static int directory(const char *p, mode_t mode) {
    if (mkdir(p, mode) && errno != EEXIST)
        return -1;
    struct stat st;

    if (lstat(p, &st) || !S_ISDIR(st.st_mode)) {
        errno = EINVAL;

        return -1;
    }

    return 0;
}

int mounts_enter(const char *base) {
    char merged[PATH_MAX];

    if (path_join(merged, sizeof merged, base, "merged") || chdir(merged))
        return -1;
    if (directory(".oldroot", 0700) || syscall(SYS_pivot_root, ".", ".oldroot") || chdir("/"))
        return -1;
    if (umount2("/.oldroot", MNT_DETACH) || rmdir("/.oldroot"))
        return -1;
    if (directory("/proc", 0555) || directory("/dev", 0755) || directory("/tmp", 01777) ||
        directory("/run", 0755))

        return -1;
    if (mount("proc", "/proc", "proc", MS_NOSUID | MS_NODEV | MS_NOEXEC, NULL) ||
        mount("tmpfs", "/dev", "tmpfs", MS_NOSUID | MS_NOEXEC, "mode=755,size=4m") ||
        mount("tmpfs", "/tmp", "tmpfs", MS_NOSUID | MS_NODEV, "mode=1777,size=64m") ||
        mount("tmpfs", "/run", "tmpfs", MS_NOSUID | MS_NODEV | MS_NOEXEC, "mode=755,size=16m"))

        return -1;

    struct {
        const char *name;

        unsigned minor;
    } nodes[] = {{"null", 3}, {"zero", 5}, {"full", 7}, {"random", 8}, {"urandom", 9}};

    for (unsigned i = 0; i < sizeof nodes / sizeof nodes[0]; i++) {
        char path[64];
        snprintf(path, sizeof path, "/dev/%s", nodes[i].name);

        if (mknod(path, S_IFCHR | 0666, makedev(1, nodes[i].minor)) || chmod(path, 0666))
            return -1;
    }

    if (directory("/dev/pts", 0755) || directory("/dev/shm", 01777) ||
        mount("devpts", "/dev/pts", "devpts", MS_NOSUID | MS_NOEXEC,
              "newinstance,ptmxmode=0666,mode=0620") ||
        mount("tmpfs", "/dev/shm", "tmpfs", MS_NOSUID | MS_NODEV | MS_NOEXEC, "mode=1777,size=32m"))

        return -1;
    if (mknod("/dev/tty", S_IFCHR | 0666, makedev(5, 0)) || chmod("/dev/tty", 0666))
        return -1;
    if (symlink("pts/ptmx", "/dev/ptmx") || symlink("/proc/self/fd", "/dev/fd") ||
        symlink("/proc/self/fd/0", "/dev/stdin") || symlink("/proc/self/fd/1", "/dev/stdout") ||
        symlink("/proc/self/fd/2", "/dev/stderr"))

        return -1;
    /* Prevent access to mutable host-wide proc knobs and sensitive proc files. */

    const char *readonly[] = {"/proc/sys", "/proc/sysrq-trigger", "/proc/irq", "/proc/bus"};

    for (unsigned i = 0; i < sizeof readonly / sizeof readonly[0]; i++) {
        if (access(readonly[i], F_OK))
            continue;
        if (mount(readonly[i], readonly[i], NULL, MS_BIND | MS_REC, NULL) ||
            mount(NULL, readonly[i], NULL,
                  MS_BIND | MS_REMOUNT | MS_RDONLY | MS_NOSUID | MS_NODEV | MS_NOEXEC, NULL))
            return -1;
    }

    const char *masked[] = {"/proc/kcore", "/proc/keys", "/proc/timer_list", "/proc/interrupts"};

    for (unsigned i = 0; i < sizeof masked / sizeof masked[0]; i++)
        if (!access(masked[i], F_OK) && mount("/dev/null", masked[i], NULL, MS_BIND, NULL))
            return -1;
    int fd = open("/etc/resolv.conf", O_WRONLY | O_CREAT | O_TRUNC | O_NOFOLLOW | O_CLOEXEC, 0644);

    if (fd < 0)
        return -1;
    const char dns[] = "nameserver 1.1.1.1\nnameserver 8.8.8.8\n";

    ssize_t n = write(fd, dns, sizeof dns - 1);
    close(fd);

    return n == (ssize_t)(sizeof dns - 1) ? 0 : -1;
}
