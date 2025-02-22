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

/**
 * @brief Split string by linefeed. Not re-entrant because of internal strtok()
 * 
 * @param str Input string to split. Mutates the string
 * @param[out] count Return value of the number of lines parsed
 * @return char** List of strings lines
 * 
 * @note The array returned has to be freed, but no memory is allocated for
 * its items. Deallocate the passed in string argument to free all the items. 
 */
char **split_lines(char *str, size_t *count);

#endif
