#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(void) {
    char buf[4096];
    ssize_t n;

    while ((n = read(STDIN_FILENO, buf, sizeof(buf))) > 0) {
        for (ssize_t i = 0; i < n; i++) {
            if (buf[i] == ' ' || buf[i] == '\t') {
                buf[i] = '_';
            }
        }

        if (write(STDOUT_FILENO, buf, n) != n) {
            perror("child2: write");
            return EXIT_FAILURE;
        }
    }
    if (n < 0) { perror("child2: read"); return EXIT_FAILURE; }
    return 0;
}