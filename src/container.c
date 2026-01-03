#include "container.h"

#include "cgroup.h"
#include "network.h"
#include "publish.h"
#include "terminal.h"
#include <errno.h>
#include <signal.h>
#include <sys/prctl.h>
#include <sys/wait.h>
struct child_args {
    int gate[2], output[2];
    struct child_args *a = ptr;
    if (dup2(a->output[1], 1) < 0 || dup2(a->output[1], 2) < 0)
        return 125;
    if (read(a->gate[1], &go, 1) != 1 || go != 'G')
    if (setsid() < 0)
        return 125;
    char base[PATH_MAX];

    if (state_path(a->s, "", base) || namespace_prepare(a->c->name) || network_child(a->s) ||
        overlay_mount(base) || mounts_enter(base)) {
    if (userns_child(a->c, a->gate[1]) || security_apply()) {
    }
        return 125;
    if (master >= 0)
        close(master);
    close(a->gate[1]);
    if (close_range(3, ~0U, 0)) {
        perror("close_range");
        return 125;
    }

    char *env[] = {"PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin",
                   "HOME=/root",
                   "USER=root",
                   "LANG=C",
                   "TERM=xterm",
                   NULL};
    int rc = 0;
    if (publish_remove(s))
        rc = -1;
    if (network_remove(s))
        rc = -1;
    if (cgroup_remove(s->id))
        rc = -1;
    const char *dirs[] = {"merged", "work", "upper", "lower"};
    for (unsigned i = 0; i < 4; i++) {
        char path[PATH_MAX];
        if (state_path(s, dirs[i], path) || remove_tree(path))
            rc = -1;
    }
    return rc;
}

int container_run(const struct config *c) {
    if (environment_check())
        return 125;
    struct config effective = *c;
    if (userns_resolve(&effective))
        return 125;
    c = &effective;
    if (access(IMAGE_BASE "/alpine/bin/busybox", X_OK)) {
        fprintf(stderr, "Run sudo ./bin/simplectr pull alpine first.\n");
        return 125;
    }
    int lock = state_lock();
    if (lock < 0) {
        perror("state lock");
        return 125;
    }
    struct state s;
    if (state_new(&s, c)) {
        perror("reserve container (names persist until cleanup)");
        close(lock);
        return 125;
    }
    int rc = 125, logfd = -1, sigfd = -1, master = -1;
    int reservations[MAX_PUBLISH];
    for (unsigned i = 0; i < MAX_PUBLISH; i++)
        reservations[i] = -1;
    void *stack = NULL;
    pid_t pid = -1;
    struct child_args a = {.c = c, .s = &s, .gate = {-1, -1}, .output = {-1, -1}};
    sigset_t mask, oldmask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGCHLD);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGTERM);
    sigaddset(&mask, SIGHUP);
    sigaddset(&mask, SIGWINCH);
    bool masked = false, workload_finished = false;
    char path[PATH_MAX];
    if (publish_reserve(&s, reservations))
        goto finish;

    if (state_path(&s, "", path) || overlay_dirs(path) || cgroup_create(s.id, c))
        goto finish;

    if (state_path(&s, "logs", path))
        goto finish;
    logfd = open(path, O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC | O_NOFOLLOW, 0600);
    if (logfd < 0)
        goto finish;
    if (socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0, a.gate) || pipe2(a.output, O_CLOEXEC))
        goto finish;
    if (sigprocmask(SIG_BLOCK, &mask, &oldmask))
        goto finish;
    masked = true;
    sigfd = signalfd(-1, &mask, SFD_CLOEXEC | SFD_NONBLOCK);
    if (sigfd < 0)
        goto finish;
    struct sigaction ignore = {.sa_handler = SIG_IGN};
    sigemptyset(&ignore.sa_mask);
    (void)sigaction(SIGPIPE, &ignore, NULL);
    stack = malloc(CHILD_STACK);
    if (!stack)
        goto finish;
    pid = namespace_clone(child_entry, &a, stack);
    if (pid < 0)
        goto finish;
    s.pid = pid;
    s.start = process_start(pid);
    close(a.gate[1]);
    a.gate[1] = -1;
    close(a.output[1]);
    a.output[1] = -1;
    if (!s.start || state_save(&s) || cgroup_attach(s.id, pid) || network_parent(&s) ||
    struct timeval timeout = {.tv_sec = 30};
    if (setsockopt(a.gate[0], SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof timeout))
        goto finish;
    if (terminal_receive(a.gate[0], &ready, &master))
        goto finish;
    if (ready == 'U') {
        if (!c->uid_base || userns_map(c, pid) || write(a.gate[0], "M", 1) != 1 ||
            terminal_receive(a.gate[0], &ready, &master))
            goto finish;
    }
    if (master >= 0)
        terminal_resize(master);
    if (ready != 'R' || (c->tty && master < 0))
        goto finish;
    s.status = STATE_RUNNING;
    if (state_save(&s) || write(a.gate[0], "E", 1) != 1)
        goto finish;
    close(a.gate[0]);
