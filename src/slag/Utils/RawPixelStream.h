#ifndef CRUCIBLEEDITOR_RAWPIXELSTREAM_H
#define CRUCIBLEEDITOR_RAWPIXELSTREAM_H

#include <vector>
#include <cassert>

namespace slag
{
    struct RawPixel
    {
        float red, green, blue, alpha;
    };

    class RawPixelStream
    {
    private:
        std::vector<RawPixel> _pixels;
        size_t _width;
        size_t _height;
    public:
        RawPixelStream(std::vector<RawPixel>&& pixels, size_t width, size_t height)
        {
            assert(pixels.size() == width*height && "stream size must match width and height");
            _pixels = std::move(pixels);
            _width = width;
            _height = height;
        }
        RawPixelStream(const RawPixelStream& from)
        {
            _pixels = from._pixels;
            _width = from._width;
            _height = from._height;
        }
        RawPixelStream& operator=(const RawPixelStream& from)
        {
            _pixels = from._pixels;
            _width = from._width;
            _height = from._height;
            return *this;
        }
        RawPixelStream(RawPixelStream&& from)
        {
            _pixels = std::move(from._pixels);
            _width = from._width;
            _height = from._height;
        }
        RawPixelStream& operator=(RawPixelStream&& from)
        {
            _pixels = std::move(from._pixels);
            _width = from._width;
            _height = from._height;
            return *this;
        }
        RawPixel& at(size_t x, size_t y)
        {
            assert(x<= _width && y<= _height && "accessing outside bounds of pixels");
            return _pixels[_width*y+x];
        }
        RawPixel* data()
        {
            return _pixels.data();
        }

        size_t width(){return _width;}
        size_t height(){return _height;}

    };
}
#endif //CRUCIBLEEDITOR_RAWPIXELSTREAM_H
