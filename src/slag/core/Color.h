#ifndef SLAG_COLOR_H
#define SLAG_COLOR_H
#include <cstdint>
#include <string>

namespace slag
{
    enum class ColorComponents : uint8_t
    {
        NONE = 0,
        RED = 1,
        GREEN = 1<<1,
        BLUE = 1<<2,
        ALPHA = 1<<3,
    };
    
    inline ColorComponents operator|(ColorComponents a, ColorComponents b)
    {
        return static_cast<ColorComponents>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
    }

    inline ColorComponents operator&(ColorComponents a, ColorComponents b)
    {
        return static_cast<ColorComponents>(static_cast<uint8_t>(a) & static_cast<uint8_t>(b));
    }

    inline ColorComponents operator^(ColorComponents a, ColorComponents b)
    {
        return static_cast<ColorComponents>(static_cast<uint8_t>(a) ^ static_cast<uint8_t>(b));
    }
    
    inline ColorComponents operator~(ColorComponents a)
    {
        return static_cast<ColorComponents>(~static_cast<uint8_t>(a));
    }

    inline ColorComponents operator|=(ColorComponents a, ColorComponents b)
    {
        a = a|b;
        return a;
    }

    inline ColorComponents operator&=(ColorComponents a, ColorComponents b)
    {
        a = a&b;
        return a;
    }

    inline ColorComponents operator^=(ColorComponents a, ColorComponents b)
    {
        a = a^b;
        return a;
    }

    struct Color
    {
    public:
        float red = 0, green = 0, blue = 0, alpha = 1;
        Color()=default;
        Color(float red, float green, float blue, float alpha=1.0f);
        Color(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha=255);
        explicit Color(const std::string& hex);

        [[nodiscard]] uint8_t redByte() const;
        [[nodiscard]] uint8_t greenByte() const;
        [[nodiscard]] uint8_t blueByte() const;
        [[nodiscard]] uint8_t alphaByte() const;
        [[nodiscard]] std::string hexCode() const;
        bool operator==(const Color& color) const;
        bool operator!=(const Color& color) const;

    };
} // slag

#endif //SLAG_COLOR_H