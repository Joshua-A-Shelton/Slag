#ifndef SLAG_COLOR_H
#define SLAG_COLOR_H

#include <string>
namespace slag
{
    ///Represents a color, stored as 4 floating point components
    class Color
    {
    public:
        ///Used Internally
        enum class ComponentFlags
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
        /**
         * Create color from hex code string
         * @param hex hex string of color to use
         */
        Color(std::string hex);
        ///red represented as byte from 0-255
        unsigned char redByte();
        ///green represented as byte from 0-255
        unsigned char greenByte();
        ///blue represented as byte from 0-255
        unsigned char blueByte();
        ///alpha represented as byte from 0-255
        unsigned char alphaByte();
        ///color hex code representation
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