#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

void DieWithError(char *errorMessage) {
    perror(errorMessage);
    exit(EXIT_FAILURE);
}
