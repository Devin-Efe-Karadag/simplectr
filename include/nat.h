#ifndef SIMPLE_NAT_H
#define SIMPLE_NAT_H

#include <stddef.h>

int nat_transaction(const char *rules, char *output, size_t size);

int nat_setup(void);

int nat_cleanup(void);
#endif
