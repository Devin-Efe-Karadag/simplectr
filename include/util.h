#ifndef SIMPLE_UTIL_H
#define SIMPLE_UTIL_H

#include <stddef.h>

#include <sys/types.h>

int mkdir_safe(const char *path, mode_t mode);

int write_file(const char *path, const char *value);

int read_file(const char *path, char *buf, size_t size);

int path_join(char *out, size_t size, const char *base, const char *leaf);

int remove_tree(const char *path);

int run_program(char *const argv[]);

int environment_check(void);

unsigned long long process_start(pid_t pid);
#endif
