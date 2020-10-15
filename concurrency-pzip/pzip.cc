/**
 * pzip
 * Created by fools on 14 de Octubre.
 * Pissconsin zip is a file compression tool.
 * The compression used is run-length encoding.
 */
#include <cstdio>    // for io
#include <cstdlib>   // atoi and other stuff
#include <iostream>  // also for io
#include <mutex>     // mutex
#include <string>    // for string
#include <thread>    // for peethreads
#include <vector>    // for something, can't put my finger on it tho

#define NUM_THREADS 2

std::mutex write_to_file_lock;
static std::string *array_of_strings[NUM_THREADS];  // i think this is correct

/**
 * Open a stream for a file.
 * @param filename specifies path to file
 * @param modes mode to get_file_stream file in
 * @return get_file_stream new stream for file
 */
static FILE *open_file(const char *filename, const char *modes) {
    FILE *stream = fopen(filename, modes);

    if (stream == nullptr) {
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
static size_t get_stream_size(FILE *stream) {
    fseek(stream, 0, SEEK_END);
    return (size_t) ftell(stream);
}

/**
 * zips for a single thread. Arguments is a pointer to the arg_struct
 * that contains both the arguments needed for unzip.
 * @param arguments struct: buffer and size
 * @return NULL
 */
static void *zip_thread(const unsigned char *buffer, size_t size,
                        unsigned int pid) {
    // #ifdef DEBUG
    //     fprintf(stderr, "buffer: %s, size: %lu\n", buffer, size);
    // #endif

    unsigned char curr;
    unsigned char next;
    size_t count = 1;
    for (size_t i = 0; i < size;) {
        curr = buffer[i];
        next = buffer[++i];
        if (curr == next) {
            count += 1;
        } else {
            // we probably don't need this, right?
            // std::lock_guard<std::mutex> guard(write_to_file_lock);

            // TODO: change this stuff to modify vectors
            // instead of just printing them
            array_of_strings[pid]->append(std::to_string(count));
            array_of_strings[pid]->append(reinterpret_cast<const char *>(curr));
            // idk how to add two strings here
            // fwrite(&count, 4, 1, stdout);
            // printf("%i", count);
            count = 1;
        }
    }
    return nullptr;
}

// iterates through a char* and merges things like 4a5a to become something
// nice like 9a.
static std::string merge() {
    std::string ret;  // sorry eitan I'm being lazy

    auto prev_num = array_of_strings[0]->substr(0, 1);
    auto prev_char = array_of_strings[0]->substr(1, 2);

    auto len = array_of_strings[0]->length();
    auto cur_str = array_of_strings[0]->substr(0, len - 1);

    for (int i = 1; i < NUM_THREADS; ++i) {
        len = array_of_strings[i]->length();
        cur_str = array_of_strings[i]->substr(0, len - 1);

        // check the ending of the string
        auto beg_of_str_num = array_of_strings[i]->substr(0, 1);
        auto beg_of_str_char = array_of_strings[i]->substr(0, 1);

        // if the things are equal, add
        if (prev_char == beg_of_str_char) {
            prev_num =
                    std::to_string(std::stoi(prev_num)
                    + std::stoi(beg_of_str_num));
            ret.append(prev_num);
            ret.append(prev_char);
            ret.append(cur_str.substr(2, len - 3));
        } else {  // just append something to ret
            ret.append(prev_num);
            ret.append(prev_char);
            ret.append(cur_str.substr(0, len - 3));
        }

        auto prev_num = array_of_strings[i]->substr(len - 3, len - 2);
        auto prev_char = array_of_strings[i]->substr(len - 2, len - 1);
    }

    return ret;
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
    auto *buffer = (unsigned char *) malloc(size);

    size_t n = 0;
    for (int i = 1; i < argc; ++i) {
        // Read all the files into the buffer
        FILE *stream = open_file(argv[i], "r");
        for (int c = fgetc(stream); c > EOF; c = fgetc(stream)) {
            buffer[n] = (unsigned char) c;
            ++n;
        }
        fclose(stream);
    }

    size_t size_of_each_threads_work = size / (size_t) NUM_THREADS;
    // pthread_t threads[NUM_THREADS];
    std::vector<std::thread> threads;

    // plz be a deep copy
    unsigned char *buffer_ptr = buffer;

    // divvy up the work between all the threads
    for (int pid = 0; pid < NUM_THREADS; ++pid) {
        // this keeps track of the size of this buffer
        size_t this_buffer_size = size_of_each_threads_work;

        // here we check to see if we need to change the size of
        // the threads work. if multiple of the same letter are
        // spread across multiple thread's works, we just take all
        // of the same letter and give it to a thread.
        // if the thread is the first or last, we don't check it
        while (buffer_ptr != buffer && pid != NUM_THREADS - 1) {
            // check if the letters are the same
            //
            if (buffer_ptr[this_buffer_size] ==
                buffer_ptr[this_buffer_size + 1]) {
                this_buffer_size++;
            }

            // if it just so happens that the remainder of the buffer is
            // all the same number, don't keep trying to look at things
            if ((buffer_ptr + this_buffer_size) == (buffer + size)) {
                break;
            }
        }

        // simple way to make zip as compressed as possible, check here for
        // first/last thing in threads buffers being the same.
        threads.emplace_back(std::thread(
                [=]() { zip_thread(buffer_ptr, this_buffer_size, pid); }));

        buffer_ptr = buffer_ptr + this_buffer_size;
    }

    // join threads here
    for (size_t pid = 0; pid < NUM_THREADS; ++pid) {
        threads[pid].join();
    }

    std::string the_string = merge();

    /// hahaHAAHHAHAHAHAHAHAHAHA
    std::cout << the_string << std::flush;
    // Run-length-encode the buffer to stdout
    // zip(buffer, size);

    free(buffer);

    return EXIT_SUCCESS;
}
