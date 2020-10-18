/**
 * pzip.cc
 * Ariel Flaster & Talib Pierson
 * 17 October 2020
 * A threaded command-line run-length encoding utility in C++
 * The resulting file format is <length> (32-bit unsigned binary)
 * followed by a literal (b-bit character), repeated for as long as necessary.
 */
#include <cstddef>   // for std::byte
#include <cstdio>    // for io
#include <cstdlib>   // atoi and other stuff
#include <iostream>  // also for io
#include <thread>    // for peethreads
#include <vector>    // for something, can't put my finger on it tho

#define NUM_THREADS 1

using buffer_t = std::vector<std::byte>;

static buffer_t thread_buff[NUM_THREADS];  // i think this is correct

// TODO: delete this when we turn this thing in
#include <mutex>

static std::mutex print_lock;

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
static void *zip(const unsigned char *buff, size_t len, size_t thread) {
#ifdef DEBUG
    std::cout << "buff is " << buff << "len is " << len << std::endl;
    std::cout << "buff for " << thread << " is [" << std::flush;
    for (size_t i = 0; i < len; i++) {
        std::cout << static_cast<unsigned char>(buff[i]) << std::flush;
    }
    std::cout << "]" << std::endl;
#endif
    unsigned char last;
    size_t count;
    for (size_t i = 0; i < len;) {
        last = buff[i++];
        for (count = 1; i < len && last == buff[i]; ++i) ++count;
        // std::cout<<"count is " << count << " and char is [" <<
        // curr<<"]"<<std::endl;
        auto *count_p = reinterpret_cast<std::byte *>(&count);
        thread_buff[thread].insert(std::end(thread_buff[thread]), count_p,
                                   count_p + 4);
        thread_buff[thread].push_back(static_cast<std::byte>(last));
    }
#ifdef DEBUG
    std::lock_guard<std::mutex> guard(print_lock);
    for (auto c : thread_buff[thread])
        std::cout << static_cast<unsigned char>(c) << std::flush;
    std::cout << " is what thread " << thread << " just did" << std::endl;
#endif
    return nullptr;
}

/**
 * Iterates through the thread buffers and merges RLEs e.g. 4a5a becomes 9a.
 * @return ret buffer
 */
static buffer_t merge() {
    auto len = thread_buff[0].size();
    // std::cout<<"len is "<<len<<std::endl;

    buffer_t prev_num(&(thread_buff[0][len - 5]), &(thread_buff[0][len - 1]));
    std::byte prev_char = thread_buff[0].at(len - 1);

    buffer_t cur_str =
            buffer_t(&(thread_buff[0][0]), &(thread_buff[0][len - 5]));

    auto ret = buffer_t(&(thread_buff[0][0]), &(thread_buff[0][len - 5]));

#ifdef DEBUG
    std::cout << "ret starts as [" << std::flush;
    for (auto c : ret) std::cout << static_cast<unsigned char>(c) << std::flush;
    std::cout << "]" << std::endl;
#endif

    for (int i = 1; i < NUM_THREADS; ++i) {
        buffer_t beg_of_str_num(&(thread_buff[i][0]), &(thread_buff[i][4]));
        std::byte beg_of_str_char = thread_buff[i][4];

#ifdef DEBUG
        std::cout << "end of the previous buffer is "
                  << *reinterpret_cast<uint32_t *>(&prev_num[0])
                  << static_cast<unsigned char>(prev_char) << std::endl;
        std::cout << "start of the next buffer is "
                  << *reinterpret_cast<uint32_t *>(&beg_of_str_num[0])
                  << static_cast<unsigned char>(beg_of_str_char) << std::endl;
#endif

        // if the things are equal, add the numbers and merge the 2 things
        if (prev_char == beg_of_str_char) {
#ifdef DEBUG
            std::cout << "they're equal!" << std::endl;
#endif

            auto int_prev_num = *reinterpret_cast<uint32_t *>(&prev_num[0]);
            auto int_beg_num =
                    *reinterpret_cast<uint32_t *>(&beg_of_str_num[0]);
            uint32_t new_count = int_beg_num + int_prev_num;
            // convert new number to byte array
            auto *new_count_ptr = reinterpret_cast<std::byte *>(&new_count);

            if (!cur_str.empty()) {
                ret.insert(std::end(ret), new_count_ptr, new_count_ptr + 4);
                ret.push_back(beg_of_str_char);

                // getting things ready for the next iteration of the loop
                len = thread_buff[i].size();
                cur_str =
                        buffer_t(&(thread_buff[i][0]), &(thread_buff[i][len - 5]));

                prev_num = buffer_t(&(thread_buff[i][len - 5]),
                                    &(thread_buff[i][len - 1]));
                prev_char = thread_buff[i].at(len - 1);
                ret.insert(std::end(ret), std::begin(cur_str),
                           std::end(cur_str));
            } else {  // prepping for next iteration of the loop
                len = thread_buff[i].size();
                cur_str =
                        buffer_t(&(thread_buff[i][0]), &(thread_buff[i][len - 5]));

                prev_num = buffer_t(new_count_ptr, new_count_ptr + 4);

#ifdef DEBUG
                std::cout << "adding cur_str to ret" << std::endl;
                std::cout << "cur_str is [" << std::flush;
                for (auto c : cur_str)
                    std::cout << static_cast<unsigned char>(c) << std::flush;
                std::cout << "]" << std::endl;
#endif

                if (i == NUM_THREADS - 1) {
                    ret.insert(std::end(ret), new_count_ptr, new_count_ptr + 4);
                    ret.push_back(beg_of_str_char);
                }
#ifdef DEBUG
                std::cout << "ret is now [" << std::flush;
                for (auto c : ret)
                    std::cout << static_cast<unsigned char>(c) << std::flush;
                std::cout << "]" << std::endl;
#endif
            }
        } else {  // just append something to ret
#ifdef DEBUG
            std::cout << "beg of str size is " << beg_of_str_num.size()
                      << std::endl;
            std::cout << "beg_of_str_num is [";
            for (auto c : beg_of_str_num)
                std::cout << static_cast<unsigned char>(c) << std::flush;
            std::cout << "]" << std::endl;
            std::cout << "beg_of_str_char is ["
                      << static_cast<unsigned char>(beg_of_str_char) << "]"
                      << std::endl;

            std::cout << "they're not equal" << std::endl;
            for (auto c : thread_buff[0])
                std::cout << static_cast<unsigned char>(c) << std::flush;
            std::cout << std::endl;
            for (auto c : thread_buff[1])
                std::cout << static_cast<unsigned char>(c) << std::flush;
            std::cout << std::endl;

#endif

            ret.insert(std::end(ret), std::begin(prev_num), std::end(prev_num));
            ret.push_back(prev_char);
#ifdef DEBUG
            std::cout << "ret is [" << std::flush;
            for (auto c : ret)
                std::cout << static_cast<unsigned char>(c) << std::flush;
            std::cout << "]" << std::endl;
#endif

            // setting up cur_str for the next iteration of the loop
            len = thread_buff[i].size();
            cur_str =
                    buffer_t(&(thread_buff[i][0]), &(thread_buff[i][len - 5]));

            prev_num = buffer_t(&(thread_buff[i][len - 5]),
                                &(thread_buff[i][len - 1]));
            prev_char = thread_buff[i].at(len - 1);

#ifdef DEBUG
            std::cout << "adding cur_str to ret" << std::endl;
            std::cout << "cur_str is [" << std::flush;
            for (auto c : cur_str)
                std::cout << static_cast<unsigned char>(c) << std::flush;
            std::cout << "]" << std::endl;
#endif

            ret.insert(std::end(ret), std::begin(cur_str), std::end(cur_str));

#ifdef DEBUG
            std::cout << "ret is now [" << std::flush;
            for (auto c : ret)
                std::cout << static_cast<unsigned char>(c) << std::flush;
            std::cout << "]" << std::endl;
#endif
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
    auto *buffer = static_cast<unsigned char *>(malloc(buff_size));

    size_t buff_index = 0;
    for (int i = 1; i < argc; ++i) {
        // Read all the files into the buffer
        FILE *stream = open_file(argv[i]);
        for (int c = fgetc(stream); c > EOF; c = fgetc(stream)) {
            buffer[buff_index] = static_cast<unsigned char>(c);
            ++buff_index;
        }
        fclose(stream);
    }

    size_t rle_size = buff_size / static_cast<size_t>(NUM_THREADS);

    std::vector<std::thread> threads;

    // plz be a deep copy
    unsigned char *buffer_ptr = buffer;

    // divvy up the work between all the threads
    for (size_t thread_index = 0; thread_index < NUM_THREADS; ++thread_index) {
        // this keeps track of the size of this buffer

        if (thread_index == NUM_THREADS - 1) {
            threads.emplace_back(std::thread([=]() {
                zip(buffer_ptr, (buff_size - (NUM_THREADS - 1) * rle_size),
                    static_cast<unsigned int>(thread_index));
            }));
        } else {
            threads.emplace_back(std::thread([=]() {
                zip(buffer_ptr, rle_size,
                    static_cast<unsigned int>(thread_index));
            }));

            buffer_ptr = buffer_ptr + rle_size;
        }
    }

    // join threads here
    for (size_t thread_index = 0; thread_index < NUM_THREADS; thread_index++) {
        threads[thread_index].join();
    }

    buffer_t to_print = merge();

    /// hahaHAAHHAHAHAHAHAHAHAHA
    // for (uint32_t  i = 0; i < the_string.size(); i+=5) {
    //    std::cout<< *reinterpret_cast<uint32_t *>(&the_string[i]);
    //    std::cout<< static_cast<unsigned char>(the_string[i+4]);
    //}
    // std::cout<<std::flush;

    // Run-length-encode the buffer to stdout
    for (auto c : to_print) std::cout << static_cast<unsigned char>(c);
    std::cout << std::flush;

    free(buffer);

    return EXIT_SUCCESS;
}
