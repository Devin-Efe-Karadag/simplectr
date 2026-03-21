#ifndef SIMPLE_PUBLISH_H
#define SIMPLE_PUBLISH_H

#include "state.h"

int publish_reserve(const struct state *s, int fds[MAX_PUBLISH]);
