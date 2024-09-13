#ifndef SLAG_COLOR_H
#define SLAG_COLOR_H

#include <string>
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
        bool operator==(const Color& comp) const
        {
            return red == comp.red && green == comp.green && blue == comp.blue && alpha == comp.alpha;
        }
        bool operator!=(const Color& comp) const
        {
            return red != comp.red || green != comp.green || blue != comp.blue || alpha != comp.alpha;
        }
    };

} // slag

#endif //SLAG_COLOR_H
