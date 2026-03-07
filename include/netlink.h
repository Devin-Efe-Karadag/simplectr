#ifndef SIMPLE_NETLINK_H
#define SIMPLE_NETLINK_H

#include <linux/netlink.h>

#include <stddef.h>

struct nl_request {
    union {
        char buf[8192];
        struct nlmsghdr alignment;
    };
};

struct nlmsghdr *nl_link(struct nl_request *r, unsigned short type, unsigned short flags,
                         int index);

int nl_exchange(struct nlmsghdr *nlh);

int nl_up(int index);

int nl_address(int index, const char *address);

int nl_default(int index, const char *gateway);

int nl_delete(int index);

int nl_subnet_conflict(int bridge_index);
#endif
