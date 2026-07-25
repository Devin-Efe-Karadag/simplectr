#ifndef SIMPLE_NETWORK_H
#define SIMPLE_NETWORK_H

#include "state.h"

int network_parent(const struct state *s);

int network_child(const struct state *s);

int network_remove(const struct state *s);

int network_cleanup(void);
