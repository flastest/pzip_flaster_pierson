/**
 * pzip
 * Created by fools on 14 de Octubre.
 * Pissconsin zip is a file compression tool.
 * The compression used is run-length encoding.
 */
#include <cstddef>   // for std::byte
#include <cstdio>    // for io
#include <cstdlib>   // atoi and other stuff
#include <iostream>  // also for io
#include <string>    // for string
#include <thread>    // for peethreads
#include <vector>    // for something, can't put my finger on it tho

#define NUM_THREADS 2

using buffer_t = std::vector<std::byte>;

static buffer_t array_of_buffers[NUM_THREADS];  // i think this is correct

//delete this when we turn this thing in:
#include <mutex>

std::mutex print_lock;


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

    std::cout<<"buffer for "<<pid<<" is ["<<std::flush;
    for (size_t i = 0; i < size; i++) {
        std::cout<<static_cast<unsigned char>(buffer[i])<<std::flush;
    }
    std::cout<<"]"<<std::endl;
#endif

    unsigned char curr;
    unsigned char next;
    size_t count = 1;
    for (size_t i = 0; i < size;) {
        curr = buffer[i];
        next = buffer[++i];
        if (curr == next && i != size) {
            count += 1;
        } else {
            //for (int j = 0; j < 4; j++) {
            //    array_of_buffers[pid].push_back(static_cast<std::byte>(count >> (j * 8u)));
            //}
//            std::cout<<"count is " << count << " and char is [" << curr<<"]"<<std::endl;
            auto *count_ptr = reinterpret_cast<std::byte *>(&count);
            array_of_buffers[pid].insert(std::end(array_of_buffers[pid]), count_ptr, count_ptr + 4);

            array_of_buffers[pid].push_back(static_cast<std::byte>(curr));
            count = 1;
        }
    }
#ifdef DEBUG
    std::lock_guard<std::mutex> guard(print_lock);

    for (auto c : array_of_buffers[pid]) std::cout << static_cast<unsigned char>(c) << std::flush;
    std::cout << " is what thread " << pid << " just did" << std::endl;
#endif
    return nullptr;
}

// iterates through a char* and merges things like 4a5a to become something
// nice like 9a.
static buffer_t merge() {

    auto len = array_of_buffers[0].size();
    //std::cout<<"len is "<<len<<std::endl;

    buffer_t prev_num(&(array_of_buffers[0][len - 5]), &(array_of_buffers[0][len - 1]));
    std::byte prev_char = array_of_buffers[0].at(len - 1);
#ifdef DEBUG
    std::cout << "prev_num is [";
    for (auto c : prev_num) std::cout << static_cast<unsigned char>(c) << std::flush;
    std::cout << "]" << std::endl;
    std::cout<<"prevchar is ["<<static_cast<unsigned char>(prev_char)<<"]"<<std::endl;
#endif

    buffer_t cur_str(&(array_of_buffers[0][0]), &(array_of_buffers[0][len - 5]));

    auto ret = cur_str;
#ifdef DEBUG
    std::cout << "ret starts as [" << std::flush;
    for (auto c : ret) std::cout << static_cast<unsigned char>(c) << std::flush;
    std::cout << "]" << std::endl;
#endif
    for (int i = 1; i < NUM_THREADS; ++i) {

        

        len = array_of_buffers[i].size();
        cur_str = buffer_t(&(array_of_buffers[i][0]), &(array_of_buffers[i][len - 5]));

        

        // check the ending of the string
        buffer_t beg_of_str_num(&(array_of_buffers[i][0]), &(array_of_buffers[i][4]));
        std::byte beg_of_str_char = array_of_buffers[i][4];

        // if the things are equal, add the numbers and merge the 2 things
        if (prev_char == beg_of_str_char) {
#ifdef DEBUG
            std::cout << "they're equal!" << std::endl;
#endif 
            std::byte* bytes_prev_num = reinterpret_cast<std::byte*> (&prev_num[0]);
            std::byte* bytes_beg_num = reinterpret_cast<std::byte*> (&beg_of_str_num[0]);

            uint32_t *num_prev = reinterpret_cast<uint32_t *>(&bytes_prev_num);
            uint32_t *num_beg = reinterpret_cast<uint32_t *>(&bytes_beg_num);
            

//            std::cout<<"num prev is "<<*num_prev<<std::endl;

            uint32_t new_count = *num_prev + *num_beg;

 //           uint32_t new_count = (*(reinterpret_cast<uint32_t*>(&bytes_prev_num)) +
   //                             *(reinterpret_cast<uint32_t*>(&bytes_beg_num)));
            //convert new number to byte array
            auto *new_count_ptr = reinterpret_cast<std::byte *>(&new_count);
#ifdef DEBUG
            std::cout<<"new count is "<<new_count<<std::endl;
            std::cout<<"char is "<<static_cast<unsigned char>(beg_of_str_char)<<std::endl;
#endif
            ret.insert(std::end(ret), new_count_ptr, new_count_ptr + 5);
            ret.push_back(beg_of_str_char);
#ifdef DEBUG          
            std::cout << "ret is [" << std::flush;
            for (auto c : ret) std::cout << static_cast<unsigned char>(c) << std::flush;
            std::cout << "]" << std::endl;
#endif
            if (!cur_str.empty()) ret.insert(std::end(ret), std::begin(cur_str) + 5, std::end(cur_str) - 5);
        

        } else {  // just append something to ret
#ifdef DEBUG
            std::cout << "beg of str size is " << beg_of_str_num.size() << std::endl;
            std::cout << "beg_of_str_num is [";
            for (auto c : beg_of_str_num) std::cout << static_cast<unsigned char>(c) << std::flush;
            std::cout << "]" << std::endl;
            std::cout << "beg_of_str_char is ["<<static_cast<unsigned char>(beg_of_str_char)<<"]"<<std::endl;

            std::cout << "they're not equal" << std::endl;
            for (auto c : array_of_buffers[0]) std::cout << static_cast<unsigned char>(c) << std::flush;
            std::cout << std::endl;
            for (auto c : array_of_buffers[1]) std::cout << static_cast<unsigned char>(c) << std::flush;
            std::cout << std::endl;

#endif

            ret.insert(std::end(ret), std::begin(prev_num), std::end(prev_num));
            ret.push_back(prev_char);
#ifdef DEBUG
            std::cout << "ret is [" << std::flush;
            for (auto c : ret) std::cout << static_cast<unsigned char>(c) << std::flush;
            std::cout << "]" << std::endl;
#endif
            //im 10% sure this is wrong
            if (!cur_str.empty()) {
#ifdef DEBUG
                std::cout<<"adding cur_str to ret"<<std::endl;
                std::cout << "cur_str is [" << std::flush;
                for (auto c : cur_str) std::cout << static_cast<unsigned char>(c) << std::flush;
                std::cout << "]" << std::endl;
#endif
                ret.insert(std::end(ret), std::begin(cur_str), std::end(cur_str));
                //ret.insert(std::end(ret), std::begin(beg_of_str_num), std::end(beg_of_str_num));
                //ret.push_back(beg_of_str_char);
#ifdef DEBUG
                std::cout << "ret is now [" << std::flush;
                for (auto c : ret) std::cout << static_cast<unsigned char>(c) << std::flush;
                std::cout << "]" << std::endl;
#endif
            }
        }

        prev_num = buffer_t(&(array_of_buffers[i][len - 5]), &(array_of_buffers[i][len - 1]));

        prev_char = array_of_buffers[i].at(len - 1);
    }
    //this is good
    ret.insert(std::end(ret), std::begin(prev_num), std::end(prev_num));
    ret.push_back(prev_char);
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

        threads.push_back(std::thread([=]() {
            zip_thread(buffer_ptr, size_of_each_threads_work,
                       static_cast<unsigned int>(pid));
        }));

        buffer_ptr = buffer_ptr + size_of_each_threads_work;
    }

    // join threads here
    for (int pid = 0; pid < NUM_THREADS; pid++) {
        threads[pid].join();
    }
    

    buffer_t the_string = merge();

    /// hahaHAAHHAHAHAHAHAHAHAHA
    for (auto c : the_string) std::cout << static_cast<unsigned char>(c) << std::flush;

    std::cout << std::flush;
    // Run-length-encode the buffer to stdout
    // zip(buffer, size);
    
    free(buffer);

    return EXIT_SUCCESS;
}
