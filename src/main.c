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

static bool cmd_name_equals(const cmd_info *cmd, const char *name) {
    return !strcasecmp(cmd_name(cmd), name);
}

static void dispatch(const cmd_info *cmd) {
    if (cmd_name_equals(cmd, "exit")) {
        exit(EXIT_SUCCESS);

    } else if (cmd_name_equals(cmd, "run")) {
        exec(cmd);
    } else {
        fprintf(stderr, "Unknown command %s\n", cmd_name(cmd));
    }
}

int main(void) {
    while (true) {
        cmd_info cmd = cmd_input("> ");
        
        dispatch(&cmd);

        cmd_free(&cmd);
    }

    return 0;
}
