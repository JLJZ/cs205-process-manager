#ifndef PROCMAN_H
#define PROCMAN_H

#include <unistd.h>

typedef struct procman procman;

procman *pm_init(void);

void pm_free(procman *sp);

void pm_spawn(procman *sp, const char *const argv[]);

#endif
