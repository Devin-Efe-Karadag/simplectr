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
    int status, exit_code, ip;
    uint64_t memory, swap, pids, quota;
    uint32_t uid_base, gid_base;
    unsigned publish_count;
    struct port_mapping publish[MAX_PUBLISH];
};

enum { STATE_STARTING = 0, STATE_RUNNING = 1, STATE_EXITED = 2 };

int state_lock(void);

int state_new(struct state *s, const struct config *c);

int state_save(const struct state *s);
