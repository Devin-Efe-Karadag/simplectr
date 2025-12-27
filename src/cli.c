#include "cli.h"

#include "cgroup.h"
#include "container.h"
#include "network.h"
#include "state.h"
#include "util.h"

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/syscall.h>
#include <unistd.h>

static int listing(struct state *s, void *arg) {
    (void)arg;

    printf("%s %-20s %-8s pid=%ld ip=", s->id, s->name,
           state_live(s)                                        ? "running"
           : state_supervised(s) && s->status == STATE_STARTING ? "starting"
           : s->status == STATE_EXITED                          ? "exited"
                                                                : "stale",
           (long)s->pid);

    if (s->ip)
        printf("10.88.0.%d", s->ip);
    else
        printf("none");

    puts("");
    return 0;
}

int container_list(void) { return state_each(listing, NULL); }

int container_logs(const char *name) {
    struct state s;
    char path[PATH_MAX];
    if (state_find(name, &s) || state_path(&s, "logs", path))
        return -1;

    int fd = open(path, O_RDONLY | O_NOFOLLOW | O_CLOEXEC);
    if (fd < 0)
        return -1;

    char buf[8192];
    ssize_t n;

    while ((n = read(fd, buf, sizeof buf)) > 0) {
        ssize_t off = 0;
        while (off < n) {
            ssize_t w = write(1, buf + off, (size_t)(n - off));
            if (w <= 0) {
                close(fd);
                return -1;
            }
            off += w;
        }
    }

    close(fd);
    return n < 0 ? -1 : 0;
}

int container_inspect(const char *name) {
    struct state s;
    if (state_find(name, &s))
        return -1;
    listing(&s, NULL);
    for (unsigned i = 0; i < s.publish_count; i++)
        printf("publish=%u:%u/tcp\n", s.publish[i].host, s.publish[i].container);
    printf("uid_base=%u\ngid_base=%u\n", s.uid_base, s.gid_base);
    printf("name=%s\nid=%s\npid=%ld\npid_start_ticks=%llu\nsupervisor=%ld\ncreated=%lld\nended=%"
           "lld\nexit_code=%d\n"
           "cgroup=" CGROUP_BASE "/%s\nrootfs=" IMAGE_BASE "/alpine\noverlay=" STATE_BASE
           "/%s\nlogs=" STATE_BASE "/%s/logs\n"
           "memory.max=%llu\nmemory.swap.max=%llu\ncpu.max=%llu 100000\npids.max=%llu\nveth=%s\n",
           s.name, s.id, (long)s.pid, s.start, (long)s.supervisor, s.created, s.ended, s.exit_code,
           s.id, s.id, s.id, (unsigned long long)s.memory, (unsigned long long)s.swap,
           (unsigned long long)s.quota, (unsigned long long)s.pids, s.ip ? s.veth : "none");
    if (state_live(&s))
        container_metrics(&s);
    char path[PATH_MAX], buf[8192];
    if (!state_path(&s, "metrics", path) && !read_file(path, buf, sizeof buf))
        fputs(buf, stdout);
    return 0;
}

int container_stop(const char *name) {
    struct state s;
    if (state_find(name, &s))
        return -1;
    if (!state_live(&s)) {
        errno = ESRCH;
        return -1;
    }
    int fd = (int)syscall(SYS_pidfd_open, s.pid, 0);
    if (fd < 0)
        return -1;
    if (!state_live(&s)) {
        close(fd);
        errno = ESRCH;
        return -1;
    }
    if (syscall(SYS_pidfd_send_signal, fd, SIGTERM, NULL, 0)) {
        close(fd);
        return -1;
    }
    struct pollfd p = {fd, POLLIN, 0};
    int rc;
    do {
        rc = poll(&p, 1, 3000);
    } while (rc < 0 && errno == EINTR);
    if (!rc)
        rc = (int)syscall(SYS_pidfd_send_signal, fd, SIGKILL, NULL, 0);

    close(fd);
    return rc < 0 ? -1 : 0;
}

static int cleanup_one(struct state *s, void *arg) {
    int *active = arg;
    if (state_live(s) || (state_supervised(s) && s->status != STATE_EXITED)) {
        *active = 1;
        return 0;
    }
    /* No live namespace init: all descendants have been killed by the kernel. */
    if (container_release(s))
        return -1;
    char path[PATH_MAX];
    return state_path(s, "", path) || remove_tree(path) ? -1 : 0;
}

int container_cleanup(void) {
    int active = 0;
    if (state_each(cleanup_one, &active))
        return -1;
    if (!active) {
        if (network_cleanup())
            return -1;
        if (cgroup_cleanup())
            return -1;
    }
    return 0;
}
