#include "namespace.h"
#include "process.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mount.h>
#include <unistd.h>

static int child(void *arg) {
    (void)arg;

    char h[64];

    if (getpid() != 1 || namespace_prepare("simplectr-test"))
