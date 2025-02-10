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

static void sp_init_server(spawner *sp) {
    if (close(sp->pipe[1]) < 0) {
        error("close() pipe failed on server init");
        return;
    }
    
    while (1) {
        char *cmd_str = read_pipe_command(sp->pipe[0]);
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

static void sp_init_client(spawner *sp) {
    if (close(sp->pipe[0]) < 0) {
        error("close() pipe failed on client init");
        return;
    }
}

spawner *sp_init(void) {
    spawner *sp = calloc(1, sizeof(spawner));
    
    if (pipe(sp->pipe) < 0) {
        perror("pipe() init failed");
        return NULL;
    }

    pid_t pid = fork();

    switch (pid) {
    case -1:
        perror("fork() failed");
        return NULL;

    case 0:
        sp->server_pid = getpid();
        sp_init_server(sp);
        break;

    default:
        sp->server_pid = pid;
        sp_init_client(sp);
        break;
    }
    
    return sp;
}

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
