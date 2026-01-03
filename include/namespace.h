#ifndef SIMPLE_NAMESPACE_H
#define SIMPLE_NAMESPACE_H

#include <sys/types.h>
#define CHILD_STACK (1024 * 1024)

pid_t namespace_clone(int (*entry)(void *), void *arg, void *stack);
