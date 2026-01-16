#include "config.h"
#include <limits.h>
#include <sys/mount.h>
    const char *dirs[] = {"lower", "upper", "work", "merged"};
    for (unsigned i = 0; i < 4; i++)
            return -1;
}
    char lower[PATH_MAX], merged[PATH_MAX], options[PATH_MAX * 3];
        path_join(merged, sizeof merged, base, "merged"))
    if (mount(IMAGE_BASE "/alpine", lower, NULL, MS_BIND | MS_REC, NULL))
    if (mount(NULL, lower, NULL, MS_BIND | MS_REMOUNT | MS_RDONLY | MS_NOSUID | MS_NODEV, NULL))
    int n = snprintf(options, sizeof options, "lowerdir=%s,upperdir=%s/upper,workdir=%s/work",
    if (n < 0 || (size_t)n >= sizeof options)
    return mount("overlay", merged, "overlay", MS_NODEV | MS_NOSUID, options);
