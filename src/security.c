#include "security.h"

#include <errno.h>
#include <linux/sched.h>
#include <sys/capability.h>
int security_apply(void) {
     * ICMP. */
        return -1;
        if (prctl(PR_CAPBSET_DROP, cap, 0, 0, 0))
            return -1;
    if (!caps)
    int rc = cap_set_proc(caps);
    if (rc)
        return -1;
