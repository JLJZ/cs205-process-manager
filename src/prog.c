#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void append_log(
	const pid_t pid,
	const char * const filename,
	const int i,
	const int count
) {
	FILE * file = fopen(filename, "a");
	if (!file) {
		fprintf(stderr, "The file cannot be open for appending.\n");
		exit(EXIT_FAILURE);
	}
	fprintf(file, "[%4d]\tProcess ran %d out of %d seconds.\n", pid, i, count);
	fclose(file);
}

enum {
	ARG_FILENAME = 1,
	ARG_DELAY = 2,
	ARGS_COUNT = ARG_DELAY + 1
};

int main(int argc, char *argv[]) {
	if (argc != ARGS_COUNT) {
		fprintf(stderr, "The program must be run with one argument.\n");
		return EXIT_FAILURE;
	}
	
	const int total_delay = atoi(argv[ARG_DELAY]);
	if (total_delay <= 0) {
		fprintf(stderr, "The first argument must be a positive integer.\n");
		return EXIT_FAILURE;
	}	
	
	const char * const filename = argv[ARG_FILENAME];
	const pid_t pid = getpid();
	const unsigned int delay_seconds = 1;
	
	int i = 0;
	while (i < total_delay) {
		append_log(pid, filename, i++, total_delay);
		sleep(delay_seconds);
	}
	append_log(pid, filename, i, total_delay);
	
	return EXIT_SUCCESS;
}
