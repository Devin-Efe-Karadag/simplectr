#include "overlay.h"

#include "config.h"
#include "util.h"

#include <limits.h>
#include <stdio.h>
#include <sys/mount.h>

int overlay_dirs(const char *base) {
    const char *dirs[] = {"lower", "upper", "work", "merged"};

    char path[PATH_MAX];

    for (unsigned i = 0; i < 4; i++)
        if (path_join(path, sizeof path, base, dirs[i]) || mkdir_safe(path, 0700))
            return -1;
    return 0;
}

int overlay_mount(const char *base) {
    char lower[PATH_MAX], merged[PATH_MAX], options[PATH_MAX * 3];

    if (path_join(lower, sizeof lower, base, "lower") ||
        path_join(merged, sizeof merged, base, "merged"))

        return -1;
    if (mount(IMAGE_BASE "/alpine", lower, NULL, MS_BIND | MS_REC, NULL))
        return -1;
    if (mount(NULL, lower, NULL, MS_BIND | MS_REMOUNT | MS_RDONLY | MS_NOSUID | MS_NODEV, NULL))
        return -1;
    int n = snprintf(options, sizeof options, "lowerdir=%s,upperdir=%s/upper,workdir=%s/work",
                     lower, base, base);
    if (n < 0 || (size_t)n >= sizeof options)
        return -1;
    return mount("overlay", merged, "overlay", MS_NODEV | MS_NOSUID, options);
}
