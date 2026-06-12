#include "process.h"

#include <errno.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
static volatile sig_atomic_t child_pid;

static void forward(int sig) {
    if (child_pid > 0)
        (void)kill((pid_t)child_pid, sig);
}

int process_signals(void) {
    struct sigaction sa = {.sa_handler = forward};
    sigemptyset(&sa.sa_mask);

    if (sigaction(SIGINT, &sa, NULL) || sigaction(SIGTERM, &sa, NULL) ||
        sigaction(SIGHUP, &sa, NULL))

        return -1;
    return 0;
}

void process_child_signals(void) {
    struct sigaction sa = {.sa_handler = SIG_DFL};
    sigemptyset(&sa.sa_mask);
    (void)sigaction(SIGINT, &sa, NULL);
    (void)sigaction(SIGTERM, &sa, NULL);
    (void)sigaction(SIGHUP, &sa, NULL);
    (void)sigaction(SIGPIPE, &sa, NULL);
    sigset_t mask;
    sigemptyset(&mask);
    (void)sigprocmask(SIG_SETMASK, &mask, NULL);
}

int process_wait(pid_t pid) {
    child_pid = pid;

    int status;

    while (waitpid(pid, &status, 0) < 0) {
        if (errno != EINTR) {
            child_pid = 0;

            return -1;
        }
    }
    child_pid = 0;

    return WIFEXITED(status) ? WEXITSTATUS(status) : 128 + WTERMSIG(status);
}
