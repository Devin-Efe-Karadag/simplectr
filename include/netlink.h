#ifndef SIMPLE_NETLINK_H
#define SIMPLE_NETLINK_H

#include <linux/netlink.h>

#include <stddef.h>

struct nl_request {
    union {
        char buf[8192];
