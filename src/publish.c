#include "publish.h"

#include "nat.h"
#include "util.h"

#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

static int port_conflict(struct state *other, void *ptr) {
    const struct state *s = ptr;

    if (!strcmp(s->id, other->id) || other->status == STATE_EXITED)
        return 0;
    for (unsigned i = 0; i < s->publish_count; i++)
        for (unsigned j = 0; j < other->publish_count; j++)
            if (s->publish[i].host == other->publish[j].host) {
                errno = EADDRINUSE;

                return -1;
            }
    return 0;
}

int publish_reserve(const struct state *s, int fds[MAX_PUBLISH]) {
    if (state_each(port_conflict, (void *)s)) {
        fprintf(
            stderr,
            "A live or stale container reserves this host port; use cleanup for stale records.\n");
        return -1;
    }

    for (unsigned i = 0; i < s->publish_count; i++) {
        fds[i] = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);

        if (fds[i] < 0)
            return -1;
        struct sockaddr_in addr = {.sin_family = AF_INET,
                                   .sin_port = htons(s->publish[i].host),
                                   .sin_addr = {.s_addr = htonl(INADDR_ANY)}};
        if (bind(fds[i], (struct sockaddr *)&addr, sizeof addr) || listen(fds[i], 1)) {
            fprintf(stderr, "TCP host port %u is already in use or unavailable.\n",
                    s->publish[i].host);
            return -1;
        }
    }

    return 0;
}

static int table_name(const struct state *s, char name[40]) {
    if (!state_id_valid(s->id) || s->ip < 2 || s->ip > 254 || s->publish_count > MAX_PUBLISH) {
        errno = EINVAL;

        return -1;
    }
    snprintf(name, 40, "simplectr_p_%s", s->id);

    return 0;
}

int publish_setup(const struct state *s) {
    if (!s->publish_count)
        return 0;
    char name[40], rules[16384];

    if (table_name(s, name))
        return -1;
    int n = snprintf(
        rules, sizeof rules,
        "add table ip %s { comment \"simplectr-publish-%s\"; }\n"
        "add chain ip %s prerouting { type nat hook prerouting priority dstnat; policy accept; }\n"
        "add chain ip %s output { type nat hook output priority dstnat; policy accept; }\n"
        "add chain ip %s postrouting { type nat hook postrouting priority 99; policy accept; }\n",
        name, s->id, name, name, name);
    if (n < 0 || (size_t)n >= sizeof rules)
        return -1;
    size_t used = (size_t)n;

    for (unsigned i = 0; i < s->publish_count; i++) {
        unsigned hp = s->publish[i].host, cp = s->publish[i].container;

        if (!hp || !cp) {
            errno = EINVAL;

            return -1;
        }
        n = snprintf(
            rules + used, sizeof rules - used,
            "add rule ip %s prerouting fib daddr type local tcp dport %u dnat to 10.88.0.%d:%u\n"
            "add rule ip %s output fib daddr type local tcp dport %u dnat to 10.88.0.%d:%u\n"
            "add rule ip %s postrouting ip daddr 10.88.0.%d tcp dport %u ct status dnat ct "
            "original proto-dst %u masquerade\n",
            name, hp, s->ip, cp, name, hp, s->ip, cp, name, s->ip, cp, hp);
        if (n < 0 || (size_t)n >= sizeof rules - used) {
            errno = EOVERFLOW;

            return -1;
        }
        used += (size_t)n;
    }

    if (write_file("/proc/sys/net/ipv4/conf/" BRIDGE "/route_localnet", "1"))
        return -1;
    return nat_transaction(rules, NULL, 0);
}

int publish_remove(const struct state *s) {
    if (!s->publish_count)
        return 0;
    char name[40], query[128], tables[32768], body[16384], marker[80];

    if (table_name(s, name))
        return -1;
    if (nat_transaction("list tables\n", tables, sizeof tables))
        return -1;
    snprintf(query, sizeof query, "table ip %s\n", name);

    if (!strstr(tables, query))
        return 0;
    snprintf(query, sizeof query, "list table ip %s\n", name);

    if (nat_transaction(query, body, sizeof body))
        return -1;
    snprintf(marker, sizeof marker, "comment \"simplectr-publish-%s\"", s->id);

    if (!strstr(body, marker)) {
        errno = EEXIST;

        return -1;
    }
    snprintf(query, sizeof query, "delete table ip %s\n", name);

    return nat_transaction(query, NULL, 0);
}
