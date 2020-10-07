/********************
 *
 * A simple run-length encoder in C++
 * The resulting file format is <length> (32-bit unsigned binary) followed
 * by a literal (b-bit character), repeated for as long as necessary.
 *******************/

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using Ccount = struct {  // Character-counter
    char c = '\0';
    uint32_t count = 0;
};

using RLE = std::vector<Ccount>; // A run-length encoding sequence


//////////////////////////////////////////////////////////////////////////////
// zip takes an input string and computes an RLE sequqnce for this string.
// It takes the RLE sequence of any previous string and extends it with the
// new data, so that if the last character of the previous string matches the
// first character of the current sequence, the two sequences are merged 
void zip(const std::string &line, RLE &rle) {
    Ccount last;
    if (line.empty()) {
        return;
    }

    // Special case: we're extending the same character from rle:
    if (!rle.empty() && rle.back().c == line.front()) {
        last = rle.back();
        rle.resize(rle.size() - 1);
    }

    for (size_t i = 0; i < line.size();) {
        last.c = line[i++];
        last.count = 1;
        // Count repeats of last
        while (i < line.size() && line[i] == last.c) {
            last.count++;
            i++;
        }
        rle.push_back(last);
    }
}


//////////////////////////////////////////////////////////////////////////////
// Read all files listed in the argument list after a search term,
// and output matching lines to standard output.
int main(int argc, char **argv) {
    if (argc <= 1) {
        printf("wzip: file1 [file2 ...]\n");
        exit(1);
    }

    RLE rle;
    std::string line;
    std::stringstream strstream;

    for (int i = 1; i < argc; i++) {
        std::ifstream in(argv[i]);       // Will throw if it fails
        strstream << in.rdbuf();    // Read entire file at once
        zip(strstream.str(), rle);
    }

    // Done, output RLE sequence
    for (auto count : rle) {
        std::cout.write(reinterpret_cast<char *>(&count.count), sizeof(count.count));
        std::cout.write(&count.c, sizeof(count.c));
    }

    return 0;
}

