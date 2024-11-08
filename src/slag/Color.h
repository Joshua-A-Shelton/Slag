#ifndef SLAG_COLOR_H
#define SLAG_COLOR_H

#include <string>
namespace slag
{

    class Color
    {
    public:
        enum ComponentFlags
        {
            RED_COMPONENT = 0b00000001,
            GREEN_COMPONENT = 0b00000010,
            BLUE_COMPONENT = 0b00000100,
            ALPHA_COMPONENT = 0b00001000
        };
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
        bool operator==(const Color& comp) const
        {
            return red == comp.red && green == comp.green && blue == comp.blue && alpha == comp.alpha;
        }
        bool operator!=(const Color& comp) const
        {
            return red != comp.red || green != comp.green || blue != comp.blue || alpha != comp.alpha;
        }
    };

    inline Color::ComponentFlags operator|(Color::ComponentFlags a, Color::ComponentFlags b)
    {
        return static_cast<Color::ComponentFlags>(static_cast<int>(a) | static_cast<int>(b));
    }

    inline Color::ComponentFlags operator|=(Color::ComponentFlags a, Color::ComponentFlags b)
    {
        a = (a|b);
        return a;
    }

    inline Color::ComponentFlags operator&(Color::ComponentFlags a, Color::ComponentFlags b)
    {
        return static_cast<Color::ComponentFlags>(static_cast<int>(a) & static_cast<int>(b));
    }

    inline Color::ComponentFlags operator&=(Color::ComponentFlags a, Color::ComponentFlags b)
    {
        a = a&b;
        return a;
    }

    inline Color::ComponentFlags operator~(Color::ComponentFlags a)
    {
        return static_cast<Color::ComponentFlags>(~static_cast<int>(a));
    }

} // slag

#endif //SLAG_COLOR_H
