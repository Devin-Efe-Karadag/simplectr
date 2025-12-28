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
