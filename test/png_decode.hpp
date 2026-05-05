#pragma once

#include <png.h>

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

inline std::vector<unsigned char> readPNG(const char* name, unsigned long& width, unsigned long& height) {
    std::string path = std::string("test/fixtures/") + name + ".png";

    png_image image{};
    image.version = PNG_IMAGE_VERSION;

    if (!png_image_begin_read_from_file(&image, path.c_str())) {
        std::cerr << "PNG read failed for " << name << ": " << image.message << "\n";
        std::exit(2);
    }
    image.format = PNG_FORMAT_RGBA;

    std::vector<unsigned char> pixels(PNG_IMAGE_SIZE(image));
    if (!png_image_finish_read(&image, nullptr, pixels.data(), 0, nullptr)) {
        std::cerr << "PNG decode failed for " << name << ": " << image.message << "\n";
        std::exit(2);
    }

    width = image.width;
    height = image.height;
    return pixels;
}
