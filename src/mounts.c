#include "util.h"
#include <limits.h>
#include <sys/stat.h>
#include <unistd.h>
        return -1;
        errno = EINVAL;
    return 0;
        return -1;
        mount("tmpfs", "/run", "tmpfs", MS_NOSUID | MS_NODEV | MS_NOEXEC, "mode=755,size=16m"))
    struct {
        unsigned minor;
    for (unsigned i = 0; i < sizeof nodes / sizeof nodes[0]; i++) {
        snprintf(path, sizeof path, "/dev/%s", nodes[i].name);
            return -1;
    }
        mount("devpts", "/dev/pts", "devpts", MS_NOSUID | MS_NOEXEC,
        return -1;
        return -1;
        symlink("/proc/self/fd/0", "/dev/stdin") || symlink("/proc/self/fd/1", "/dev/stdout") ||
        return -1;
    const char *readonly[] = {"/proc/sys", "/proc/sysrq-trigger", "/proc/irq", "/proc/bus"};
        if (access(readonly[i], F_OK))
        if (mount(readonly[i], readonly[i], NULL, MS_BIND | MS_REC, NULL) ||
                  MS_BIND | MS_REMOUNT | MS_RDONLY | MS_NOSUID | MS_NODEV | MS_NOEXEC, NULL))
    }
    for (unsigned i = 0; i < sizeof masked / sizeof masked[0]; i++)
            return -1;
    if (fd < 0)
    const char dns[] = "nameserver 1.1.1.1\nnameserver 8.8.8.8\n";
    close(fd);
}
