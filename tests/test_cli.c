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
    assert(parse_size("", &n));

    struct port_mapping port;
    assert(!parse_port("18080:8080", &port) && port.host == 18080 && port.container == 8080);
    assert(parse_port("0:80", &port));
    assert(parse_port("65536:80", &port));
    assert(parse_port("80:80;id", &port));
    assert(parse_port("-1:80", &port));
    assert(parse_port("80:80:90", &port));
    assert(parse_port("999999999999999999:1", &port));

    char *advanced[] = {"--name", "test",      "--tty",      "--userns", "testuser", "--net",
                        "bridge", "--publish", "18080:8080", "--",       "/bin/sh",  NULL};
    assert(!parse_config(11, advanced, &c) && c.tty && c.publish_count == 1);
    puts("CLI validation tests passed");

    return 0;
}
