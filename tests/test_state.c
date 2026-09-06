#include "config.h"
#include "state.h"
#include "util.h"

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static void reject_record(const struct state *s, unsigned magic, off_t size) {
    char path[PATH_MAX];
    assert(!state_path(s, "state", path));

    int fd = open(path, O_WRONLY | O_CLOEXEC | O_NOFOLLOW);
    assert(fd >= 0);
    assert(pwrite(fd, &magic, sizeof magic, 0) == sizeof magic);
    assert(!ftruncate(fd, size));
    close(fd);

    struct state loaded;
    assert(state_load(s->id, &loaded) == -1 && errno == EINVAL);

    struct stat st;
    assert(!stat(path, &st) && st.st_size == size); /* Reader must not migrate records. */
    assert(!state_save(s));
}

int main(void) {
    int lock = state_lock();
    assert(lock >= 0);

    struct config c = {.bridge = true,
                       .memory = 268435456,
                       .pids = 64,
                       .quota = 100000,
                       .publish_count = 2,
                       .publish = {{18081, 8080}, {18082, 8080}}};
    snprintf(c.name, sizeof c.name, "state-test-%ld", (long)getpid());

    struct state s, loaded;
    assert(!state_new(&s, &c));
    assert(!state_load(s.id, &loaded));
    assert(loaded.publish_count == 2 && loaded.publish[1].host == 18082);
    reject_record(&s, 0x4b454c31U, (off_t)offsetof(struct state, uid_base));
    reject_record(&s, 0x4b454c32U, (off_t)offsetof(struct state, publish_count));
    reject_record(&s, 0x4b454c32U, sizeof s);
    reject_record(&s, s.magic, sizeof s - 1);
    reject_record(&s, s.magic, sizeof s + 1);
    assert(!state_load(s.id, &loaded) && !strcmp(loaded.name, c.name));

    char path[PATH_MAX];
    assert(!state_path(&s, "", path));
    assert(!remove_tree(path));
    close(lock);
    puts("Current state format and rejection tests passed");
}
