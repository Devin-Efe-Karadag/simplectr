#ifndef SIMPLE_CONFIG_H
#define SIMPLE_CONFIG_H

#include <stdbool.h>

#include <stdint.h>
#define RUNTIME "simplectr"
#define STATE_BASE "/run/simplectr"
#define IMAGE_BASE "/var/lib/simplectr/images"
#define CGROUP_BASE "/sys/fs/cgroup/simplectr"
#define BRIDGE "simplectr0"
#define MAX_PUBLISH 16

struct port_mapping {
    uint16_t host, container;
};

struct config {
    char name[33];
    bool bridge, tty;
    char map_user[33];
    uint32_t uid_base, gid_base;
    uint64_t memory, swap, pids, quota;
    unsigned publish_count;
    struct port_mapping publish[MAX_PUBLISH];
    char **argv;
};

int valid_name(const char *s);

int parse_size(const char *s, uint64_t *n);

int parse_port(const char *text, struct port_mapping *mapping);

int parse_config(int argc, char **argv, struct config *c);
#endif
