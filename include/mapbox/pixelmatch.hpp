#ifndef PIXELMATCH_HPP
#define PIXELMATCH_HPP

#include <cstdint>
#include <cstddef>
#include <cstring>
#include <algorithm>

namespace mapbox {

namespace detail {

inline double rgb2y(double r, double g, double b) { return r * 0.29889531 + g * 0.58662247 + b * 0.11448223; }
inline double rgb2i(double r, double g, double b) { return r * 0.59597799 - g * 0.27417610 - b * 0.32180189; }
inline double rgb2q(double r, double g, double b) { return r * 0.21147017 - g * 0.52261711 + b * 0.31114694; }

// Squared YIQ distance between two pixels; semi-transparent pixels are blended against white.
// Based on "Measuring perceived color difference using YIQ NTSC transmission color space
// in mobile applications" by Y. Kotsarenko and F. Ramos.
inline double colorDelta(const uint8_t* img1, const uint8_t* img2, std::size_t k, std::size_t m, bool yOnly = false) {
    int r1 = img1[k],     g1 = img1[k + 1], b1 = img1[k + 2], a1 = img1[k + 3];
    int r2 = img2[m],     g2 = img2[m + 1], b2 = img2[m + 2], a2 = img2[m + 3];

    int dri = r1 - r2, dgi = g1 - g2, dbi = b1 - b2, da = a1 - a2;

    if (yOnly && !dri && !dgi && !dbi && !da) return 0;

    double dr = dri, dg = dgi, db = dbi;
    if (a1 < 255 || a2 < 255) { // blend with white background
        dr = (r1 * a1 - r2 * a2 - 255 * da) / 255.0;
        dg = (g1 * a1 - g2 * a2 - 255 * da) / 255.0;
        db = (b1 * a1 - b2 * a2 - 255 * da) / 255.0;
    }

    double y = rgb2y(dr, dg, db);
    if (yOnly) return y;
    double i = rgb2i(dr, dg, db);
    double q = rgb2q(dr, dg, db);
    return 0.5053 * y * y + 0.299 * i * i + 0.1957 * q * q;
}

inline void drawPixel(uint8_t* output, std::size_t pos, uint8_t r, uint8_t g, uint8_t b) {
    output[pos]     = r;
    output[pos + 1] = g;
    output[pos + 2] = b;
    output[pos + 3] = 255;
}

inline void drawGrayPixel(const uint8_t* img, std::size_t i, uint8_t* output, std::size_t outPos) {
    int r = img[i], g = img[i + 1], b = img[i + 2], a = img[i + 3];
    double y = rgb2y(r, g, b);
    uint8_t v = static_cast<uint8_t>(255 + (y - 255) * 0.1 * a / 255);
    drawPixel(output, outPos, v, v, v);
}

// Check if a pixel has 3+ adjacent pixels of the same RGBA value.
inline bool hasManySiblings(const uint8_t* img, std::size_t stride, std::size_t x1, std::size_t y1,
                            std::size_t width, std::size_t height) {
    std::size_t x0 = x1 > 0 ? x1 - 1 : 0;
    std::size_t y0 = y1 > 0 ? y1 - 1 : 0;
    std::size_t x2 = std::min(x1 + 1, width - 1);
    std::size_t y2 = std::min(y1 + 1, height - 1);
    std::size_t pos = y1 * stride + x1 * 4;
    int zeroes = (x1 == x0 || x1 == x2 || y1 == y0 || y1 == y2) ? 1 : 0;

    for (std::size_t x = x0; x <= x2; x++) {
        for (std::size_t y = y0; y <= y2; y++) {
            if (x == x1 && y == y1) continue;
            if (std::memcmp(img + pos, img + y * stride + x * 4, 4) == 0) {
                if (++zeroes > 2) return true;
            }
        }
    }
    return false;
}

// Check if a pixel is likely a part of anti-aliasing.
// Based on "Anti-aliased Pixel and Intensity Slope Detector" paper by V. Vysniauskas, 2009.
inline bool antialiased(const uint8_t* img, std::size_t stride,
                        std::size_t x1, std::size_t y1, std::size_t width, std::size_t height,
                        const uint8_t* img1, std::size_t stride1,
                        const uint8_t* img2, std::size_t stride2) {
    std::size_t x0 = x1 > 0 ? x1 - 1 : 0;
    std::size_t y0 = y1 > 0 ? y1 - 1 : 0;
    std::size_t x2 = std::min(x1 + 1, width - 1);
    std::size_t y2 = std::min(y1 + 1, height - 1);
    std::size_t pos = y1 * stride + x1 * 4;
    int zeroes = (x1 == x0 || x1 == x2 || y1 == y0 || y1 == y2) ? 1 : 0;
    double minD = 0, maxD = 0;
    std::size_t minX = 0, minY = 0, maxX = 0, maxY = 0;

    for (std::size_t x = x0; x <= x2; x++) {
        for (std::size_t y = y0; y <= y2; y++) {
            if (x == x1 && y == y1) continue;
            double delta = colorDelta(img, img, pos, y * stride + x * 4, true);
            if (delta == 0) {
                if (++zeroes > 2) return false;
            } else if (delta < minD) {
                minD = delta; minX = x; minY = y;
            } else if (delta > maxD) {
                maxD = delta; maxX = x; maxY = y;
            }
        }
    }

    // if there are no both darker and brighter pixels among siblings, it's not anti-aliasing
    if (minD == 0 || maxD == 0) return false;

    // if either the darkest or the brightest pixel has 3+ equal siblings in both images
    // (definitely not anti-aliased), this pixel is anti-aliased
    return (hasManySiblings(img1, stride1, minX, minY, width, height) &&
            hasManySiblings(img2, stride2, minX, minY, width, height)) ||
           (hasManySiblings(img1, stride1, maxX, maxY, width, height) &&
            hasManySiblings(img2, stride2, maxX, maxY, width, height));
}

}

inline uint64_t pixelmatch(const uint8_t* img1,
                           std::size_t stride1,
                           const uint8_t* img2,
                           std::size_t stride2,
                           std::size_t width,
                           std::size_t height,
                           uint8_t* output = nullptr,
                           double threshold = 0.1,
                           bool includeAA = false)
{
    using namespace detail;

    // fast path for identical images
    bool identical = true;
    for (std::size_t y = 0; y < height; y++) {
        if (std::memcmp(img1 + y * stride1, img2 + y * stride2, width * 4) != 0) {
            identical = false;
            break;
        }
    }
    if (identical) {
        if (output) {
            for (std::size_t y = 0; y < height; y++) {
                for (std::size_t x = 0; x < width; x++) {
                    drawGrayPixel(img1, y * stride1 + x * 4, output, (y * width + x) * 4);
                }
            }
        }
        return 0;
    }

    // 35215 is the maximum possible value for the YIQ difference metric
    double maxDelta = 35215 * threshold * threshold;
    uint64_t diff = 0;

    for (std::size_t y = 0; y < height; y++) {
        for (std::size_t x = 0; x < width; x++) {
            std::size_t pos1 = y * stride1 + x * 4;
            std::size_t pos2 = y * stride2 + x * 4;
            std::size_t posOut = (y * width + x) * 4;

            double delta = std::memcmp(img1 + pos1, img2 + pos2, 4) == 0
                ? 0 : colorDelta(img1, img2, pos1, pos2);

            if (delta > maxDelta) {
                if (!includeAA &&
                    (antialiased(img1, stride1, x, y, width, height, img1, stride1, img2, stride2) ||
                     antialiased(img2, stride2, x, y, width, height, img1, stride1, img2, stride2))) {
                    if (output) drawPixel(output, posOut, 255, 255, 0);
                } else {
                    if (output) drawPixel(output, posOut, 255, 0, 0);
                    diff++;
                }
            } else if (output) {
                drawGrayPixel(img1, pos1, output, posOut);
            }
        }
    }

    return diff;
}

inline uint64_t pixelmatch(const uint8_t* img1,
                           const uint8_t* img2,
                           std::size_t width,
                           std::size_t height,
                           uint8_t* output = nullptr,
                           double threshold = 0.1,
                           bool includeAA = false)
{
    return pixelmatch(img1, width * 4, img2, width * 4, width, height, output, threshold, includeAA);
}

}

#endif
