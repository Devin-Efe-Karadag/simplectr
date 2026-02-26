#ifndef SIMPLE_USERNS_H
#define SIMPLE_USERNS_H

#include "config.h"

#include <sys/types.h>

int userns_resolve(struct config *c);

int userns_child(const struct config *c, int gate);

int userns_map(const struct config *c, pid_t pid);
#endif
