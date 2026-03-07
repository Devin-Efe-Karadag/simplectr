#include "netlink.h"

#include <arpa/inet.h>
#include <errno.h>
#include <libmnl/libmnl.h>
#include <linux/if_link.h>
#include <linux/rtnetlink.h>
#include <net/if.h>
#include <stdbool.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

struct nlmsghdr *nl_link(struct nl_request *r, unsigned short type, unsigned short flags,
                         int index) {
    memset(r, 0, sizeof *r);

    struct nlmsghdr *n = mnl_nlmsg_put_header(r->buf);
    n->nlmsg_type = type;
    n->nlmsg_flags = NLM_F_REQUEST | NLM_F_ACK | flags;

    struct ifinfomsg *i = mnl_nlmsg_put_extra_header(n, sizeof *i);
    i->ifi_family = AF_UNSPEC;
    i->ifi_index = index;

    return n;
}

static int exchange(struct nlmsghdr *n, mnl_cb_t cb, void *data) {
    struct mnl_socket *s = mnl_socket_open2(NETLINK_ROUTE, SOCK_CLOEXEC);

    if (!s)
        return -1;
    int rc = -1;

    struct timeval timeout = {.tv_sec = 3};

    if (setsockopt(mnl_socket_get_fd(s), SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof timeout))
        goto done;
    if (mnl_socket_bind(s, 0, MNL_SOCKET_AUTOPID))
        goto done;
    unsigned port = mnl_socket_get_portid(s);
    n->nlmsg_seq = (unsigned)time(NULL);

    if (mnl_socket_sendto(s, n, n->nlmsg_len) < 0)
        goto done;
    struct nl_request reply;

    for (;;) {
        ssize_t len = mnl_socket_recvfrom(s, reply.buf, sizeof reply.buf);

        if (len < 0)
            goto done;
        int ret = mnl_cb_run(reply.buf, (size_t)len, n->nlmsg_seq, port, cb, data);

        if (ret < 0)
            goto done;
        if (!ret) {
            rc = 0;
            break;
        }
    }
done: {
    int saved = errno;
    mnl_socket_close(s);
    errno = saved;

    return rc;
}
}

int nl_exchange(struct nlmsghdr *n) { return exchange(n, NULL, NULL); }

int nl_up(int index) {
    struct nl_request r;

    struct nlmsghdr *n = nl_link(&r, RTM_NEWLINK, 0, index);

    struct ifinfomsg *i = mnl_nlmsg_get_payload(n);
    i->ifi_change = IFF_UP;
    i->ifi_flags = IFF_UP;

    return nl_exchange(n);
}

int nl_delete(int index) {
    struct nl_request r;

    return nl_exchange(nl_link(&r, RTM_DELLINK, 0, index));
}

int nl_address(int index, const char *address) {
    struct in_addr addr;

    if (inet_pton(AF_INET, address, &addr) != 1) {
        errno = EINVAL;

        return -1;
    }

    struct nl_request r = {0};

    struct nlmsghdr *n = mnl_nlmsg_put_header(r.buf);
    n->nlmsg_type = RTM_NEWADDR;
    n->nlmsg_flags = NLM_F_REQUEST | NLM_F_ACK | NLM_F_CREATE | NLM_F_EXCL;

    struct ifaddrmsg *a = mnl_nlmsg_put_extra_header(n, sizeof *a);
    a->ifa_family = AF_INET;
    a->ifa_prefixlen = 24;
    a->ifa_scope = RT_SCOPE_UNIVERSE;
    a->ifa_index = (unsigned)index;
    mnl_attr_put(n, IFA_LOCAL, sizeof addr, &addr);
    mnl_attr_put(n, IFA_ADDRESS, sizeof addr, &addr);

    return nl_exchange(n);
}

int nl_default(int index, const char *gateway) {
    struct in_addr addr;

    if (inet_pton(AF_INET, gateway, &addr) != 1) {
        errno = EINVAL;

        return -1;
    }

    struct nl_request r = {0};

    struct nlmsghdr *n = mnl_nlmsg_put_header(r.buf);
    n->nlmsg_type = RTM_NEWROUTE;
    n->nlmsg_flags = NLM_F_REQUEST | NLM_F_ACK | NLM_F_CREATE | NLM_F_EXCL;

    struct rtmsg *route = mnl_nlmsg_put_extra_header(n, sizeof *route);
    route->rtm_family = AF_INET;
    route->rtm_table = RT_TABLE_MAIN;
    route->rtm_protocol = RTPROT_BOOT;
    route->rtm_scope = RT_SCOPE_UNIVERSE;
    route->rtm_type = RTN_UNICAST;
    mnl_attr_put_u32(n, RTA_OIF, (unsigned)index);
    mnl_attr_put(n, RTA_GATEWAY, sizeof addr, &addr);

    return nl_exchange(n);
}

struct conflict {
    int bridge;

    bool found;
};

static int route_cb(const struct nlmsghdr *n, void *ptr) {
    struct conflict *c = ptr;

    if (n->nlmsg_len < NLMSG_LENGTH(sizeof(struct rtmsg))) {
        errno = EPROTO;

        return MNL_CB_ERROR;
    }

    struct rtmsg *r = mnl_nlmsg_get_payload(n);

    if (r->rtm_family != AF_INET || !r->rtm_dst_len || r->rtm_dst_len > 32)
        return MNL_CB_OK;
    uint32_t dst = 0, oif = 0;

    const struct nlattr *a;
    mnl_attr_for_each(a, n, sizeof *r) {
        unsigned type = mnl_attr_get_type(a);

        if (type == RTA_DST || type == RTA_OIF) {
            if (mnl_attr_validate(a, MNL_TYPE_U32))
                return MNL_CB_ERROR;
            if (type == RTA_DST)
                dst = ntohl(mnl_attr_get_u32(a));
            else
                oif = mnl_attr_get_u32(a);
        }
    }

    unsigned prefix = r->rtm_dst_len < 24 ? r->rtm_dst_len : 24;
    uint32_t mask = 0xffffffffU << (32 - prefix);

    if ((dst & mask) == (0x0a580000U & mask) && (int)oif != c->bridge)
        c->found = true;
    return MNL_CB_OK;
}

int nl_subnet_conflict(int bridge_index) {
    struct nl_request req = {0};

    struct nlmsghdr *n = mnl_nlmsg_put_header(req.buf);
    n->nlmsg_type = RTM_GETROUTE;
    n->nlmsg_flags = NLM_F_REQUEST | NLM_F_DUMP;

    struct rtmsg *r = mnl_nlmsg_put_extra_header(n, sizeof *r);
    r->rtm_family = AF_INET;

    struct conflict c = {.bridge = bridge_index};

    if (exchange(n, route_cb, &c))
        return -1;
    if (c.found) {
        errno = EADDRINUSE;

        return -1;
    }

    return 0;
}
