#include <mapbox/pixelmatch.hpp>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#include "picopng.hpp"
#pragma GCC diagnostic pop

#include <vector>
#include <cassert>
#include <iostream>
#include <fstream>
#include <string>
#include <algorithm>
#include <iterator>

std::vector<unsigned char> readFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    assert(file.good());
    return std::vector<unsigned char>(
        std::istreambuf_iterator<char>(file),
        std::istreambuf_iterator<char>());
}

std::vector<unsigned char> readPNG(const char* name, unsigned long& width, unsigned long& height) {
    auto buffer = readFile(std::string("test/fixtures/") + name + ".png");
    std::vector<unsigned char> image;
    int err = decodePNG(image, width, height, buffer.data(), buffer.size());
    assert(err == 0);
    (void)err;
    return image;
}

void diffTest(const char* imgPath1,
              const char* imgPath2,
              const char* diffPath,
              double threshold,
              bool includeAA,
              uint64_t expectedMismatch) {
    (void)expectedMismatch;
    std::cout << "comparing " << imgPath1 << " to " << imgPath2
              << ", threshold: " << threshold << ", includeAA: " << includeAA << std::endl;

    unsigned long w1, h1, w2, h2, wd, hd;
    auto img1 = readPNG(imgPath1, w1, h1);
    auto img2 = readPNG(imgPath2, w2, h2);
    auto expectedDiff = readPNG(diffPath, wd, hd);

    assert(w1 == w2 && h1 == h2 && w1 == wd && h1 == hd);

    std::vector<unsigned char> actualDiff(w1 * h1 * 4);
    uint64_t mismatch = mapbox::pixelmatch(img1.data(), img2.data(), w1, h1,
                                           actualDiff.data(), threshold, includeAA);

    assert(mismatch == expectedMismatch);
    (void)mismatch;
    assert(std::equal(actualDiff.begin(), actualDiff.end(), expectedDiff.begin()));
}

int main() {
    diffTest("1a", "1b", "1diff", 0.05, false, 143);
    diffTest("2a", "2b", "2diff", 0.05, false, 12439);
    diffTest("3a", "3b", "3diff", 0.05, false, 212);
    diffTest("4a", "4b", "4diff", 0.05, false, 36089);
    return 0;
}
