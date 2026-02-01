#ifndef SIMPLE_CGROUP_H
#define SIMPLE_CGROUP_H

#include "config.h"

#include <sys/types.h>

int cgroup_create(const char *id, const struct config *c);
