                continue;
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
