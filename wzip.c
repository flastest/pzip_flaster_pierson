/**
 * wzip
 * Talib Pierson & Ariel Flaster
 * September 2020
 * Wisconsin zip is a file compression tool.
 * The compression used is run-length encoding.
 */
#include <stdio.h>
#include <stdlib.h>

FILE *open(const char *filename, const char *modes) {
    /**
     * File opening function
     * Returns open stream to file specified by filename
     */
    FILE *stream = fopen(filename, modes);

    if (stream == NULL) {
        printf("wzip: cannot open file\n");
        exit(EXIT_FAILURE);
    }

    return stream;
}

size_t fsize(FILE *stream) {
    /**
     * File size function
     * Returns siz of file specified by stream
     */
    fseek(stream, 0, SEEK_END);
    return ftell(stream);
}

void zip(const unsigned char *buffer, size_t size) {
    /**
     * Zip run-length-encoding procedure
     * Prints compressed buffer to stdout
     */
    unsigned char curr;
    unsigned char next;
    size_t count = 1;
    for (size_t i = 0; i < size;) {
        curr = buffer[i];
        next = buffer[++i];
        if (curr == next) {
            count += 1;
        } else {
            fwrite(&count, 4, 1, stdout);
            // printf("%i", count);
            printf("%c", curr);
            count = 1;
        }
    }
}

int main(int argc, char *argv[]) {
    /**
     * File compression tool
     * Compresses all input files to stdout
     */
    if (argc < 2) {
        printf("wzip: file1 [file2 ...]\n");
        exit(EXIT_FAILURE);
    }

    size_t size = 0;
    for (int i = 1; i < argc; ++i) {
        // For each file get size
        FILE *stream = open(argv[i], "r");
        size += fsize(stream);
        fclose(stream);
    }

    // Make a buffer of the sum-size
    unsigned char *buffer = malloc(size);

    size_t n = 0;
    for (int i = 1; i < argc; ++i) {
        // Read all the files into the buffer
        FILE *stream = open(argv[i], "r");
        for (int c = fgetc(stream); c > EOF; c = fgetc(stream)) {
            buffer[n] = c;
            ++n;
        }
        fclose(stream);
    }

    // Run-length-encode the buffer to stdout
    zip(buffer, size);
}
