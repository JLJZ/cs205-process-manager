#ifndef SPAWN_H
#define SPAWN_H

#include <unistd.h>

typedef struct spawner spawner;

spawner *sp_init(void);

void sp_free(spawner *sp);

void sp_spawn(spawner *sp, const char *const argv[]);

#endif
