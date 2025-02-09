#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

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

/**
 * @brief Splits a command string into tokens
 * 
 * @param command_string Target command
 * @param length Number of tokens parsed
 * @return const char** Parsed tokens
 */
static char **parse_command(const char *command_string, size_t *length) {
    size_t cmd_len = strlen(command_string) + 1;
    char cmd[cmd_len];
    strcpy(cmd, command_string);
    for (size_t i = 0; i < cmd_len; ++i) {
        if (cmd[i] == '\r' || cmd[i] == '\n') {
            cmd[i] = '\0';
            break;
        }
    }

    size_t capacity = 4;
    size_t count = 0;
    char **tokens = calloc(capacity, sizeof(char *));
    char *token = strtok(cmd, " ");

    while (token) {
        if (count == capacity) {
            capacity *= 2;
            tokens = realloc(tokens, sizeof(char *) * capacity);
        }

        tokens[count++] = strdup(token);
        token = strtok(NULL, " ");
    }
    
    *length = count;
    return realloc(tokens, count * sizeof(tokens[0]));
}

int main(void) {
    bool should_exit = false;
    char* cmd = NULL;
    
    while (!should_exit) {
        printf("> ");
        cmd = read_line();
        size_t size;
        char **tokens = parse_command(cmd, &size);
        
        for (size_t i = 0; i < size; ++i) {
            puts(tokens[i]);
        }

        if (size) {
            should_exit = !strcasecmp("exit", tokens[0]);
        }
        free(cmd);
        while (size --> 0) free(tokens[size]);
        free(tokens);
    }

    return 0;
}
