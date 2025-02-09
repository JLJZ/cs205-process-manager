#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#include "shell.h"

static void exec(const cmd_info *cmd) {
    if (cmd->argc < 2) {
        fprintf(stderr, "Usage: run [exe] [args]\n");
        return;
    }

    char **argv = malloc((size_t)cmd->argc * sizeof(char *));

    /* Copy argument tokens, ignoring first token */
    memcpy(argv, cmd->argv + 1, sizeof(char *) * (size_t)cmd->argc);

    /* Set last argument token to NULL for execvp() */
    *(argv + cmd->argc - 1) = NULL;

    fprintf(stderr, "Error %d\n", execvp(argv[0], argv));
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
