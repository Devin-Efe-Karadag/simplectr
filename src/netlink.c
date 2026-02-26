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
