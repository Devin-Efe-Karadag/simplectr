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
