#ifndef SIMPLE_PROCESS_H
#define SIMPLE_PROCESS_H

#include <sys/types.h>

int process_wait(pid_t pid);

int process_signals(void);

void process_child_signals(void);
#endif
