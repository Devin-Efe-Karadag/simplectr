#include "config.h"

#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int valid_name(const char *s) {
    size_t n = strlen(s);

    if (!n || n > 32 || !isalnum((unsigned char)s[0]))
        return 0;

    for (size_t i = 0; i < n; i++)
        if (!isalnum((unsigned char)s[i]) && s[i] != '-' && s[i] != '_')
            return 0;

    return 1;
}

int parse_size(const char *s, uint64_t *n) {
    if (!isdigit((unsigned char)*s))
        return -1;

    errno = 0;
    char *end;
    unsigned long long value = strtoull(s, &end, 10), factor = 1;

    if (errno)
        return -1;
    if (*end) {
        if (end[1])
            return -1;
        switch (*end) {
        case 'K':
            factor = 1024;
            break;
        case 'M':
            factor = 1024 * 1024;
            break;
        case 'G':
            factor = 1024ULL * 1024 * 1024;
            break;
        default:
            return -1;
        }
    }

    if (value > UINT64_MAX / factor)
        return -1;

    *n = value * factor;
    return 0;
}

int parse_port(const char *text, struct port_mapping *mapping) {
    const char *p = text;
    unsigned parts[2] = {0};

    for (unsigned part = 0; part < 2; part++) {
        if (*p < '0' || *p > '9')
            return -1;
        while (*p >= '0' && *p <= '9') {
            parts[part] = parts[part] * 10 + (unsigned)(*p++ - '0');
            if (parts[part] > 65535)
                return -1;
        }
        if (!parts[part])
            return -1;
        if (!part) {
            if (*p++ != ':')
                return -1;
        } else if (*p)
            return -1;
    }

    *mapping = (struct port_mapping){(uint16_t)parts[0], (uint16_t)parts[1]};
    return 0;
}

int parse_config(int argc, char **argv, struct config *c) {
    *c = (struct config){.memory = 256 * 1024 * 1024, .swap = 0, .pids = 64, .quota = 100000};

    unsigned seen = 0;

    for (int i = 0; i < argc; i++) {
        if (!strcmp(argv[i], "--")) {
            if (i + 1 == argc || argv[i + 1][0] != '/' || !c->name[0])
                return -1;
            if (c->publish_count && !c->bridge)
                return -1;
            c->argv = &argv[i + 1];
            return 0;
        }
        if (!strcmp(argv[i], "--tty") || !strcmp(argv[i], "-t")) {
            if (seen & 256)
                return -1;
            seen |= 256;
            c->tty = true;
            continue;
        }
        if (i + 1 >= argc)
            return -1;
        const char *key = argv[i++], *v = argv[i];
        unsigned bit;
        if (!strcmp(key, "--publish")) {
            if (c->publish_count == MAX_PUBLISH || parse_port(v, &c->publish[c->publish_count]))
                return -1;
            for (unsigned j = 0; j < c->publish_count; j++)
                if (c->publish[j].host == c->publish[c->publish_count].host)
                    return -1;
            c->publish_count++;
            continue;
        }
        if (!strcmp(key, "--name")) {
            bit = 1;
            if (!valid_name(v))
                return -1;
            strcpy(c->name, v);
        } else if (!strcmp(key, "--userns")) {
            bit = 128;
            if (!valid_name(v))
                return -1;
            strcpy(c->map_user, v);
        } else if (!strcmp(key, "--net")) {
            bit = 2;
            if (strcmp(v, "none") && strcmp(v, "bridge"))
                return -1;
            c->bridge = !strcmp(v, "bridge");
        } else if (!strcmp(key, "--rootfs")) {
            bit = 4;
            if (strcmp(v, "alpine"))
                return -1;
        } else if (!strcmp(key, "--memory")) {
            bit = 8;
            if (parse_size(v, &c->memory) || c->memory < 4 * 1024 * 1024)
                return -1;
        } else if (!strcmp(key, "--memory-swap")) {
            bit = 16;
            if (parse_size(v, &c->swap))
                return -1;
        } else if (!strcmp(key, "--pids")) {
            bit = 32;
            if (parse_size(v, &c->pids) || !c->pids || c->pids > 1048576)
                return -1;
            for (const char *p = v; *p; p++)
                if (!isdigit((unsigned char)*p))
                    return -1;
        } else if (!strcmp(key, "--cpus")) {
            bit = 64;
            char *end;
            errno = 0;
            double cpu = strtod(v, &end);
            if (errno || *end || !isfinite(cpu) || cpu < 0.01 || cpu > 1024)
                return -1;
            c->quota = (uint64_t)(cpu * 100000);
        } else
            return -1;
        if (seen & bit)
            return -1;
        seen |= bit;
    }
    return -1;
}
