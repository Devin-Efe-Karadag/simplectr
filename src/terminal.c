#include "terminal.h"

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/signalfd.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>

int terminal_child(uid_t uid, gid_t gid) {
    int master = posix_openpt(O_RDWR | O_NOCTTY | O_CLOEXEC);

    if (master < 0)
        return -1;
    if (grantpt(master) || unlockpt(master))
        goto fail;
    char name[128];

    if (ptsname_r(master, name, sizeof name))
        goto fail;
    int slave = open(name, O_RDWR | O_NOCTTY | O_CLOEXEC);

    if (slave < 0)
        goto fail;
    if (fchown(slave, uid, gid) || ioctl(slave, TIOCSCTTY, 0) || dup2(slave, 0) < 0 ||
        dup2(slave, 1) < 0 || dup2(slave, 2) < 0) {
        int saved = errno;
        close(slave);
        errno = saved;

        goto fail;
    }
    close(slave);

    return master;
fail: {
    int saved = errno;
    close(master);
    errno = saved;

    return -1;
}
}

int terminal_send(int socket, char byte, int fd) {
    struct iovec iov = {&byte, 1};

    union {
        struct cmsghdr align;

        char bytes[CMSG_SPACE(sizeof(int))];
    } control = {0};

    struct msghdr msg = {.msg_iov = &iov, .msg_iovlen = 1};

    if (fd >= 0) {
        msg.msg_control = control.bytes;
        msg.msg_controllen = sizeof control.bytes;

        struct cmsghdr *c = CMSG_FIRSTHDR(&msg);
        c->cmsg_level = SOL_SOCKET;
        c->cmsg_type = SCM_RIGHTS;
        c->cmsg_len = CMSG_LEN(sizeof(int));
        memcpy(CMSG_DATA(c), &fd, sizeof fd);
    }

    return sendmsg(socket, &msg, MSG_NOSIGNAL) == 1 ? 0 : -1;
}

int terminal_receive(int socket, char *byte, int *fd) {
    struct iovec iov = {byte, 1};

    union {
        struct cmsghdr align;

        char bytes[CMSG_SPACE(sizeof(int))];
    } control = {0};

    struct msghdr msg = {.msg_iov = &iov,
                         .msg_iovlen = 1,
                         .msg_control = control.bytes,
                         .msg_controllen = sizeof control.bytes};
    *fd = -1;

    ssize_t n = recvmsg(socket, &msg, MSG_CMSG_CLOEXEC);

    if (n != 1) {
        if (!n)
            errno = EPIPE;
        return -1;
    }

    struct cmsghdr *c = CMSG_FIRSTHDR(&msg);

    if (c && c->cmsg_level == SOL_SOCKET && c->cmsg_type == SCM_RIGHTS &&
        c->cmsg_len == CMSG_LEN(sizeof(int)))
        memcpy(fd, CMSG_DATA(c), sizeof *fd);
    if (msg.msg_flags & MSG_CTRUNC) {
        if (*fd >= 0)
            close(*fd);
        *fd = -1;
        errno = EPROTO;

        return -1;
    }

    return 0;
}

static int write_all(int fd, const char *buf, size_t size) {
    while (size) {
        ssize_t n = write(fd, buf, size);

        if (n < 0 && errno == EINTR)
            continue;
        if (n <= 0)
            return -1;
        buf += n;
        size -= (size_t)n;
    }

    return 0;
    while (!exited || !eof) {
        struct pollfd fds[] = {
            {eof ? -1 : output, POLLIN, 0}, {signals, POLLIN, 0}, {input ? 0 : -1, POLLIN, 0}};
        if (poll(fds, 3, 200) < 0) {
            if (errno == EINTR)
                continue;
            goto done;
        }
                    if (size > 16 * 1024 * 1024 - logged)
                        size = 16 * 1024 * 1024 - logged;
                    if (write_all(logfd, buf, size))
                        goto done;
                    logged += size;
                }
            } else if (!n || (tty && errno == EIO)) {
                eof = true;
                input = false;
            } else if (errno != EINTR)
                goto done;
        }
