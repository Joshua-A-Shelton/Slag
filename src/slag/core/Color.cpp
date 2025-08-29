#include <ios>
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
} // slag