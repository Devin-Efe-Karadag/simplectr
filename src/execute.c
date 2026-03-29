#include "execute.h"

#include "cgroup.h"
#include "namespace.h"
#include "process.h"
#include "security.h"
#include "state.h"
#include "terminal.h"
#include "util.h"

#include <errno.h>
#include <fcntl.h>
#include <grp.h>
#include <sched.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/prctl.h>
#include <sys/signalfd.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

struct exec_args {
    int gate[2], output[2], userfd, reportfd;
    char **argv;
    bool tty;
    struct state state;
};

static int exec_child(void *ptr) {
    struct exec_args *a = ptr;

    close(a->reportfd); /* Do not keep the helper report pipe alive if the helper crashes. */

    if (prctl(PR_SET_PDEATHSIG, SIGKILL))
        return 125;
    if (dup2(a->output[1], 1) < 0 || dup2(a->output[1], 2) < 0)
        return 125;

    char byte;
    if (read(a->gate[1], &byte, 1) != 1 || byte != 'G')
        return 125;
    process_child_signals();
    if (setsid() < 0)
        return 125;

    int master = -1;
    if (a->tty) {
        master = terminal_child(a->state.uid_base, a->state.gid_base);
        if (master < 0) {
            perror("exec PTY");
            return 125;
        }
    }

    if (setgroups(0, NULL))
        return 125;
    if (a->state.uid_base &&
        (setns(a->userfd, CLONE_NEWUSER) || setresgid(0, 0, 0) || setresuid(0, 0, 0))) {
        perror("exec user namespace");
        return 125;
    }

    if (prctl(PR_SET_PDEATHSIG, SIGKILL) || security_apply())
        return 125;
    if (terminal_send(a->gate[1], 'R', master) || read(a->gate[1], &byte, 1) != 1 || byte != 'E')
        return 125;
    if (close_range(3, ~0U, 0))
        return 125;

    char *env[] = {"PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin",
                   "HOME=/root",
                   "USER=root",
                   "LANG=C",
                   "TERM=xterm",
                   NULL};

    execve(a->argv[0], a->argv, env);
    int saved = errno;
    perror("execve");
    return saved == ENOENT ? 127 : 126;
}

int container_exec(const char *name, char **argv, bool tty) {
    if (environment_check() || !valid_name(name) || !argv[0] || argv[0][0] != '/')
        return 125;
    int lock = state_lock();
    if (lock < 0)
        return 125;
    struct state state;
    int rc = 125, root = -1, master = -1, sigfd = -1, report[2] = {-1, -1};
    pid_t worker = -1, pid = -1;
    const char *names[] = {"mnt", "uts", "ipc", "net", "cgroup", "pid", "user"};
    const int flags[] = {CLONE_NEWNS,     CLONE_NEWUTS, CLONE_NEWIPC, CLONE_NEWNET,
                         CLONE_NEWCGROUP, CLONE_NEWPID, CLONE_NEWUSER};
    int ns[7] = {-1, -1, -1, -1, -1, -1, -1};
    struct exec_args a = {
        .gate = {-1, -1}, .output = {-1, -1}, .userfd = -1, .argv = argv, .tty = tty};
    sigset_t mask, oldmask;
    bool masked = false, workload_finished = false;
    if (state_find(name, &state) || state.status != STATE_RUNNING || !state_live(&state)) {
        errno = ESRCH;
        goto done;
    }
    a.state = state;
    char path[64];
    for (unsigned i = 0; i < 7; i++) {
        snprintf(path, sizeof path, "/proc/%ld/ns/%s", (long)state.pid, names[i]);
        ns[i] = open(path, O_RDONLY | O_CLOEXEC);
        if (ns[i] < 0)
            goto done;
    }
    snprintf(path, sizeof path, "/proc/%ld/root", (long)state.pid);
    root = open(path, O_RDONLY | O_DIRECTORY | O_CLOEXEC);
    if (root < 0)
        goto done;
    if (!state_live(&state)) {
        errno = ESRCH;
        goto done;
    }
    if (socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0, a.gate) || pipe2(a.output, O_CLOEXEC) ||
        pipe2(report, O_CLOEXEC))
        goto done;
    sigemptyset(&mask);
    sigaddset(&mask, SIGCHLD);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGTERM);
    sigaddset(&mask, SIGHUP);
    sigaddset(&mask, SIGWINCH);
    if (sigprocmask(SIG_BLOCK, &mask, &oldmask))
        goto done;
    masked = true;
    sigfd = signalfd(-1, &mask, SFD_CLOEXEC | SFD_NONBLOCK);
    if (sigfd < 0)
        goto done;
    struct sigaction ignore = {.sa_handler = SIG_IGN};
    sigemptyset(&ignore.sa_mask);
    if (sigaction(SIGPIPE, &ignore, NULL))
        goto done;
    worker = fork();
    if (worker < 0)
        goto done;
    if (!worker) {
        close(report[0]);
        close(a.gate[0]);
        close(a.output[0]);
        close(lock);
        for (unsigned i = 0; i < 6; i++)
            if (setns(ns[i], flags[i])) {
                perror("exec setns");
                _exit(125);
            }
        /* Joining an existing mount namespace does not change root/cwd. Use the pinned target root.
         */
        if (fchdir(root) || chroot(".") || chdir("/")) {
            perror("exec root");
            _exit(125);
        }
        a.userfd = ns[6];
        a.reportfd = report[1];
        void *stack = malloc(CHILD_STACK);
        if (!stack)
            _exit(125);
        pid_t child = clone(exec_child, (char *)stack + CHILD_STACK, CLONE_PARENT | SIGCHLD, &a);
        if (write(report[1], &child, sizeof child) != sizeof child)
            _exit(125);
        _exit(child > 0 ? 0 : 125);
    }
    close(report[1]);
    report[1] = -1;
    close(a.gate[1]);
    a.gate[1] = -1;
    close(a.output[1]);
    a.output[1] = -1;
    ssize_t n;
    do {
        n = read(report[0], &pid, sizeof pid);
    } while (n < 0 && errno == EINTR);
    int status;
    while (waitpid(worker, &status, 0) < 0)
        if (errno != EINTR)
        close(sigfd);
    if (lock >= 0)
        close(lock);
    if (masked)
        (void)sigprocmask(SIG_SETMASK, &oldmask, NULL);
    return rc;
}
