/**
 * wzip
 * Created by Talib Pierson on 9/1/20.
 * Wisconsin zip is a file compression tool.
 * The compression used is run-length encoding.
 */
#include <pthread.h>    //for pthreads and mutex
#include <stdio.h>      //for io
#include <stdlib.h>     //idk what this is for

#include <unistd.h>     //for sleep

pthread_mutex_t write_to_file_lock;
const int NUM_THREADS = 2;

struct arg_struct {
    unsigned char *_buffer;
    size_t _size;
};

/**
 * Open a stream for a file.
 * @param filename specifies path to file
 * @param modes mode to get_file_stream file in
 * @return get_file_stream new stream for file
 */
FILE *open_file(const char *filename, const char *modes) {
    FILE *stream = fopen(filename, modes);

    if (stream == NULL) {
        printf("wzip: cannot get_file_stream file\n");
        exit(EXIT_FAILURE);
    }

    return stream;
}

/**
 * Calculate the size of a file.
 * @param stream specify stream for file
 * @return size of file specified by stream
 */
size_t get_stream_size(FILE *stream) {
    fseek(stream, 0, SEEK_END);
    return (size_t) ftell(stream);
}

// writes whatever void *ptr points to to the stream.
// this shoul dbe used at the end when all the threads 
// are done doing the stuff and they want to write to 
// the output thingy
void write_to_file(void *ptr, FILE *stream) {
    // writes to a file, locks a mutex so 2 threads don't fuck each other up
    //pthread_mutex_lock(&write_to_file_lock);
    fwrite(ptr, 4, 1, stream);
    //pthread_mutex_unlock(&write_to_file_lock);
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
 * @param arguments struct: buffer and size
 * @return NULL
 */
void *zip_thread(void *arguments) {
    struct arg_struct *args = (struct arg_struct *) arguments;

    unsigned char *buffer = args->_buffer;
    size_t size = args->_size;

#ifdef DEBUG
    fprintf(stderr, "buffer: %hhn, size: %lu\n", buffer, size);
#endif

    unsigned char curr;
    unsigned char next;
    size_t count = 1;
    for (size_t i = 0; i < size;) {
        curr = buffer[i];
        next = buffer[++i];
        if (curr == next) {
            count += 1;
        } else {
            pthread_mutex_lock(&write_to_file_lock);
            write_to_file(&count, stdout);
            // fwrite(&count, 4, 1, stdout);
            // printf("%i", count);
            printf("%c", curr);
            pthread_mutex_unlock(&write_to_file_lock);
            count = 1;
        }
    }
    return NULL;
}

void *do_nothinggg(){
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
        FILE *stream = open_file(argv[i], "r");
        size += get_stream_size(stream);
        fclose(stream);
    }

    // Make a buffer of the sum-size
    unsigned char *buffer = (unsigned char *) malloc(size);

    size_t n = 0;
    for (int i = 1; i < argc; ++i) {
        // Read all the files into the buffer
        FILE *stream = open_file(argv[i], "r");
        for (int c = fgetc(stream); c > EOF; c = fgetc(stream)) {
            buffer[n] = c;
            ++n;
        }
        fclose(stream);
    }

    size_t size_of_each_threads_work = size / (size_t) NUM_THREADS;
    pthread_t threads[NUM_THREADS];

    //plz be a deep copy
    unsigned char *buffer_ptr = buffer;

    struct arg_struct args[NUM_THREADS];

    // divvy up the work between all the threads
    for (int pid = 0; pid < NUM_THREADS; ++pid) {
        // this keeps track of the size of this buffer
        size_t this_buffer_size = size_of_each_threads_work;

        //if there;s nothign for this thread to do, have it do nothing
        if(buffer_ptr >= buffer + size){
            pthread_create(&threads[pid],NULL, do_nothinggg, NULL);
        }

        // here we check to see if we need to change the size of 
        // the threads work. if multiple of the same letter are
        // spread across multiple thread's works, we just take all
        // of the same letter and give it to a thread.
        // if the thread is the first or last, we don't check it
        while (buffer_ptr != buffer && pid != NUM_THREADS - 1) {
            // check if the letters are the same
            //
            if ( buffer_ptr[this_buffer_size] == buffer_ptr[this_buffer_size+1]) {
                this_buffer_size++;
            }


            //if it just so happens that the remainder of the buffer is
            // all the same number, don't keep trying to look at things
            if ((buffer_ptr + this_buffer_size) == (buffer + size)) 
            {
                break;
            }
        }

        //struct arg_struct args;
        //make sure that the following is a deep copy, not shallow
        struct arg_struct arggies;

        arggies._buffer = buffer_ptr;
        arggies._size = this_buffer_size;

        args[pid] = arggies;

        // simple way to make zip as compressed as possible, check here for
        // first/last thing in threads buffers being the same.
        pthread_create(&threads[pid], NULL, zip_thread, (void *) &args[pid]);

        //sleep(1);
        buffer_ptr = buffer_ptr + this_buffer_size;

    }

    // join threads here
    for (int pid = 0; pid < NUM_THREADS; ++pid) {
        pthread_join(threads[pid], NULL);
    }

    // Run-length-encode the buffer to stdout
    // zip(buffer, size);

    return EXIT_SUCCESS;
}
