/**
 * pzip.cc
 * Ariel Flaster & Talib Pierson
 * 17 October 2020
 * Pissconsin zip multi-threaded command-line run-length encoding utility in C++
 * The resulting file format is repeated
 * {length: unsigned 4-byte integer, value: 1-byte character}
 */
#include <cstddef>   // for std::byte
#include <cstdio>    // for io
#include <cstdlib>   // atoi and other stuff
#include <iostream>  // also for io
#include <thread>    // for peethreads
#include <vector>    // for something, can't put my finger on it tho

#define NUM_THREADS 8ul

/// character count
using rle_t = struct rle {
    std::byte c{0};
    uint32_t n{0};
};

/// buffer of run-length encodings
using buff_t = std::vector<rle_t>;

/// array of buffers for each thread
static buff_t buffs[NUM_THREADS];

/**
 * Open a file in read mode and create a new stream for it.
 * @param filename specifies path to file to open
 * @return get_file_stream new stream for file specified
 */
static FILE *open_file(const char *filename) {
    FILE *stream = fopen(filename, "r");

    if (stream == nullptr) {
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
static size_t file_size(FILE *stream) {
    fseek(stream, 0, SEEK_END);
    return static_cast<size_t>(ftell(stream));
}

/**
 * Write run-length encoded buffer to in thread_buffer[index]
 * @param buff character buffer to compress
 * @param len size of data
 * @param thread index of buffer for thread in thread_buffer
 */
static void zip(const std::byte *buff, size_t len, size_t thread) {
    rle_t last;

    for (size_t i = 0; i < len;) {
        last.c = buff[i++];
        for (last.n = 1; i < len && last.c == buff[i]; ++i) ++last.n;
        buffs[thread].push_back(last);
    }
}

/**
 * Iterates through the thread buffs and merges RLEs e.g. 4a5a becomes 9a.
 * @return merged buffer
 */
static buff_t merge() {
    size_t len = buffs[0].size();
    rle_t last = buffs[0][len - 1];
    auto temp = buff_t(&(buffs[0][0]), &(buffs[0][len - 1]));
    auto ret = buff_t(&(buffs[0][0]), &(buffs[0][len - 1]));

    if (NUM_THREADS == 1) {
        return buffs[0];
    }

    for (size_t i = 1; i < NUM_THREADS; ++i) {
        rle_t first = buffs[i][0];

        // if equal, add the numbers and merge
        if (last.c == first.c) {
            uint32_t new_count = last.n + first.n;

            if (!temp.empty()) {
                // append to ret
                ret.push_back({first.c, new_count});

                // for the next iteration of the loop
                len = buffs[i].size();
                last = buffs[i][len - 1];
                temp = buff_t(&(buffs[i][0]), &(buffs[i][len - 1]));
                ret.insert(std::end(ret), std::begin(temp), std::end(temp));
            } else {
                // for next iteration of the loop
                len = buffs[i].size();
                last = {first.c, new_count};
                temp = buff_t(&(buffs[i][0]), &(buffs[i][len - 1]));
                if (i == NUM_THREADS - 1) {
                    ret.push_back({first.c, new_count});
                }
            }
        } else {
            // append to ret
            ret.push_back(last);

            // for the next iteration of the loop
            len = buffs[i].size();
            last = buffs[i][len - 1];
            temp = buff_t(&(buffs[i][0]), &(buffs[i][len - 1]));
            ret.insert(std::end(ret), std::begin(temp), std::end(temp));
        }
    }

    // this is good
    if (!temp.empty()) {
        ret.push_back(last);
    }

    return ret;
}

/**
 * Compresses all input files to stdout.
 * @param argc argument count
 * @param argv array of files to read
 * @return EXIT_SUCCESS iff successful
 */
int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("wzip: file1 [file2 ...]\n");
        exit(EXIT_FAILURE);
    }

    size_t buff_len = 0;
    for (int i = 1; i < argc; ++i) {
        // For each file get size
        FILE *stream = open_file(argv[i]);
        buff_len += file_size(stream);
        fclose(stream);
    }

    // Make a buffer of the sum-size
    auto *buffer = static_cast<std::byte *>(malloc(buff_len));

    size_t buff_index = 0;
    for (int i = 1; i < argc; ++i) {
        // Read all the files into the buffer
        FILE *stream = open_file(argv[i]);
        for (int c = fgetc(stream); c > EOF; c = fgetc(stream)) {
            buffer[buff_index] = static_cast<std::byte>(c);
            ++buff_index;
        }
        fclose(stream);
    }

    size_t rle_len = buff_len / NUM_THREADS;

    std::vector<std::thread> threads;

    std::byte *buff_p = buffer;

    // divvy up the work between all the threads
    for (size_t thread = 0; thread < NUM_THREADS; ++thread) {
        if (thread == NUM_THREADS - 1) {
            threads.emplace_back(std::thread([=]() {
                zip(buff_p, (buff_len - (NUM_THREADS - 1) * rle_len), thread);
            }));
        } else {
            threads.emplace_back(
                    std::thread([=]() { zip(buff_p, rle_len, thread); }));
            buff_p += rle_len;
        }
    }

    // join threads here
    for (size_t thread = 0; thread < NUM_THREADS; thread++) {
        threads[thread].join();
    }

    buff_t merged = merge();

    for (rle_t rle : merged) {
        std::cout.write(reinterpret_cast<const char *>(&rle.n), sizeof(rle.n));
        std::cout.write(reinterpret_cast<const char *>(&rle.c), sizeof(rle.c));
    }
    std::cout << std::flush;

    free(buffer);

    return EXIT_SUCCESS;
}
