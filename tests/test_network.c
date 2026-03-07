#include "netlink.h"

#include <assert.h>
#include <errno.h>
#include <libmnl/libmnl.h>
#include <linux/if_link.h>
#include <linux/rtnetlink.h>
#include <net/if.h>
#include <sched.h>
#include <stdio.h>

int main(void) {
    assert(!unshare(CLONE_NEWNET));

    int lo = (int)if_nametoindex("lo");
    assert(lo > 0);
    assert(!nl_up(lo));

    struct nl_request r;

    struct nlmsghdr *n = nl_link(&r, RTM_NEWLINK, NLM_F_CREATE | NLM_F_EXCL, 0);
    struct nlattr *info = mnl_attr_nest_start(n, IFLA_LINKINFO);
    mnl_attr_put_strz(n, IFLA_INFO_KIND, "dummy");
    mnl_attr_nest_end(n, info);
    assert(!nl_exchange(n));

    int idx = (int)if_nametoindex("simplectr-test0");
    assert(idx > 0);
    assert(!nl_address(idx, "10.88.0.1"));
    assert(!nl_up(idx));
    assert(!nl_subnet_conflict(idx));
    assert(!if_nametoindex("simplectr-test0"));
}
