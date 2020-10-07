/**
 * wunzip
 * Created by Talib Pierson on 9/8/20.
 * Wisconsin unzip is a file decompression tool.
 * The compression used is run-length encoding.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

FILE *open(const char *filename, const char *modes) {
    /**
     * File opening function
     * Returns open stream to file specified by filename
     */
    FILE *stream = fopen(filename, modes);

    if (stream == NULL) {
        printf("wunzip: cannot open file\n");
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

void wunzip(const unsigned char *buffer, size_t size) {
    /**
     * Run length decoding procedure
     * Prints decompressed stream to stdout
     */
    size_t count;
    unsigned char c;

    for (size_t i = 0; i + 4 < size; i += 5) {
        count = buffer[i] |
                buffer[i + 1] << 8u |
                buffer[i + 2] << 16u |
                buffer[i + 3] << 24u;
        c = buffer[i + 4];

        for (; count > 0; --count) {
            printf("%c", c);
        }
    }
}

int main(int argc, char *argv[]) {
    /**
     * File decompression tool
     * Decompresses input file to stdout
     */
    if (argc < 2) {
        printf("wunzip: file1 [file2 ...]\n");
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

    // Run-length-decode the buffer to stdout
    wunzip(buffer, size);
}
