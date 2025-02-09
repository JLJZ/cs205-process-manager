#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "shell.h"

int main(void) {
    bool should_exit = false;
    while (!should_exit) {
        cmd_info cmd = cmd_input("> ");
        should_exit = strcasecmp("exit", cmd_name(&cmd));

        for (int i = 0; i < cmd.argc; i++) {
            puts(cmd.argv[i]);
        }

        cmd_free(&cmd);
    };
    return 0;
}
