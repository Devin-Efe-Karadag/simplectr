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
