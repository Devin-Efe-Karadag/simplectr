#ifndef SIMPLE_CLI_H
#define SIMPLE_CLI_H

int container_list(void);

int container_inspect(const char *name);

int container_logs(const char *name);

int container_stop(const char *name);

int container_cleanup(void);
#endif
