#include "mounts.h"
#include "namespace.h"
#include "overlay.h"
#include "process.h"
#include "security.h"
#include "util.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static int child(void *arg) {
    if (namespace_prepare("mount-test") || overlay_mount(arg) || mounts_enter(arg)) {
        perror("mounts");

        return 1;
    }

    if (!access("/home/testuser/.profile", F_OK) || !access("/.oldroot", F_OK))
        return 2;
    if (security_apply()) {
        perror("hardening");

        return 4;
    }

    char *args[] = {
