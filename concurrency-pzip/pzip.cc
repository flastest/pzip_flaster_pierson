/**
 * pzip.cc
 * Ariel Flaster & Talib Pierson
 * 17 October 2020
 * A threaded command-line run-length encoding utility in C++
 * The resulting file format is <length> (32-bit unsigned binary)
 * followed by a literal (b-bit character),
 * repeated for as long as necessary.
 */
#include <cstddef>   // for std::byte
#include <cstdio>    // for io
#include <cstdlib>   // atoi and other stuff
#include <iostream>  // also for io
#include <thread>    // for peethreads
#include <vector>    // for something, can't put my finger on it tho

#define NUM_THREADS 1ul

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
 * @return nullptr
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
 * @return ret buffer
 */
static buff_t merge() {
    size_t len = buffs[0].size();
    buff_t prev_num(&(buffs[0][len - 5]), &(buffs[0][len - 1]));
    std::byte prev_char = buffs[0].at(len - 1);
    buff_t cur_str = buff_t(&(buffs[0][0]), &(buffs[0][len - 5]));

    auto ret = buff_t(&(buffs[0][0]), &(buffs[0][len - 5]));

    for (int i = 1; i < NUM_THREADS; ++i) {
        buff_t beg_of_str_num(&(buffs[i][0]), &(buffs[i][4]));
        std::byte beg_of_str_char = buffs[i][4];

        // if the things are equal, add the numbers and merge the 2 things
        if (prev_char == beg_of_str_char) {
            auto int_prev_num = *reinterpret_cast<uint32_t *>(&prev_num[0]);
            auto int_beg_num = *reinterpret_cast<uint32_t *>(&beg_of_str_num[0]);
            uint32_t new_count = int_beg_num + int_prev_num;
            // convert new number to byte array
            auto *new_count_ptr = reinterpret_cast<std::byte *>(&new_count);

            if (!cur_str.empty()) {
                ret.insert(std::end(ret), new_count_ptr, new_count_ptr + 4);
                ret.push_back(beg_of_str_char);

                // getting things ready for the next iteration of the loop
                len = buffs[i].size();
                cur_str = buff_t(&(buffs[i][0]), &(buffs[i][len - 5]));

                prev_num = buff_t(&(buffs[i][len - 5]), &(buffs[i][len - 1]));
                prev_char = buffs[i].at(len - 1);
                ret.insert(std::end(ret), std::begin(cur_str), std::end(cur_str));
            } else {  // prepping for next iteration of the loop
                len = buffs[i].size();
                cur_str = buff_t(&(buffs[i][0]), &(buffs[i][len - 5]));

                prev_num = buff_t(new_count_ptr, new_count_ptr + 4);

                if (i == NUM_THREADS - 1) {
                    ret.insert(std::end(ret), new_count_ptr, new_count_ptr + 4);
                    ret.push_back(beg_of_str_char);
                }
            }
        } else {  // just append something to ret

            ret.insert(std::end(ret), std::begin(prev_num), std::end(prev_num));
            ret.push_back(prev_char);

            // setting up cur_str for the next iteration of the loop
            len = buffs[i].size();
            cur_str = buff_t(&(buffs[i][0]), &(buffs[i][len - 5]));

            prev_num = buff_t(&(buffs[i][len - 5]), &(buffs[i][len - 1]));
            prev_char = buffs[i].at(len - 1);


            ret.insert(std::end(ret), std::begin(cur_str), std::end(cur_str));

        }
    }
    // this is good
    if (!cur_str.empty()) {
        ret.insert(std::end(ret), std::begin(prev_num), std::end(prev_num));
        ret.push_back(prev_char);
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

    size_t buff_size = 0;
    for (int i = 1; i < argc; ++i) {
        // For each file get size
        FILE *stream = open_file(argv[i]);
        buff_size += file_size(stream);
        fclose(stream);
    }

    // Make a buffer of the sum-size
    auto *buffer = static_cast<std::byte *>(malloc(buff_size));

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

    size_t rle_size = buff_size / NUM_THREADS;

    std::vector<std::thread> threads;

    std::byte *buffer_ptr = buffer;

    // divvy up the work between all the threads
    for (size_t thread = 0; thread < NUM_THREADS; ++thread) {
        if (thread == NUM_THREADS - 1) {
            threads.emplace_back(std::thread([=]() {
                zip(buffer_ptr, (buff_size - (NUM_THREADS - 1) * rle_size), thread);
            }));
        } else {
            threads.emplace_back(std::thread([=]() {
                zip(buffer_ptr, rle_size, thread);
            }));

            buffer_ptr += rle_size;
        }
    }

    // join threads here
    for (size_t thread = 0; thread < NUM_THREADS; thread++) {
        threads[thread].join();
    }

    buff_t merged = merge();

    for (rle_t rle : merged) {
        std::cout.write(reinterpret_cast<const char *>(reinterpret_cast<unsigned char *>(&rle.n)), sizeof(rle.n));
        std::cout.write(reinterpret_cast<const char *>(&rle.c), sizeof(rle.c));
    }
    std::cout << std::flush;

    free(buffer);

    return EXIT_SUCCESS;
}
