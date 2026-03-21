#include "config.h"

#include <assert.h>
#include <stdio.h>

int main(void) {
    uint64_t n;
    assert(valid_name("web-1"));
    assert(!valid_name("../bad"));
    assert(!valid_name(""));
    assert(!parse_size("256M", &n) && n == 268435456);
    assert(parse_size("-1", &n));
    assert(parse_size("18446744073709551615G", &n));

    char *args[] = {"--name", "test", "--cpus", "0.5", "--", "/bin/sh", "-c", "exit 7", NULL};

    struct config c;
    assert(!parse_config(8, args, &c));
    assert(c.quota == 50000);
    assert(c.argv == args + 5);

    char *bad[] = {"--name", "../escape", "--", "/bin/true", NULL};
    assert(parse_config(4, bad, &c));

    char *dup[] = {"--name", "a", "--name", "b", "--", "/bin/true", NULL};
    assert(parse_config(6, dup, &c));

    char *nan[] = {"--name", "a", "--cpus", "nan", "--", "/bin/true", NULL};
    assert(parse_config(6, nan, &c));

    char *unknown[] = {"--name", "a", "--publish", "80:80", "--", "/bin/true", NULL};
    assert(parse_config(6, unknown, &c));

    char *relative[] = {"--name", "a", "--", "bin/sh", NULL};
    assert(parse_config(4, relative, &c));
    assert(!valid_name("x;touch-x"));
    assert(!valid_name("a/b"));
    assert(parse_size("1MB", &n));
