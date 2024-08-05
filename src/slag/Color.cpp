#include <stdexcept>
#include <algorithm>
#include <sstream>
#include "Color.h"

namespace slag
{
    Color::Color()
    {
        red = 0;
        green = 0;
        blue = 0;
        alpha = 0;
    }
    Color::Color(float r, float g, float b, float a)
    {
        red = r;
        green = g;
        blue = b;
        alpha = a;
    }

    Color::Color(unsigned char r, unsigned char g, unsigned char b, unsigned char a)
    {
        red = ((float)r)/255.0f;
        green = ((float)g)/255.0f;
        blue = ((float)b)/255.0f;
        alpha = ((float)a)/255.0f;
    }

    Color::Color(std::string hex)
    {
        if(hex[0] == '#')
        {
            hex.erase(0,1);
        }
        if(hex.size()!=6 && hex.size()!=8)
        {
            throw std::runtime_error("color string incorrectly formatted");
        }
        for(int i=0; i< hex.size(); i++)
        {
            auto digit = hex[i];
            if((digit< '0' || digit > '9') && (digit < 'A' || digit > 'F') && (digit < 'a' || digit>'f'))
            {
                throw std::runtime_error("color string incorrectly formatted");
            }
        }
        if(hex.size() == 6)
        {
            unsigned long value = stoul(hex, nullptr,16);
            red = ((value >>16) & 0xff)/255.0f;
            green = ((value >>8) & 0xff)/255.0f;
            blue = ((value >>0) & 0xff)/255.0f;
            alpha = 1;
        }
        else
        {
            unsigned long value = stoul(hex, nullptr,16);
            red = ((value >>24) & 0xff)/255.0f;
            green = ((value >>16) & 0xff)/255.0f;
            blue = ((value >>8) & 0xff)/255.0f;
            alpha = ((value >>0) & 0xff)/255.0f;
        }
    }

    unsigned char Color::redByte()
    {
        return red*255;
    }

    unsigned char Color::greenByte()
    {
        return green*255;
    }

    unsigned char Color::blueByte()
    {
        return blue*255;
    }

    unsigned char Color::alphaByte()
    {
        return alpha*255;
    }

    std::string Color::hexCode()
    {
        std::stringstream ss;
        int r = redByte();
        int g = greenByte();
        int b = blueByte();
        int a = alphaByte();
        ss << "#" <<std::hex<< (r<<24 | g<<16 | b << 8 | a);
        return ss.str();
    }

    ColorArray::ColorArray(Color* colors, size_t x, size_t y)
    {
        _colors.resize(x*y,Color(0.0f,0.0f,0.0f,1.0f));
        memcpy_s(_colors.data(),x*y*sizeof(Color),colors,x*y*sizeof(Color));
        _width = x;
        _height = y;
    }

    ColorArray::ColorArray(std::vector<Color>&& colors, size_t x, size_t y)
    {
        _colors.swap(colors);
        _width = x;
        _height = y;
    }

    ColorArray::ColorArray(Color color, size_t x, size_t y)
    {
        _colors.resize(x*y,color);
        _width = x;
        _width = y;
    }

    ColorArray::ColorArray(const ColorArray& from)
    {
        copy(from);
    }

    ColorArray& ColorArray::operator=(const ColorArray& from)
    {
        copy(from);
        return *this;
    }

    ColorArray::ColorArray(ColorArray&& from)
    {
        move(std::move(from));
    }

    ColorArray& ColorArray::operator=(ColorArray&& from)
    {
        move(std::move(from));
        return *this;
    }

    size_t ColorArray::width()
    {
        return _width;
    }

    size_t ColorArray::height()
    {
        return _height;
    }

    Color& ColorArray::at(size_t x, size_t y)
    {
        if(x < _width && y < _height)
        {
            return _colors[y*_width+x];
        }
        throw std::runtime_error("access of color array is out of bounds");
    }

    void ColorArray::copy(const slag::ColorArray& from)
    {
        _colors = from._colors;
        _width = from._width;
        _height = from._height;
    }

    void ColorArray::move(slag::ColorArray&& from)
    {
        _colors.swap(from._colors);
        _width = from._width;
        _height = from._height;
    }
} // slag