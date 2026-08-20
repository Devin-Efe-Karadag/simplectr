#include "state.h"

#include "util.h"

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/file.h>
#include <sys/random.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#define MAGIC 0x4b454c33U

int state_id_valid(const char *id) {
    if (strlen(id) != 16)
        return 0;
    for (unsigned i = 0; i < 16; i++)
        if (!((id[i] >= '0' && id[i] <= '9') || (id[i] >= 'a' && id[i] <= 'f')))
            return 0;
    return 1;
}

int state_lock(void) {
    if (mkdir_safe(STATE_BASE, 0700))
        return -1;
    int fd = open(STATE_BASE "/lock", O_RDWR | O_CREAT | O_CLOEXEC | O_NOFOLLOW, 0600);

    if (fd < 0)
        return -1;
    if (flock(fd, LOCK_EX)) {
        close(fd);

        return -1;
    }

    return fd;
}

int state_path(const struct state *s, const char *leaf, char path[PATH_MAX]) {
    if (!state_id_valid(s->id)) {
        errno = EINVAL;

        return -1;
    }

    int n = snprintf(path, PATH_MAX, STATE_BASE "/%s%s%s", s->id, *leaf ? "/" : "", leaf);

    return n < 0 || n >= PATH_MAX ? -1 : 0;
}

int state_save(const struct state *s) {
    char path[PATH_MAX], temp[PATH_MAX];

    if (state_path(s, "state", path) || state_path(s, "state.new", temp))
        return -1;
    int fd = open(temp, O_WRONLY | O_CREAT | O_TRUNC | O_NOFOLLOW | O_CLOEXEC, 0600);

    if (fd < 0)
        return -1;
    int rc = write(fd, s, sizeof *s) == sizeof *s ? 0 : -1;

    if (!rc)
        rc = fsync(fd);
    close(fd);

    if (!rc)
        rc = rename(temp, path);
    if (rc)
        (void)unlink(temp);
    return rc;
}

int state_load(const char *id, struct state *s) {
    if (!state_id_valid(id)) {
        errno = EINVAL;

        return -1;
    }

    char path[PATH_MAX];
    snprintf(path, sizeof path, STATE_BASE "/%s/state", id);

    int fd = open(path, O_RDONLY | O_CLOEXEC | O_NOFOLLOW);

    if (fd < 0)
        return -1;
    struct stat st;

    if (fstat(fd, &st) || !S_ISREG(st.st_mode) || st.st_size != sizeof *s) {
        close(fd);
        errno = EINVAL;

        return -1;
    }
    memset(s, 0, sizeof *s);

    ssize_t n = read(fd, s, sizeof *s);
    close(fd);

    if (n != sizeof *s || s->magic != MAGIC || s->id[16] || s->name[32] || s->veth[15] ||
        strcmp(id, s->id) || !valid_name(s->name) || s->status < 0 || s->status > 2 || s->ip < 0 ||
        s->ip > 254 || s->publish_count > MAX_PUBLISH || (s->publish_count && !s->ip) ||
        s->pid < 0 || s->supervisor <= 1) {
        errno = EINVAL;

        return -1;
    }

    if ((s->uid_base == 0) != (s->gid_base == 0) ||
        (s->uid_base && (s->uid_base < 65536 || s->gid_base < 65536 ||
                         s->uid_base > UINT32_MAX - 65536 || s->gid_base > UINT32_MAX - 65536))) {
        errno = EINVAL;

        return -1;
    }

    for (unsigned i = 0; i < s->publish_count; i++) {
        if (!s->publish[i].host || !s->publish[i].container) {
            errno = EINVAL;

            return -1;
        }

        for (unsigned j = 0; j < i; j++)
            if (s->publish[i].host == s->publish[j].host) {
                errno = EINVAL;

                return -1;
            }
    }

    char veth[16];
    snprintf(veth, sizeof veth, "kl%.10s", id);

    if (strcmp(veth, s->veth)) {
        errno = EINVAL;

        return -1;
    }

    return 0;
}

int state_each(int (*fn)(struct state *, void *), void *arg) {
    DIR *d = opendir(STATE_BASE);

    if (!d)
        return -1;
    int rc = 0;

    struct dirent *e;

    while ((e = readdir(d))) {
        if (!state_id_valid(e->d_name))
            continue;
        struct state s;

        if (state_load(e->d_name, &s)) {
            rc = -1;
            break;
        }
        rc = fn(&s, arg);

        if (rc)
            break;
    }
    closedir(d);

    return rc;
}

struct find_arg {
    const char *name;

    struct state *out;
};

static int match(struct state *s, void *ptr) {
    struct find_arg *a = ptr;

    if (!strcmp(s->name, a->name) || !strcmp(s->id, a->name)) {
        *a->out = *s;

        return 1;
    }

    return 0;
}

int state_find(const char *name, struct state *s) {
    struct find_arg a = {name, s};

    int rc = state_each(match, &a);

    if (rc == 1)
        return 0;
    if (!rc)
        errno = ENOENT;
    return -1;
}

int state_live(const struct state *s) {
    return s->pid > 1 && s->start && process_start(s->pid) == s->start;
}

int state_supervised(const struct state *s) {
    return s->supervisor > 1 && s->supervisor_start &&
           process_start(s->supervisor) == s->supervisor_start;
}

static int used_ip(struct state *s, void *ptr) {
    unsigned char *used = ptr;

    if (s->status != STATE_EXITED && s->ip)
        used[s->ip] = 1;
    return 0;
}

int state_new(struct state *s, const struct config *c) {
    struct state existing;

    if (!state_find(c->name, &existing)) {
        errno = EEXIST;

        return -1;
    }

    if (errno != ENOENT)
        return -1;
    *s = (struct state){.magic = MAGIC,
                        .supervisor = getpid(),
                        .supervisor_start = process_start(getpid()),
                        .created = time(NULL),
                        .memory = c->memory,
                        .swap = c->swap,
                        .pids = c->pids,
                        .quota = c->quota,
                        .uid_base = c->uid_base,
                        .gid_base = c->gid_base};
    s->publish_count = c->publish_count;
    memcpy(s->publish, c->publish, sizeof s->publish);
    strcpy(s->name, c->name);

    unsigned char random[8];

    if (getrandom(random, sizeof random, 0) != sizeof random)
        return -1;
    for (unsigned i = 0; i < 8; i++)
        snprintf(s->id + 2 * i, 3, "%02x", random[i]);
    snprintf(s->veth, sizeof s->veth, "kl%.10s", s->id);

    if (c->bridge) {
        unsigned char used[255] = {0};

        if (state_each(used_ip, used))
            return -1;
        for (int i = 2; i <= 254; i++)
            if (!used[i]) {
                s->ip = i;
                break;
            }
        if (!s->ip) {
            errno = ENOSPC;

            return -1;
        }
    }

    char path[PATH_MAX];

    if (state_path(s, "", path) || mkdir(path, 0700))
        return -1;
    if (state_save(s)) {
        int saved = errno;
        (void)rmdir(path);
        errno = saved;

        return -1;
    }

    return 0;
}
