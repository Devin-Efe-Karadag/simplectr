    return s->pid > 1 && s->start && process_start(s->pid) == s->start;
}

int state_supervised(const struct state *s) {
    return s->supervisor > 1 && s->supervisor_start &&
}

static int used_ip(struct state *s, void *ptr) {
    unsigned char *used = ptr;

    if (s->status != STATE_EXITED && s->ip)
        used[s->ip] = 1;
    return 0;
    if (!state_find(c->name, &existing)) {
        return -1;
    if (errno != ENOENT)
        return -1;
    *s = (struct state){.magic = MAGIC,
                        .supervisor = getpid(),
                        .supervisor_start = process_start(getpid()),
                        .created = time(NULL),
                        .memory = c->memory,
                        .swap = c->swap,
                        .pids = c->pids,
                        .quota = c->quota,
                        .uid_base = c->uid_base,
                        .gid_base = c->gid_base};
    s->publish_count = c->publish_count;
    memcpy(s->publish, c->publish, sizeof s->publish);
    strcpy(s->name, c->name);

    unsigned char random[8];

    if (getrandom(random, sizeof random, 0) != sizeof random)
        return -1;
    for (unsigned i = 0; i < 8; i++)
        snprintf(s->id + 2 * i, 3, "%02x", random[i]);
    snprintf(s->veth, sizeof s->veth, "kl%.10s", s->id);

    if (c->bridge) {
        unsigned char used[255] = {0};

        if (state_each(used_ip, used))
            return -1;
        for (int i = 2; i <= 254; i++)
            if (!used[i]) {
                s->ip = i;
                break;
            }
        if (!s->ip) {
            errno = ENOSPC;

            return -1;
    }
    if (state_path(s, "", path) || mkdir(path, 0700))
    if (state_save(s)) {
        (void)rmdir(path);
        return -1;
    return 0;
