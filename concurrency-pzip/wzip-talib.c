/**
 * wzip
 * Talib Pierson
 * October 2020
 * Wisconsin zip is a simple command-line run-length encoding utility in C.
 * The resulting file format is <length> (32-bit unsigned binary)
 * followed by a literal (b-bit character), repeated for as long as necessary.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct c_count_t {
    unsigned char c;
    uint32_t count;
};

struct sequence {
    struct c_count_t *counts;
    size_t size;
};

static struct c_count_t c_count = {'\0', 0};

/**
 * Open a file in read mode and create a new stream for it.
 * @param filename pathname specifies file to open
 * @return new stream to file specified by filename
 */
static FILE *get_file_stream(const char *filename) {
    FILE *stream = fopen(filename, "r");

    if (stream == NULL) {
        printf("wzip: cannot open file\n");
        exit(EXIT_FAILURE);
    }

    return stream;
}

/**
 * Get the size of a stream and return it.
 * @param stream specify stream to get the size of
 * @return the size of the stream specified
 */
static size_t get_stream_size(FILE *stream) {
    fseek(stream, 0, SEEK_END);
    return (size_t) ftell(stream);
}

/**
 * Write 32-bit unsigned binary data to stdout.
 * @param count 32-bit unsigned binary to write.
 */
static void write_out_binary(uint32_t count) {
    if (fwrite(&count, sizeof(count), 1, stdout) < 1) {
        perror("Can't write to stdout");
        exit(EXIT_FAILURE);
    }
}

/**
 * Run-length encode buffer.
 * Prints compressed buffer to stdout.
 * @param buffer string to compress
 * @param buffer_size size of buffer
 */
static void zip(const unsigned char *buffer, size_t buffer_size) {
    if (buffer_size > 0) {
        unsigned char curr;
        unsigned char last = buffer[0];  // warning:
        // Assigned value is garbage or undefined
        uint32_t count;

        for (size_t i = 1; i < buffer_size;) {
            for (count = 1; (curr = buffer[i++]) == (last); ++count) {  // warning:
                // Assigned value is garbage or undefined
            }
            write_out_binary(count);
            printf("%c", last);
            last = curr;
        }
    }
}

/**
 * Compress all input files to stdout.
 * @param argc number of arguments
 * @param argv array of arguments
 * @return EXIT_SUCCESS if successful
 */
int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("wzip: file1 [file2 ...]\n");
        exit(EXIT_FAILURE);
    }

    size_t buffer_size = 0;
    for (int i = 1; i < argc; ++i) {
        // For each file get size
        FILE *file_stream = get_file_stream(argv[i]);
        buffer_size += get_stream_size(file_stream);
        fclose(file_stream);
    }

    // Make a buffer of buffer_size
    unsigned char *buffer = (unsigned char *) malloc(buffer_size);

    size_t buffer_index = 0;
    for (int i = 1; i < argc; ++i) {
        // Read all the files into the buffer
        FILE *file_stream = get_file_stream(argv[i]);
        for (int c = fgetc(file_stream); c > EOF; c = fgetc(file_stream)) {
            buffer[buffer_index] = (unsigned char) c;
            ++buffer_index;
        }
        fclose(file_stream);
    }

    // Run-length-encode the buffer to stdout
    zip(buffer, buffer_size);

    free(buffer);

    return EXIT_SUCCESS;
}
