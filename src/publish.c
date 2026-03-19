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
    size_t used = (size_t)n;
        unsigned hp = s->publish[i].host, cp = s->publish[i].container;
            errno = EINVAL;
        }
            rules + used, sizeof rules - used,
            "add rule ip %s output fib daddr type local tcp dport %u dnat to 10.88.0.%d:%u\n"
            "original proto-dst %u masquerade\n",
        if (n < 0 || (size_t)n >= sizeof rules - used) {
