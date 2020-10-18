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
    // save last rle of first buff
    rle_t last = buffs[0].back();
    // init ret to all of the first buff except its last rle
    buff_t ret(buffs[0].begin(), buffs[0].end() - 1);

    if (NUM_THREADS == 1) {
        return buffs[0];
    }

    for (size_t i = 1; i < NUM_THREADS; ++i) {
        rle_t first = buffs[i].front();

        // if characters equal, add the counts and merge
        if (last.c == first.c) {
            // add the character counts and append new rle to ret
            ret.push_back({first.c, last.n + first.n});

            // append rest of current buff except its first rle which was merged
            // and last rle which might be merged in the next loop
            if (buffs[i].size() > 1) {
                ret.insert(std::end(ret), buffs[i].begin() + 1,
                           buffs[i].end() - 1);
            }
        } else {
            // append to last ret because it wasn't merged
            ret.push_back(last);

            // append rest of current buff except and last rle
            // which might be merged in the next loop
            ret.insert(std::end(ret), buffs[i].begin(), buffs[i].end() - 1);
        }
        // save last for the next iteration of the loop
        last = buffs[i].back();
    }

    // push back last rle because
    // nothing follows it to merge it with
    ret.push_back(last);

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
        // std::cout << rle.n << static_cast<unsigned char>(rle.c);
        std::cout.write(reinterpret_cast<char *>(&rle.n), sizeof(rle.n));
        std::cout.write(reinterpret_cast<char *>(&rle.c), sizeof(rle.c));
    }
    std::cout << std::flush;

    free(buffer);

    return EXIT_SUCCESS;
}
