        return -1;
    n = nl_link(&r, RTM_NEWLINK, 0, other);
    mnl_attr_put_u32(n, IFLA_NET_NS_FD, (unsigned)fd);

    int rc = nl_exchange(n);
    close(fd);

    return rc;
}
