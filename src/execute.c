#include <sys/signalfd.h>
#include <sys/wait.h>
struct exec_args {
    char **argv;
    struct state state;
static int exec_child(void *ptr) {
    close(a->reportfd); /* Do not keep the helper report pipe alive if the helper crashes. */
        return 125;
        return 125;
    if (read(a->gate[1], &byte, 1) != 1 || byte != 'G')
    process_child_signals();
        return 125;
    if (a->tty) {
        if (master < 0) {
            return 125;
        }
    if (setgroups(0, NULL))
    if (a->state.uid_base &&
        perror("exec user namespace");
    }
        return 125;
        return 125;
        return 125;
                   "HOME=/root",
                   "LANG=C",
                   NULL};
