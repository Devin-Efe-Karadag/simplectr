#ifndef SIMPLE_TERMINAL_H
#define SIMPLE_TERMINAL_H

#include <sys/types.h>

void terminal_resize(int master);

void terminal_drain(int output, int logfd);
