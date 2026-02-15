#include "config.h"
#include "state.h"
#include "util.h"

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static void reject_record(const struct state *s, unsigned magic, off_t size) {
    char path[PATH_MAX];
