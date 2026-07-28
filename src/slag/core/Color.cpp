#include "Color.h"

#include <format>
#include <sstream>

namespace slag
{
    Color::Color(float red, float green, float blue, float alpha)
    {
        this->red = red;
        this->green = green;
        this->blue = blue;
        this->alpha = alpha;
    }

    Color::Color(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha)
    {
        this->red = ((float)red)/255.0f;
        this->green = ((float)green)/255.0f;
        this->blue = ((float)blue)/255.0f;
        this->alpha = ((float)alpha)/255.0f;
    }

    Color::Color(const std::string& hex)
    {
        auto hexString = hex;
        if(hexString[0] == '#')
        {
            hexString.erase(0,1);
        }
        if(hexString.size()!=6 && hexString.size()!=8)
        {
            throw std::format_error("color string incorrectly formatted");
        }
        for(int i=0; i< hexString.size(); i++)
        {
            auto digit = hexString[i];
            if((digit< '0' || digit > '9') && (digit < 'A' || digit > 'F') && (digit < 'a' || digit>'f'))
            {
                throw std::format_error("color string incorrectly formatted");
            }
        }
        if(hexString.size() == 6)
        {
            unsigned long value = stoul(hexString, nullptr,16);
            red = ((value >>16) & 0xff)/255.0f;
            green = ((value >>8) & 0xff)/255.0f;
            blue = ((value >>0) & 0xff)/255.0f;
            alpha = 1;
        }
        else
        {
            unsigned long value = stoul(hexString, nullptr,16);
            red = ((value >>24) & 0xff)/255.0f;
            green = ((value >>16) & 0xff)/255.0f;
            blue = ((value >>8) & 0xff)/255.0f;
            alpha = ((value >>0) & 0xff)/255.0f;
        }
    }

    uint8_t Color::redByte() const
    {
        return red*255;
    }

    uint8_t Color::greenByte() const
    {
        return green*255;
    }

    uint8_t Color::blueByte() const
    {
        return blue*255;
    }

    uint8_t Color::alphaByte() const
    {
        return alpha*255;
    }

    std::string Color::hexCode() const
    {
        std::stringstream ss;
        int r = redByte();
        int g = greenByte();
        int b = blueByte();
        int a = alphaByte();
        ss << "#" <<std::hex<< (r<<24 | g<<16 | b << 8 | a);
        return ss.str();
    }

    bool Color::operator==(const Color& color) const
    {
        return red == color.red && green == color.green && blue == color.blue && alpha == color.alpha;
    }

    bool Color::operator!=(const Color& color) const
    {
        return red != color.red || green != color.green || blue != color.blue || alpha != color.alpha;
    }
} // slag