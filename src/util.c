#include "util.h"

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <sys/utsname.h>
#include <sys/wait.h>
#include <unistd.h>

int mkdir_safe(const char *path, mode_t mode) {
    if (mkdir(path, mode) && errno != EEXIST)
        return -1;
    struct stat st;

    if (lstat(path, &st) || !S_ISDIR(st.st_mode) || st.st_uid != 0 || (st.st_mode & 0022)) {
        errno = EPERM;

        return -1;
    }

    return 0;
}

int path_join(char *out, size_t size, const char *base, const char *leaf) {
    int n = snprintf(out, size, "%s/%s", base, leaf);

    if (n < 0 || (size_t)n >= size) {
        errno = ENAMETOOLONG;

        return -1;
    }

    return 0;
}

int write_file(const char *path, const char *value) {
    int fd = open(path, O_WRONLY | O_CLOEXEC | O_NOFOLLOW);

    if (fd < 0)
        return -1;
    size_t n = strlen(value);

    ssize_t ret;
    do {
        ret = write(fd, value, n);
    } while (ret < 0 && errno == EINTR);
    int saved = errno;
    close(fd);
    errno = saved;

    return ret == (ssize_t)n ? 0 : -1;
}

int read_file(const char *path, char *buf, size_t size) {
    int fd = open(path, O_RDONLY | O_CLOEXEC | O_NOFOLLOW);

    if (fd < 0)
        return -1;
    ssize_t n = read(fd, buf, size - 1);

    int saved = errno;
    close(fd);
    errno = saved;

    if (n < 0)
        return -1;
    buf[n] = 0;

    return 0;
}

int remove_tree(const char *path) {
    /* Caller supplies only a validated runtime-owned directory under a locked base. */

    struct stat st;

    if (lstat(path, &st))
        return errno == ENOENT ? 0 : -1;
    if (!S_ISDIR(st.st_mode))
        return unlink(path);
    DIR *d = opendir(path);

    if (!d)
        return -1;
    int rc = 0;

    struct dirent *e;

    while ((e = readdir(d))) {
        if (!strcmp(e->d_name, ".") || !strcmp(e->d_name, ".."))
            continue;
        char child[PATH_MAX];

        if (path_join(child, sizeof child, path, e->d_name) || remove_tree(child)) {
            rc = -1;
            break;
        }
    }
    closedir(d);

    if (!rc)
        rc = rmdir(path);
    return rc;
}

int run_program(char *const argv[]) {
    pid_t pid = fork();

    if (pid < 0)
        return -1;
    if (!pid) {
        char *env[] = {"PATH=/usr/sbin:/usr/bin:/sbin:/bin", "LC_ALL=C", NULL};
        (void)close_range(3, ~0U, 0);
        execve(argv[0], argv, env);
        _exit(127);
    }

    int status;

    while (waitpid(pid, &status, 0) < 0)
        if (errno != EINTR)
            return -1;
    return WIFEXITED(status) ? WEXITSTATUS(status) : 128 + WTERMSIG(status);
}

int environment_check(void) {
    struct utsname u;

    struct statfs fs;

    char buf[8192];

    if (uname(&u) || strcmp(u.sysname, "Linux") || strcmp(u.machine, "aarch64")) {
        fprintf(stderr, "simplectr requires ARM64 Linux. Build and run inside the Ubuntu VM.\n");

        return -1;
    }

    if (geteuid()) {
        fprintf(stderr, "Run with sudo: rootful namespace setup requires root.\n");

        return -1;
    }

    if (statfs("/sys/fs/cgroup", &fs) || fs.f_type != 0x63677270) {
        fprintf(stderr, "Mount a writable unified cgroup v2 hierarchy at /sys/fs/cgroup.\n");

        return -1;
    }

    if (read_file("/proc/filesystems", buf, sizeof buf) || !strstr(buf, "overlay")) {
        fprintf(stderr, "OverlayFS is unavailable; run sudo modprobe overlay.\n");

        return -1;
    }

    return 0;
}

unsigned long long process_start(pid_t pid) {
    char path[64], buf[4096];
    snprintf(path, sizeof path, "/proc/%ld/stat", (long)pid);

    if (read_file(path, buf, sizeof buf))
        return 0;
    char *p = strrchr(buf, ')');

    if (!p)
        return 0;
    p += 2;

    if (*p == 'Z' || *p == 'X')
        return 0;
    for (int field = 3; field < 22; field++) {
        p = strchr(p, ' ');

        if (!p)
            return 0;
        p++;
    }

    return strtoull(p, NULL, 10);
}
