#include "cgroup.h"

#include "util.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

/* Inode receipts prevent cleanup from deleting a pre-existing or replaced cgroup. */
static int receipt(const char *path, const char *id, int create) {
    char marker[256], value[64];

    snprintf(marker, sizeof marker, STATE_BASE "/cg-%s", id);

    struct stat st;
    if (lstat(path, &st) || !S_ISDIR(st.st_mode))
        return -1;

    snprintf(value, sizeof value, "%llu\n", (unsigned long long)st.st_ino);

    if (create) {
        int fd = open(marker, O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC | O_NOFOLLOW, 0600);
        if (fd < 0)
            return -1;
        int rc = write(fd, value, strlen(value)) == (ssize_t)strlen(value) ? 0 : -1;
        if (!rc)
            rc = fsync(fd);
        close(fd);
        return rc;
    }

    char saved[64];
    if (read_file(marker, saved, sizeof saved))
        return -1;
    if (strcmp(value, saved)) {
        errno = EEXIST;
        return -1;
    }

    return 0;
}

static void forget(const char *id) {
    char path[256];
    snprintf(path, sizeof path, STATE_BASE "/cg-%s", id);
    (void)unlink(path);
}

static int base_ensure(void) {
    if (mkdir_safe(STATE_BASE, 0700))
        return -1;
    if (!mkdir(CGROUP_BASE, 0700)) {
        if (receipt(CGROUP_BASE, "base", 1)) {
            int saved = errno;
            (void)rmdir(CGROUP_BASE);
            errno = saved;
            return -1;
        }
    } else if (errno != EEXIST || receipt(CGROUP_BASE, "base", 0))
        return -1;
    return write_file(CGROUP_BASE "/cgroup.subtree_control", "+cpu +memory +pids");
}

int cgroup_cleanup(void) {
    if (access(CGROUP_BASE, F_OK) && errno == ENOENT) {
        forget("base");
        return 0;
    }
    if (receipt(CGROUP_BASE, "base", 0) || rmdir(CGROUP_BASE))
        return -1;
    forget("base");
