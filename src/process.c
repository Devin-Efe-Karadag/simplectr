static void forward(int sig) {
    if (child_pid > 0)
        (void)kill((pid_t)child_pid, sig);
}

int process_signals(void) {
    struct sigaction sa = {.sa_handler = forward};
    sigemptyset(&sa.sa_mask);

    if (sigaction(SIGINT, &sa, NULL) || sigaction(SIGTERM, &sa, NULL) ||
        sigaction(SIGHUP, &sa, NULL))
    sigemptyset(&sa.sa_mask);
    sigset_t mask;
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
