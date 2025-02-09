#ifndef EXECUTOR_H
#define EXECUTOR_H

typedef struct executor executor;

executor *executor_init(void);

void executor_enqueue_task(executor *e, const char *prog, char *const *args);

void executor_send_command(executor *e, const char *cmd);

#endif
