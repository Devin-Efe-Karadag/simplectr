#include "network.h"

#include "nat.h"
#include "netlink.h"
#include "util.h"

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <libmnl/libmnl.h>
#include <linux/if_link.h>
#include <linux/rtnetlink.h>
#include <linux/veth.h>
#include <net/if.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static int owned(const char *name, const char *alias) {
    char path[128], buf[128];
    snprintf(path, sizeof path, "/sys/class/net/%s/ifalias", name);

    if (read_file(path, buf, sizeof buf))
        return -1;
    buf[strcspn(buf, "\n")] = 0;

    if (strcmp(buf, alias)) {
        errno = EEXIST;

        return -1;
    }

    return 0;
}

static int set_alias(int index, const char *alias) {
    struct nl_request r;

    struct nlmsghdr *n = nl_link(&r, RTM_NEWLINK, 0, index);
    mnl_attr_put_strz(n, IFLA_IFALIAS, alias);

    return nl_exchange(n);
    }

    if (nl_address(index, "10.88.0.1") && errno != EEXIST)
        return -1;
    if (nl_up(index) || nat_setup())
        return -1;
    return index;
}

static int veth_create(const struct state *s, const char *peer) {
    struct nl_request r;

    struct nlmsghdr *n = nl_link(&r, RTM_NEWLINK, NLM_F_CREATE | NLM_F_EXCL, 0);
    mnl_attr_put_strz(n, IFLA_IFNAME, s->veth);

    char alias[64];
    snprintf(alias, sizeof alias, "simplectr:%s", s->id);
    mnl_attr_put_strz(n, IFLA_IFALIAS, alias);

    struct nlattr *info = mnl_attr_nest_start(n, IFLA_LINKINFO);
    mnl_attr_put_strz(n, IFLA_INFO_KIND, "veth");

    struct nlattr *data = mnl_attr_nest_start(n, IFLA_INFO_DATA),
                  *p = mnl_attr_nest_start(n, VETH_INFO_PEER);
    struct ifinfomsg *i = mnl_nlmsg_put_extra_header(n, sizeof *i);
    i->ifi_family = AF_UNSPEC;
    mnl_attr_put_strz(n, IFLA_IFNAME, peer);
    if (fd < 0)
        return -1;
    n = nl_link(&r, RTM_NEWLINK, 0, other);
    mnl_attr_put_u32(n, IFLA_NET_NS_FD, (unsigned)fd);

    int rc = nl_exchange(n);
    close(fd);

    return rc;
}
    mnl_attr_put_strz(n, IFLA_IFNAME, "eth0");
