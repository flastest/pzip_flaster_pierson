/**
 * pzip
 * Created by fools on 14 de Octubre.
 * Pissconsin zip is a file compression tool.
 * The compression used is run-length encoding.
 */
#include <cstdio>    // for io
#include <cstdlib>   // atoi and other stuff
#include <iostream>  // also for io
#include <string>    // for string
#include <thread>    // for peethreads
#include <vector>    // for something, can't put my finger on it tho

#define NUM_THREADS 1

static std::vector<std::byte> array_of_buffers[NUM_THREADS];  // i think this is correct

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
    return static_cast<size_t>(ftell(stream));
}

/**
 * zips for a single thread. Arguments is a pointer to the arg_struct
 * that contains both the arguments needed for unzip.
 * @param buffer string
 * @param size size of buffer
 * @return NULL
 */
static void *zip_thread(const unsigned char *buffer, size_t size,
                        unsigned int pid) {
#ifdef DEBUG
    std::cout<<"buffer is "<<buffer<< "size is " << size <<std::endl;
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
            //for (int j = 0; j < 4; j++) {
            //    array_of_buffers[pid].push_back(static_cast<std::byte>(count >> (j * 8u)));
            //}

            auto *count_ptr = reinterpret_cast<std::byte *>(&count);
            array_of_buffers[pid].insert(std::end(array_of_buffers[pid]), count_ptr, count_ptr + 4);

            array_of_buffers[pid].push_back(static_cast<std::byte>(curr));
            count = 1;
        }
    }
    return nullptr;
}

// iterates through a char* and merges things like 4a5a to become something
// nice like 9a.
static char *merge() {
    auto len = array_of_buffers[0].size();

    std::vector<std::byte> prev_num(&(array_of_buffers[0][len - 5]), &(array_of_buffers[0][len - 2]));
    std::byte prev_char = array_of_buffers[0].at(len - 1);

    std::vector<std::byte> cur_str(&(array_of_buffers[0][0]), &(array_of_buffers[0][len - 1]));

    auto ret = cur_str;

    for (int i = 1; i < NUM_THREADS; ++i) {
        len = array_of_buffers[i].size();
        cur_str = std::vector<std::byte>(&(array_of_buffers[i][0]), &(array_of_buffers[i][len - 1]));

        // check the ending of the string
        std::byte beg_of_str_num = array_of_buffers[i][0];
        std::byte beg_of_str_char = array_of_buffers[i][1];

        // if the things are equal, add the numbers and merge the 2 things
        if (prev_char == beg_of_str_char) {
            /// START OF REALLY BAD COMPILER ERRORS OMG
            // No viable overloaded '='
            prev_num = (*(reinterpret_cast<uint32_t *>(&prev_num)) + *(reinterpret_cast<uint32_t *>(&beg_of_str_num)));
            ret.append(prev_num, 1);
            ret.append(prev_char, 1);
            ret += (cur_str.substr(2, len - 2));
        } else {  // just append something to ret
            ret.append(prev_num, 1);
            ret.append(prev_char, 1);
            ret += (cur_str.substr(0, len - 2));
        }

        prev_num = array_of_buffers[i].substr(len - 2, len - 1);
        prev_char = array_of_buffers[i].substr(len - 1);
    }
    return ret;
    /// END OF REALLY BAD COMPILER ERRORS OMG
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
    auto *buffer = static_cast<unsigned char *>(malloc(size));

    size_t n = 0;
    for (int i = 1; i < argc; ++i) {
        // Read all the files into the buffer
        FILE *stream = open_file(argv[i], "r");
        for (int c = fgetc(stream); c > EOF; c = fgetc(stream)) {
            buffer[n] = static_cast<unsigned char>(c);
            ++n;
        }
        fclose(stream);
    }

    size_t size_of_each_threads_work = size / static_cast<size_t>(NUM_THREADS);

    std::vector<std::thread> threads;

    // plz be a deep copy
    unsigned char *buffer_ptr = buffer;


    // divvy up the work between all the threads
    for (int pid = 0; pid < NUM_THREADS; ++pid) {
        // this keeps track of the size of this buffer
        /// START OF CODE LOOKS KINDA SUS
        size_t this_buffer_size = size_of_each_threads_work;

        // here we check to see if we need to change the size of
        // the threads work. if multiple of the same letter are
        // spread across multiple thread's works, we just take all
        // of the same letter and give it to a thread.
        // if the thread is the first or last, we don't check it


        // simple way to make zip as compressed as possible, check here for
        // first/last thing in threads buffers being the same.
        threads.push_back(std::thread([=]() {
            zip_thread(buffer_ptr, size_of_each_threads_work,
                       static_cast<unsigned int>(pid));
        }));
        /// END OF CODE LOOKS KINDA SUS

        buffer_ptr = buffer_ptr + size_of_each_threads_work;
    }

    // join threads here
    for (int pid = 0; pid < NUM_THREADS; pid++) {
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
