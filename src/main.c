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
