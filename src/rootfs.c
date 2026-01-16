#include "rootfs.h"

#include "config.h"
#include "util.h"

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>

int rootfs_pull(void) {
    if (mkdir_safe("/var/lib/simplectr", 0700) || mkdir_safe(IMAGE_BASE, 0700))
        return -1;
    int lock = open(IMAGE_BASE "/.lock", O_CREAT | O_RDWR | O_CLOEXEC | O_NOFOLLOW, 0600);

    if (lock < 0 || flock(lock, LOCK_EX)) {
        if (lock >= 0)
            close(lock);
        return -1;
    }

    struct stat st;

    int rc = -1;

    if (!lstat(IMAGE_BASE "/alpine", &st)) {
        puts("alpine is already cached");
        rc = 0;

        goto out;
    }

    char temp[] = IMAGE_BASE "/.pull-XXXXXX";

    if (!mkdtemp(temp))
        goto out;
    int cwd = open(".", O_RDONLY | O_DIRECTORY | O_CLOEXEC);

    if (cwd < 0 || chdir(temp)) {
        if (cwd >= 0)
            close(cwd);
        goto cleanup;
    }

    char *download[] = {"/usr/bin/curl",
                        "--fail",
                        "--location",
                        "--proto",
                        "=https",
                        "--tlsv1.2",
                        "--silent",
                        "--show-error",
                        "--output",
                        "alpine-minirootfs-3.24.1-aarch64.tar.gz",
                        "https://dl-cdn.alpinelinux.org/alpine/v3.24/releases/aarch64/"
                        "alpine-minirootfs-3.24.1-aarch64.tar.gz",
                        NULL};
    char *checksum[] = {"/usr/bin/curl",
                        "--fail",
                        "--location",
                        "--proto",
                        "=https",
                        "--tlsv1.2",
                        "--silent",
                        "--show-error",
                        "--output",
                        "SHA256",
                        "https://dl-cdn.alpinelinux.org/alpine/v3.24/releases/aarch64/"
                        "alpine-minirootfs-3.24.1-aarch64.tar.gz.sha256",
                        NULL};
    char *verify[] = {"/usr/bin/sha256sum", "--check", "--strict", "SHA256", NULL};

    char *extract[] = {"/usr/bin/tar",
                       "--extract",
                       "--gzip",
                       "--file",
                       "alpine-minirootfs-3.24.1-aarch64.tar.gz",
                       "--directory",
                       "rootfs",
                       "--numeric-owner",
                       NULL};
    if (run_program(download) || run_program(checksum) || run_program(verify) ||
        mkdir("rootfs", 0700) || run_program(extract))

        goto restore;
    if (rename("rootfs", IMAGE_BASE "/alpine"))
        goto restore;
    rc = 0;
    puts("Cached verified Alpine 3.24.1 aarch64 minirootfs");
restore:
    if (fchdir(cwd))
        rc = -1;
    close(cwd);
cleanup:
    if (remove_tree(temp))
        rc = -1;
out:
    close(lock);

    return rc;
}
