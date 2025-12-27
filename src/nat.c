#include "nat.h"

#include "config.h"
#include "util.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

/* Fixed program path and argv. Rules arrive through stdin, never through a shell. */
int nat_transaction(const char *rules, char *output, size_t size) {
    int in[2], out[2];

    if (pipe2(in, O_CLOEXEC))
        return -1;
    if (pipe2(out, O_CLOEXEC)) {
        close(in[0]);
        close(in[1]);

        return -1;
    }

    pid_t pid = fork();

    if (pid < 0) {
        close(in[0]);
        close(in[1]);
        close(out[0]);
        close(out[1]);

        return -1;
    }

    if (!pid) {
        if (dup2(in[0], 0) < 0 || dup2(out[1], 1) < 0)
            _exit(126);
        (void)close_range(3, ~0U, 0);

        char *argv[] = {"/usr/sbin/nft", "-f", "-", NULL};

        char *env[] = {"PATH=/usr/sbin:/usr/bin:/sbin:/bin", "LC_ALL=C", NULL};
        execve(argv[0], argv, env);
        _exit(127);
    }
    close(in[0]);
    close(out[1]);

    ssize_t sent = write(in[1], rules, strlen(rules));
    close(in[1]);

    size_t used = 0;

    char buf[4096];

    ssize_t n;

    while ((n = read(out[0], buf, sizeof buf)) > 0) {
        if (output && size > used + 1) {
            size_t take = (size_t)n;

            if (take > size - used - 1)
                take = size - used - 1;
            memcpy(output + used, buf, take);
            used += take;
        }
    }

    if (output && size)
        output[used] = 0;
    close(out[0]);

    int status;

    while (waitpid(pid, &status, 0) < 0)
        if (errno != EINTR)
            return -1;
    if (sent != (ssize_t)strlen(rules) || n < 0 || !WIFEXITED(status) || WEXITSTATUS(status)) {
        errno = EIO;

        return -1;
    }

    return 0;
}

static int owned_table(void) {
    char buf[8192];

    if (nat_transaction("list tables\n", buf, sizeof buf))
        return -1;
    if (!strstr(buf, "table ip simplectr_nat\n"))
        return 0;
    if (nat_transaction("list table ip simplectr_nat\n", buf, sizeof buf))
        return -1;
    if (!strstr(buf, "comment \"simplectr-owned-v1\"")) {
        errno = EEXIST;

        return -1;
    }

    return 1;
}

int nat_setup(void) {
    int own = owned_table();

    if (own < 0)
        return -1;
    if (!own) {
        const char rules[] =
            "add table ip simplectr_nat { comment \"simplectr-owned-v1\"; }\n"
            "add chain ip simplectr_nat postrouting { type nat hook postrouting "
            "priority srcnat; policy accept; }\n"
            "add rule ip simplectr_nat postrouting ip saddr 10.88.0.0/24 oifname != "
            "\"simplectr0\" masquerade\n";
        if (nat_transaction(rules, NULL, 0))
            return -1;
    }

    char old[32];

    if (read_file("/proc/sys/net/ipv4/ip_forward", old, sizeof old))
        return -1;
    int fd =
        open(STATE_BASE "/ip_forward", O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW | O_CLOEXEC, 0600);
    if (fd >= 0) {
        ssize_t n = write(fd, old, strlen(old));
        close(fd);

        if (n != (ssize_t)strlen(old))
            return -1;
    } else if (errno != EEXIST)
        return -1;
    return write_file("/proc/sys/net/ipv4/ip_forward", "1");
}

int nat_cleanup(void) {
    int own = owned_table();

    if (own < 0)
        return -1;
    if (own && nat_transaction("delete table ip simplectr_nat\n", NULL, 0))
        return -1;
    char old[32];

    if (!read_file(STATE_BASE "/ip_forward", old, sizeof old)) {
        if (strcmp(old, "0\n") && strcmp(old, "1\n")) {
            errno = EINVAL;

            return -1;
        }

        if (write_file("/proc/sys/net/ipv4/ip_forward", old) || unlink(STATE_BASE "/ip_forward"))
            return -1;
    } else if (errno != ENOENT)
        return -1;
    return 0;
}
