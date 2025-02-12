#ifndef PROCMAN_H
#define PROCMAN_H

#include <unistd.h>

typedef struct procman procman;

procman *pm_init(void);

void pm_shutdown(procman *pm, unsigned int retries);

void pm_execute(procman *pm, const char *cmd_string);

#endif
