#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#include "procman.h"
#include "argparse.h"

/**
 * @brief Read a command from stdin
 * 
 * @return char* Command string
 */
static void read_command(args *a) {
    char *input = NULL;
    size_t len = 0;
    
    if (getline(&input, &len, stdin) < 0) {
        exit(EXIT_FAILURE);
    }

    args_parse(a, input);
    
    free(input);
}

int main(void) {
    procman *pm = pm_init();

    while (true) {
        // cmd_info cmd = cmd_input("> ");
        printf("cs205$ ");

        args a;
        read_command(&a);

        pm_spawn(pm, (const char *const *)a.argv);

        args_free(&a);
    }
    
    pm_free(pm);

    return 0;
}
