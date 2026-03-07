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
