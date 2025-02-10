#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

#include "spawn.h"

#define error(msg) do { perror("[spawner] " msg); } while (0);

#define BUFFER_SIZE 64

struct spawner {
    int pipe[2];
    pid_t server_pid;
};

static char *pack_argv(const char *const argv[], size_t *size) {
    char *bytes = NULL;
    size_t length = 0;
    
    for (int i = 0; argv[i] != NULL; ++i) {
        const char *token = argv[i];
        size_t token_len = strlen(argv[i]) + 1;
        bytes = realloc(bytes, length + token_len);
        memcpy(bytes + length, token, token_len);
        length += token_len;
    }
    
    *size = length;
    
    return bytes;
}

// /**
//  * @brief Segments \0 delimited data into an array of strings.
//  * 
//  * @param data buffer containing tokens delimited by \0 
//  * @param size size of data
//  * @return const char** array of argument tokens
//  */
// static const char **unpack_argv(const char *data, size_t size) {
//     const char** tokens = calloc(1, sizeof(char *));
//     size_t len = 0;
//     size_t i = 0;
    
//     while (i < size) {
//         const char *token = data + i;
//         tokens[len++] = token;

//         /* Allocate 1 more than required for the NULL sentinel value */
//         tokens = realloc(tokens, sizeof(char **) * (len + 1));

//         /* Jump to the next string at the end of this current string */
//         i += strlen(data) + 2;
//     }
//     tokens[len] = NULL;
    
//     return tokens;
// }

static char *read_pipe(int fd) {
    char *str = NULL;
    ssize_t size = 0;

    char buffer[BUFFER_SIZE];
    ssize_t read_size = read(fd, buffer, BUFFER_SIZE);

    while (read_size > 0) {
        str = realloc(str, (size_t)(size + read_size));
        memcpy(str + size, buffer, (size_t)read_size);
        size += read_size;

        /* End of buffer reached */
        if (read_size < BUFFER_SIZE) {
            break;
        }

        read_size = read(fd, buffer, BUFFER_SIZE);
    }
    
    if (read_size == -1) {
        free(str);
        error("read() pipe failed on server");
        return NULL;
    }

    *(str + size) = '\0';
    
    return str;
}

static void sp_server_init(spawner *sp) {
    if (close(sp->pipe[1]) < 0) {
        error("close() pipe failed on server init");
        return;
    }
    
    while (1) {
        char *cmd_str = read_pipe(sp->pipe[0]);
        if (cmd_str == NULL) {
            continue;
        }
        printf("Command: %s\n", cmd_str);

        free(cmd_str);
    }
    
    if (close(sp->pipe[0]) < 0) {
        error("close() pipe failed on server init");
        return;
    }

    exit(EXIT_SUCCESS);
}

static void sp_client_init(spawner *sp) {
    if (close(sp->pipe[0]) < 0) {
        error("close() pipe failed on client init");
        return;
    }
}

spawner *sp_init(void) {
    spawner *sp = calloc(1, sizeof(spawner));
    sp->processes = NULL;
    
    if (pipe(sp->pipe) < 0) {
        error("pipe() init failed");
        goto err;
    }

    pid_t pid = fork();

    switch (pid) {
    case -1:
        error("fork() failed");
        goto err;

    case 0:
        sp->server_pid = getpid();
        sp_server_init(sp);
        break;

    default:
        sp->server_pid = pid;
        sp_client_init(sp);
        break;
    }
    
    return sp;
err:
    free(sp);
    return NULL;
}

void sp_spawn(spawner *sp, const char *const argv[]) {
    size_t size;
    const char* data = pack_argv(argv, &size);
    
    if (write(sp->pipe[1], data, size) < 0) {
        error("write() pipe failed on client");
    }
}

void sp_free(spawner *sp) {
    if (kill(sp->server_pid, SIGTERM) < 0) {
        if (kill(sp->server_pid, SIGKILL) < 0) {
            error("failed to kill server");
        }
    }
}
