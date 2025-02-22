#ifndef INPUT_H
#define INPUT_H

/**
 * @brief Read until EOF
 * 
 * @return char* Read string
 */
char *read_all(int fd, size_t buffer_size);

#endif
