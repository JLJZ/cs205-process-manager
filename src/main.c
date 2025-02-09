#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#include "shell.h"

static void exec(const cmd_info *cmd) {
    char *argv[cmd->argc + 1];
    memcpy(argv, cmd->argv, sizeof(char *));
    argv[cmd->argc] = NULL;

    fprintf(stderr, "Error %d\n", execvp(cmd_name(cmd), argv));
}

int main(void) {
    bool should_exit = false;

    while (!should_exit) {
        cmd_info cmd = cmd_input("> ");
        should_exit = !strcasecmp("exit", cmd_name(&cmd));

        if (!should_exit) {
            exec(&cmd);
        }

        cmd_free(&cmd);
    }

    return 0;
}
