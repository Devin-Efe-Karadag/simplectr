#include "userns.h"

#include "util.h"

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <grp.h>
#include <pwd.h>
#include <sched.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/prctl.h>
#include <sys/stat.h>
#include <unistd.h>

static int subrange(const char *file, const char *name, uint32_t *base) {
    FILE *f = fopen(file, "re");

    if (!f)
        return -1;
    char line[256], owner[128];

    unsigned long long start, count;

    int rc = -1;

    while (fgets(line, sizeof line, f)) {
        if (sscanf(line, "%127[^:]:%llu:%llu", owner, &start, &count) == 3 &&
            !strcmp(owner, name) && start >= 65536 && start <= UINT32_MAX - 65536 &&
            count >= 65536) {
            *base = (uint32_t)start;
            rc = 0;
            break;
        }
    }
    fclose(f);

    if (rc)
        errno = EINVAL;
    return rc;
}

int userns_resolve(struct config *c) {
    if (!c->map_user[0])
        return 0;
    if (!getpwnam(c->map_user) || subrange("/etc/subuid", c->map_user, &c->uid_base) ||
        subrange("/etc/subgid", c->map_user, &c->gid_base)) {
        fprintf(stderr, "--userns requires a host account with at least 65536 subordinate UIDs and "
                        "GIDs in /etc/subuid and /etc/subgid.\n");
        return -1;
    }

    return 0;
}

/* Shift the private overlay, never the cached image; do not follow symlinks or cross mounts. */
static int shift_dir(int fd, dev_t device, const struct config *c) {
    DIR *d = fdopendir(dup(fd));

    if (!d)
        return -1;
    struct dirent *e;

    int rc = 0;

    while ((e = readdir(d))) {
        if (!strcmp(e->d_name, ".") || !strcmp(e->d_name, ".."))
            continue;
        struct stat st;

        if (fstatat(fd, e->d_name, &st, AT_SYMLINK_NOFOLLOW)) {
            rc = -1;
            break;
        }

        if (st.st_dev != device)
            continue;
        if (st.st_uid >= 65536 || st.st_gid >= 65536) {
            errno = ERANGE;
            rc = -1;
            break;
        }

        if (S_ISDIR(st.st_mode)) {
            int child = openat(fd, e->d_name, O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);

            if (child < 0) {
                rc = -1;
                break;
            }
            rc = shift_dir(child, device, c);
            close(child);

            if (rc)
                break;
        }

        if (fchownat(fd, e->d_name, c->uid_base + st.st_uid, c->gid_base + st.st_gid,
                     AT_SYMLINK_NOFOLLOW)) {
            rc = -1;
            break;
        }
    }
    closedir(d);

    return rc;
}

static int shift_root(const struct config *c) {
    const char *roots[] = {"/", "/dev", "/tmp", "/run", "/dev/shm"};

    for (unsigned i = 0; i < sizeof roots / sizeof roots[0]; i++) {
        int fd = open(roots[i], O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);

        if (fd < 0)
            return -1;
        struct stat st;

        int rc = fstat(fd, &st);

        if (!rc)
            rc = shift_dir(fd, st.st_dev, c);
        if (!rc)
            rc = fchown(fd, c->uid_base, c->gid_base);
        close(fd);

        if (rc)
            return -1;
    }

    return 0;
}

int userns_child(const struct config *c, int gate) {
    if (!c->uid_base)
        return 0;
    if (shift_root(c) || setgroups(0, NULL) || unshare(CLONE_NEWUSER))
        return -1;
    char ready = 'U';

    if (write(gate, &ready, 1) != 1 || read(gate, &ready, 1) != 1 || ready != 'M')
        return -1;
    if (setresgid(0, 0, 0) || setresuid(0, 0, 0))
        return -1;
    /* Credential changes clear PDEATHSIG; re-arm before the final parent handshake. */

    return prctl(PR_SET_PDEATHSIG, SIGKILL);
}

int userns_map(const struct config *c, pid_t pid) {
    char path[64], value[64];
    snprintf(path, sizeof path, "/proc/%ld/setgroups", (long)pid);

    if (write_file(path, "deny"))
        return -1;
    snprintf(path, sizeof path, "/proc/%ld/uid_map", (long)pid);
    snprintf(value, sizeof value, "0 %u 65536\n", c->uid_base);

    if (write_file(path, value))
        return -1;
    snprintf(path, sizeof path, "/proc/%ld/gid_map", (long)pid);
    snprintf(value, sizeof value, "0 %u 65536\n", c->gid_base);

    return write_file(path, value);
}
