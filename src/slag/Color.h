#ifndef CRUCIBLEEDITOR_COLOR_H
#define CRUCIBLEEDITOR_COLOR_H

#include <string>
#include <vector>

namespace slag
{

    class Color
    {
    public:
        float red,green,blue,alpha;
        Color();
        Color(float r, float g, float b, float a);
        Color(unsigned char r, unsigned char g, unsigned char b, unsigned char a);
        Color(std::string hex);
        unsigned char redByte();
        unsigned char greenByte();
        unsigned char blueByte();
        unsigned char alphaByte();
        std::string hexCode();
    };

    class ColorArray
    {
    public:
        ColorArray(Color* colors, size_t x, size_t y);
        ColorArray(std::vector<Color>&& colors, size_t x, size_t y);
        ColorArray(Color color, size_t x, size_t y);
        ColorArray(const ColorArray& from);
        ColorArray& operator=(const ColorArray& from);
        ColorArray(ColorArray&& from);
        ColorArray& operator=(ColorArray&& from);
        size_t width();
        size_t height();
        Color& at(size_t x, size_t y);
    private:
        size_t _width;
        size_t _height;
        std::vector<Color> _colors;
        void move(ColorArray&& from);
        void copy(const ColorArray& from);
    };

} // slag

#endif //CRUCIBLEEDITOR_COLOR_H
