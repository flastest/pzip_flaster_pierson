/**
 * wzip
 * Created by Talib Pierson on 9/1/20.
 * Wisconsin zip is a file compression tool.
 * The compression used is run-length encoding.
 */
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

pthread_mutex_t write_to_file_lock;
const int NUM_THREADS = 2;


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

void write_to_file(void *ptr, FILE *stream)
{
    // writes to a file, locks a mutex so 2 threads don't fuck each other up
    pthread_mutex_lock(&write_to_file_lock);
    fwrite(ptr, 4, 1, stream);
    pthread_mutex_unlock(&write_to_file_lock);
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
            
            write_to_file(&count, stdout);
            //fwrite(&count, 4, 1, stdout);
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

    size_t size_of_each_threads_work = size / NUM_THREADS;
    //divvy up the work between all the threads
    for (int pid = 0; i < NUM_THREADS; i++)
    {
        //create a thread
        zip(buffer + pid * )
    }


    //join threads here

    // Run-length-encode the buffer to stdout
    zip(buffer, size);
}
