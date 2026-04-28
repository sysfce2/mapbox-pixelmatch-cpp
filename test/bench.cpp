#include <mapbox/pixelmatch.hpp>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#include "picopng.hpp"
#pragma GCC diagnostic pop

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

static std::vector<unsigned char> readPNG(const char* name, unsigned long& w, unsigned long& h) {
    std::ifstream f(std::string("test/fixtures/") + name + ".png", std::ios::binary);
    if (!f.good()) { std::cerr << "cannot open " << name << "\n"; std::exit(2); }
    std::vector<unsigned char> buf{std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>()};
    std::vector<unsigned char> img;
    if (decodePNG(img, w, h, buf.data(), buf.size()) != 0) { std::cerr << "decode failed\n"; std::exit(2); }
    return img;
}

struct Case { const char* a; const char* b; double threshold; };

int main() {
    static const Case cases[] = {
        {"1a", "1b", 0.05}, {"2a", "2b", 0.05}, {"3a", "3b", 0.05}, {"4a", "4b", 0.05},
        {"5a", "5b", 0.05}, {"6a", "6b", 0.05}, {"7a", "7b", 0.1},
    };

    using clock = std::chrono::steady_clock;
    double totalMs = 0;
    std::cout << std::fixed << std::setprecision(3);

    for (const auto& c : cases) {
        unsigned long w, h;
        auto img1 = readPNG(c.a, w, h);
        auto img2 = readPNG(c.b, w, h);
        std::vector<unsigned char> output(w * h * 4);

        // calibrate iterations so each case takes ~100ms
        int iters = 1;
        for (;;) {
            auto t0 = clock::now();
            for (int i = 0; i < iters; i++) {
                mapbox::pixelmatch(img1.data(), img2.data(), w, h, output.data(), c.threshold);
            }
            double ms = std::chrono::duration<double, std::milli>(clock::now() - t0).count();
            if (ms >= 100.0 || iters >= 1 << 20) {
                double per = ms / iters;
                totalMs += per;
                std::cout << c.a << " vs " << c.b << " (" << w << "x" << h << "): "
                          << per << " ms/run (" << iters << " iters)\n";
                break;
            }
            iters *= 2;
        }
    }
    std::cout << "total: " << totalMs << " ms/round\n";
    return 0;
}
