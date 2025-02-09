#ifndef SHELL_H
#define SHELL_H

typedef struct cmd_info {
    int argc;
    const char *const *const argv;
    const char *const str_buffer; /* Allocated block for tokens */
} cmd_info;

const char *cmd_name(const cmd_info *cmd);

cmd_info cmd_input(const char *prompt);

void cmd_free(cmd_info *cmd);

#endif
