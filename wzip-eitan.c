/*
 * A simple run-length encoder in C
 * The resulting file format is <length> (32-bit unsigned binary) followed
 * by a literal (b-bit character), repeated for as long as necessary.
 */

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/**
 * Read and compress the contents of a file to stdout.
 * @param fp input file pointer
 */
void zip(FILE *fp) {
    int curr, last = fgetc(fp);
    uint32_t count;

    while (last != EOF) {
        // Count repeats of last
        for (count = 1; (curr = fgetc(fp)) == last; count++) {
        }

        if (fwrite(&count, sizeof(count), 1, stdout) < 1) {
            perror("Can't write to stdout");
            exit(1);
        }
        printf("%c", last);
        last = curr;
    }

    if (!feof(fp)) {
        perror("Can't read from file");
        exit(EXIT_FAILURE);
    }
}

/**
 * Read all files listed in the argument list after a search term,
 * and output matching lines to standard output.
 * @param argc argument count
 * @param argv arguments
 * @return EXIT_SUCCESS iff successful
 */
int main(int argc, char **argv) {
    if (argc <= 1) {
        fprintf(stderr, "%s: searchterm file ...\n", argv[0]);
        exit(1);
    }

    for (int i = 1; i < argc; i++) {
        FILE *in = fopen(argv[i], "r");
        if (in == NULL) {
            perror("wunzip: cannot open file");
            exit(EXIT_FAILURE);
        }
        zip(in);
        fclose(in);
    }

    return EXIT_SUCCESS;
}
