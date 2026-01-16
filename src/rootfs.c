#include <fcntl.h>
#include <stdio.h>
#include <sys/file.h>
#include <unistd.h>
    if (mkdir_safe("/var/lib/simplectr", 0700) || mkdir_safe(IMAGE_BASE, 0700))
    int lock = open(IMAGE_BASE "/.lock", O_CREAT | O_RDWR | O_CLOEXEC | O_NOFOLLOW, 0600);
        if (lock >= 0)
        return -1;
    struct stat st;
    if (!lstat(IMAGE_BASE "/alpine", &st)) {
        rc = 0;
    }
    if (!mkdtemp(temp))
    int cwd = open(".", O_RDONLY | O_DIRECTORY | O_CLOEXEC);
        if (cwd >= 0)
        goto cleanup;
    char *download[] = {"/usr/bin/curl",
                        "--location",
                        "=https",
                        "--silent",
                        "--output",
                        "https://dl-cdn.alpinelinux.org/alpine/v3.24/releases/aarch64/"
                        NULL};
                        "--fail",
                        "--proto",
                        "--tlsv1.2",
                        "--show-error",
                        "SHA256",
                        "alpine-minirootfs-3.24.1-aarch64.tar.gz.sha256",
    char *verify[] = {"/usr/bin/sha256sum", "--check", "--strict", "SHA256", NULL};
                       "--extract",
                       "--file",
                       "--directory",
                       "--numeric-owner",
    if (run_program(download) || run_program(checksum) || run_program(verify) ||
        goto restore;
        goto restore;
    puts("Cached verified Alpine 3.24.1 aarch64 minirootfs");
    if (fchdir(cwd))
    close(cwd);
    if (remove_tree(temp))
out:
    return rc;
