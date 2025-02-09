#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "shell.h"

/**
 * @brief Parse string into a new cmd_info
 * 
 * @param cmd_string Command string
 * @return cmd_info Parse command
 */
static cmd_info cmd_parse(const char *cmd_string) {
    char *str_buffer = strdup(cmd_string);

    int argv_size = 16;
    int argc = 0;
    char **argv = malloc(sizeof(char *) * (size_t)argv_size);

    char *tok = strtok(str_buffer, " ");

    while (tok) {
        if (argc == argv_size) {
            argv_size *= 2;
            argv = realloc(argv, sizeof(char *) * (size_t)argv_size);
        }

        argv[argc++] = tok;
        tok = strtok(NULL, " ");
    }
    
    cmd_info cmd = {
        .argc = argc,
        .argv = realloc(argv, (size_t)argc * sizeof(char *)),
        .str_buffer = str_buffer
    };
    
    return cmd;
}

/**
 * @brief Read a command from stdin
 * 
 * @return char* Command string
 */
static char *read_line(void) {
    char *cmd = NULL;
    size_t len = 0;
    
    if (getline(&cmd, &len, stdin) < 0) {
        exit(EXIT_FAILURE);
    }
    
    return cmd;
}

const char *cmd_name(const cmd_info *cmd) {
    return cmd->argv[0];
}

cmd_info cmd_input(const char *prompt) {
    fputs(prompt, stdout);
    char *cmd_string = read_line();
    cmd_info cmd = cmd_parse(cmd_string);
    free(cmd_string);
    return cmd;
}

void cmd_free(cmd_info *cmd) {
    free((void *)cmd->str_buffer);
    free((void *)cmd->argv);
}
