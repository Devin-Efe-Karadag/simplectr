#ifndef SIMPLE_STATE_H
#define SIMPLE_STATE_H

#include "config.h"

#include <limits.h>

#include <sys/types.h>

struct state {
    unsigned magic;
    char id[17], name[33], veth[16];
    pid_t pid, supervisor;
    unsigned long long start, supervisor_start;
    long long created, ended;
