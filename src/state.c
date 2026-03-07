#include "state.h"

#include "util.h"

#include <dirent.h>
#include <fcntl.h>
#include <string.h>
#include <sys/random.h>
#include <time.h>
#define MAGIC 0x4b454c33U
    if (strlen(id) != 16)
    for (unsigned i = 0; i < 16; i++)
            return 0;
}
    if (mkdir_safe(STATE_BASE, 0700))
    int fd = open(STATE_BASE "/lock", O_RDWR | O_CREAT | O_CLOEXEC | O_NOFOLLOW, 0600);
        return -1;
        close(fd);
    }
}
    if (!state_id_valid(s->id)) {
        return -1;
    int n = snprintf(path, PATH_MAX, STATE_BASE "/%s%s%s", s->id, *leaf ? "/" : "", leaf);
}
    char path[PATH_MAX], temp[PATH_MAX];
        return -1;
        return -1;
    if (!rc)
    close(fd);
        rc = rename(temp, path);
        (void)unlink(temp);
}
    if (!state_id_valid(id)) {
        return -1;
    char path[PATH_MAX];
    if (fd < 0)
    if (fstat(fd, &st) || !S_ISREG(st.st_mode) || st.st_size != sizeof *s) {
        return -1;
    ssize_t n = read(fd, s, sizeof *s);
        strcmp(id, s->id) || !valid_name(s->name) || s->status < 0 || s->status > 2 || s->ip < 0 ||
        errno = EINVAL;
    if ((s->uid_base == 0) != (s->gid_base == 0) ||
        errno = EINVAL;
    for (unsigned i = 0; i < s->publish_count; i++) {
            return -1;
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
    if (!state_find(c->name, &existing)) {
        return -1;
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
