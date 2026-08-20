#ifndef SIMPLE_TERMINAL_H
#define SIMPLE_TERMINAL_H

#include <sys/types.h>

void terminal_resize(int master);

void terminal_drain(int output, int logfd);

int terminal_child(uid_t uid, gid_t gid);

int terminal_send(int socket, char byte, int fd);

int terminal_receive(int socket, char *byte, int *fd);

int terminal_relay(pid_t pid, int output, int logfd, int signals, int tty);
#endif
