## pixelmatch-cpp

A C++ port of [pixelmatch](https://github.com/mapbox/pixelmatch), the smallest, simplest and fastest JavaScript pixel-level image comparison library.

This is a header-only library; add `include/` to your include path, use it as a CMake subdirectory, or install it and link against `mapbox::pixelmatch`.

## Building & testing

Requires CMake 3.15+ and a C++11 compiler.

```bash
cmake -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

To generate an Xcode project: `cmake -B build -G Xcode && open build/pixelmatch.xcodeproj`.

## API

```cpp
namespace mapbox {

uint64_t pixelmatch(
    const uint8_t* img1,
    const uint8_t* img2,
    std::size_t width,
    std::size_t height,
    uint8_t* output = nullptr,
    double threshold = 0.1,
    bool includeAA = false
);

}
```

`img1` and `img2` must point to buffers of size `width * height * 4`. The return value is the number of mismatched pixels.

Optional arguments:

- `output` - If non-null, must point to an output buffer of the same size, which recieves the diff.
- `threshold` — Matching threshold, ranges from `0` to `1`. Smaller values make the comparison more sensitive.
- `includeAA` — If `true`, disables detecting and ignoring anti-aliased pixels.
