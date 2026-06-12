#include "cli.h"
#include "config.h"
#include "container.h"
#include "execute.h"
#include "rootfs.h"
#include "state.h"
#include "util.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

static int usage(void) {
    fputs("Usage:\n  simplectr doctor\n  simplectr pull alpine\n"
          "  simplectr run --name NAME [--rootfs alpine] [--net none|bridge]\n"
          "    [--publish HOST_PORT:CONTAINER_PORT] [--tty] [--userns HOST_ACCOUNT] [--memory "
          "256M] [--memory-swap 0] [--cpus 1] "
          "[--pids 64] -- "
          "/command [args...]\n"
          "  simplectr exec [--tty] NAME -- /command [args...]\n  simplectr list | inspect NAME | "
          "logs "
          "NAME | stop NAME | cleanup\n",
          stderr);
    return 2;
}

int main(int argc, char **argv) {
    if (argc < 2)
        return usage();
    if (!strcmp(argv[1], "--help")) {
        (void)usage();

        return 0;
    }

    if (!strcmp(argv[1], "run")) {
        struct config c;

        if (parse_config(argc - 2, argv + 2, &c))
            return usage();
        return container_run(&c);
    }

    if (!strcmp(argv[1], "exec")) {
        int i = 2;

        bool tty = false;

        if (i < argc && (!strcmp(argv[i], "--tty") || !strcmp(argv[i], "-t"))) {
            tty = true;
            i++;
        }

        if (i + 2 >= argc || !valid_name(argv[i]) || strcmp(argv[i + 1], "--") ||
            argv[i + 2][0] != '/')

            return usage();
        return container_exec(argv[i], &argv[i + 2], tty);
    }

    if (environment_check())
        return 1;
    if (argc == 2 && !strcmp(argv[1], "doctor")) {
        puts("ARM64 Linux, root, cgroup v2, OverlayFS: OK");

        return 0;
    }

    if (argc == 3 && !strcmp(argv[1], "pull") && !strcmp(argv[2], "alpine")) {
        if (rootfs_pull()) {
            perror("pull");

            return 1;
        }

        return 0;
    }

    if (!((argc == 2 && (!strcmp(argv[1], "list") || !strcmp(argv[1], "cleanup"))) ||
          (argc == 3 && valid_name(argv[2]) &&
           (!strcmp(argv[1], "inspect") || !strcmp(argv[1], "logs") || !strcmp(argv[1], "stop")))))
        return usage();
    int lock = state_lock();

    if (lock < 0) {
        perror("lock");

        return 1;
    }

    int rc;

    if (!strcmp(argv[1], "list"))
        rc = container_list();
    else if (!strcmp(argv[1], "cleanup"))
        rc = container_cleanup();
    else if (!strcmp(argv[1], "inspect"))
        rc = container_inspect(argv[2]);
    else if (!strcmp(argv[1], "logs"))
        rc = container_logs(argv[2]);
    else
        rc = container_stop(argv[2]);
    if (rc)
        perror(argv[1]);
    close(lock);

    return rc ? 1 : 0;
}
