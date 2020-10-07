/**
 * wzip
 * Created by Talib Pierson on 9/1/20.
 * Wisconsin zip is a file compression tool.
 * The compression used is run-length encoding.
 */
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

pthread_mutex_t write_to_file_lock;
const int NUM_THREADS = 1;

struct arg_struct {
    unsigned char *_buffer;
    size_t _size;
};

/**
 * Open a stream for a file.
 * @param filename specifies path to file
 * @param modes mode to open file in
 * @return open new stream for file
 */
FILE *open(const char *filename, const char *modes) {
    FILE *stream = fopen(filename, modes);

    if (stream == NULL) {
        printf("wzip: cannot open file\n");
        exit(EXIT_FAILURE);
    }

    return stream;
}

/**
 * Calculate the size of a file.
 * @param stream specify stream for file
 * @return size of file specified by stream
 */
size_t fsize(FILE *stream) {
    fseek(stream, 0, SEEK_END);
    return ftell(stream);
}

void write_to_file(void *ptr, FILE *stream) {
    // writes to a file, locks a mutex so 2 threads don't fuck each other up
    pthread_mutex_lock(&write_to_file_lock);
    fwrite(ptr, 4, 1, stream);
    pthread_mutex_unlock(&write_to_file_lock);
}

/**
 * Zip run-length-encoding procedure.
 * Prints compressed buffer to stdout.
 * @param buffer to compress
 * @param size of buffer
 */
void zip(const unsigned char *buffer, size_t size) {
    unsigned char curr;
    unsigned char next;
    size_t count = 1;
    for (size_t i = 0; i < size;) {
        curr = buffer[i];
        next = buffer[++i];
        if (curr == next) {
            count += 1;
        } else {
            write_to_file(&count, stdout);
            // fwrite(&count, 4, 1, stdout);
            // printf("%i", count);
            printf("%c", curr);
            count = 1;
        }
    }
}

/**
 * zips for a single thread. Arguments is a pointer to the arg_struct
 * that contains both the arguments needed for unzip.
 * @param arguments
 * @return NULL
 */
void *zip_thread(void *arguments) {
    struct arg_struct *args = (struct arg_struct *) arguments;

    unsigned char *buffer = args->_buffer;
    size_t size = args->_size;

    unsigned char curr;
    unsigned char next;
    size_t count = 1;
    for (size_t i = 0; i < size;) {
        curr = buffer[i];
        next = buffer[++i];
        if (curr == next) {
            count += 1;
        } else {
            write_to_file(&count, stdout);
            // fwrite(&count, 4, 1, stdout);
            // printf("%i", count);
            printf("%c", curr);
            count = 1;
        }
    }
    return NULL;
}

/**
 * File compression tool.
 * Compresses all input files to stdout.
 * @param argc argument count
 * @param argv arguments
 * @return EXIT_SUCCESS iff successful
 */
int main(int argc, char *argv[]) {
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
    unsigned char *buffer = (unsigned char *) malloc(size);

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

    size_t size_of_each_threads_work = size / (size_t) NUM_THREADS;
    pthread_t threads[NUM_THREADS];

    // divvy up the work between all the threads
    for (int pid = 0; pid < NUM_THREADS; ++pid) {
        // create a thread, add it to the list of threads
        struct arg_struct args;
        args._buffer = (buffer + (pid * size_of_each_threads_work));
        args._size = size_of_each_threads_work;

        //simple way to make zip as complressed as possible, check here for
        // first/last thing in threads buffers being the same.
        pthread_create(&threads[pid], NULL, zip_thread, (void *)&args);
    }

    // join threads here
    for (int pid = 0; pid < NUM_THREADS; ++pid) {
        pthread_join(threads[pid], NULL);
    }

    // Run-length-encode the buffer to stdout
    zip(buffer, size);

    return EXIT_SUCCESS;
}
