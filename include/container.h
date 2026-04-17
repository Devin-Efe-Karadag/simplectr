#ifndef SIMPLE_CONTAINER_H
#define SIMPLE_CONTAINER_H

#include "config.h"

#include "state.h"

int container_run(const struct config *c);

void container_metrics(const struct state *s);

int container_release(struct state *s);
