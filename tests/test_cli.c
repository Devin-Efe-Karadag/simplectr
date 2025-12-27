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
