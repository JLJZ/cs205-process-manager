#ifndef INPUT_H
#define INPUT_H

/**
 * @brief Read until EOF
 *
 * @param fd Input file descriptor
 * @param buffer_size The number of bytes to read at once.
 * 64 bytes if 0 specified
 * @param terminator Stop reading when terminator is reached. Not included in
 * the output string
 * @return char* Read string
 */
char *read_all(int fd, size_t buffer_size, char terminator);

#endif
