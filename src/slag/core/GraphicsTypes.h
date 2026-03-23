#ifndef SLAG_GRAPHICSTYPES_H
#define SLAG_GRAPHICSTYPES_H
#include <cstdint>
#include "Pixels.h"

namespace slag
{
    enum class GraphicsTypeBits: uint32_t
    {
        NONE                    = 0,
        BOOLEAN_BIT             = 1,
        INTEGER_8_BIT           = 1<<1,
        INTEGER_16_BIT          = 1<<2,
        INTEGER_32_BIT          = 1<<3,
        INTEGER_64_BIT          = 1<<4,
        UNSIGNED_BIT            = 1<<5,
        FLOAT_16_BIT            = 1<<6,
        FLOAT_32_BIT            = 1<<7,
        FLOAT_64_BIT            = 1<<8,
        VECTOR2_BIT             = 1<<9,
        VECTOR3_BIT             = 1<<10,
        VECTOR4_BIT             = 1<<11,
        MATRIX2X_BIT            = 1<<12,
        MATRIX3X_BIT            = 1<<13,
        MATRIX4X_BIT            = 1<<14,
        STRUCT_BIT              = 1<<15,
    };

    inline constexpr GraphicsTypeBits operator|(GraphicsTypeBits lhs, GraphicsTypeBits rhs)
    {
        uint32_t l = static_cast<uint32_t>(lhs);
        uint32_t r = static_cast<uint32_t>(rhs);
        return static_cast<GraphicsTypeBits>(l | r);
    }

    inline constexpr GraphicsTypeBits operator&(GraphicsTypeBits lhs, GraphicsTypeBits rhs)
    {
        uint32_t l = static_cast<uint32_t>(lhs);
        uint32_t r = static_cast<uint32_t>(rhs);
        return static_cast<GraphicsTypeBits>(l & r);
    }

    inline constexpr GraphicsTypeBits operator^(GraphicsTypeBits lhs, GraphicsTypeBits rhs)
    {
        uint32_t l = static_cast<uint32_t>(lhs);
        uint32_t r = static_cast<uint32_t>(rhs);
        return static_cast<GraphicsTypeBits>(l ^ r);
    }

    inline constexpr GraphicsTypeBits operator~(GraphicsTypeBits lhs)
    {
        uint32_t l = static_cast<uint32_t>(lhs);
        return static_cast<GraphicsTypeBits>(~l);
    }

    inline constexpr GraphicsTypeBits operator!(GraphicsTypeBits lhs)
    {
        uint32_t l = static_cast<uint32_t>(lhs);
        return static_cast<GraphicsTypeBits>(!l);
    }

    inline constexpr GraphicsTypeBits operator|=(GraphicsTypeBits& lhs, GraphicsTypeBits rhs)
    {
        lhs = lhs | rhs;
        return lhs;
    }

    inline constexpr GraphicsTypeBits operator&=(GraphicsTypeBits& lhs, GraphicsTypeBits rhs)
    {
        lhs = lhs & rhs;
        return lhs;
    }

    inline constexpr GraphicsTypeBits operator^=(GraphicsTypeBits& lhs, GraphicsTypeBits rhs)
    {
        lhs = lhs ^ rhs;
        return lhs;
    }

    enum class GraphicsType: uint32_t
    {
        UNKNOWN                         = (uint32_t)(GraphicsTypeBits::NONE),
        BOOLEAN                         = (uint32_t)(GraphicsTypeBits::BOOLEAN_BIT),
        INTEGER_8                       = (uint32_t)(GraphicsTypeBits::INTEGER_8_BIT),
        INTEGER_16                      = (uint32_t)(GraphicsTypeBits::INTEGER_16_BIT),
        INTEGER                         = (uint32_t)(GraphicsTypeBits::INTEGER_32_BIT),
        INTEGER_64                      = (uint32_t)(GraphicsTypeBits::INTEGER_64_BIT),
        UNSIGNED_INTEGER_8              = (uint32_t)(GraphicsTypeBits::UNSIGNED_BIT | GraphicsTypeBits::INTEGER_8_BIT),
        UNSIGNED_INTEGER_16             = (uint32_t)(GraphicsTypeBits::UNSIGNED_BIT | GraphicsTypeBits::INTEGER_16_BIT),
        UNSIGNED_INTEGER                = (uint32_t)(GraphicsTypeBits::UNSIGNED_BIT | GraphicsTypeBits::INTEGER_32_BIT),
        UNSIGNED_INTEGER_64             = (uint32_t)(GraphicsTypeBits::UNSIGNED_BIT | GraphicsTypeBits::INTEGER_64_BIT),
        HALF                            = (uint32_t)(GraphicsTypeBits::FLOAT_16_BIT),
        FLOAT                           = (uint32_t)(GraphicsTypeBits::FLOAT_32_BIT),
        DOUBLE                          = (uint32_t)(GraphicsTypeBits::FLOAT_64_BIT),
        BOOLEAN_VECTOR_2                = (uint32_t)(GraphicsTypeBits::BOOLEAN_BIT | GraphicsTypeBits::VECTOR2_BIT),
        BOOLEAN_VECTOR_3                = (uint32_t)(GraphicsTypeBits::BOOLEAN_BIT | GraphicsTypeBits::VECTOR3_BIT),
        BOOLEAN_VECTOR_4                = (uint32_t)(GraphicsTypeBits::BOOLEAN_BIT | GraphicsTypeBits::VECTOR4_BIT),
        INTEGER_8_VECTOR2               = (uint32_t)(GraphicsTypeBits::INTEGER_8_BIT | GraphicsTypeBits::VECTOR2_BIT),
        INTEGER_8_VECTOR3               = (uint32_t)(GraphicsTypeBits::INTEGER_8_BIT | GraphicsTypeBits::VECTOR3_BIT),
        INTEGER_8_VECTOR4               = (uint32_t)(GraphicsTypeBits::INTEGER_8_BIT | GraphicsTypeBits::VECTOR4_BIT),
        INTEGER_16_VECTOR2              = (uint32_t)(GraphicsTypeBits::INTEGER_16_BIT | GraphicsTypeBits::VECTOR2_BIT),
        INTEGER_16_VECTOR3              = (uint32_t)(GraphicsTypeBits::INTEGER_16_BIT | GraphicsTypeBits::VECTOR3_BIT),
        INTEGER_16_VECTOR4              = (uint32_t)(GraphicsTypeBits::INTEGER_16_BIT | GraphicsTypeBits::VECTOR4_BIT),
        INTEGER_VECTOR2                 = (uint32_t)(GraphicsTypeBits::INTEGER_32_BIT | GraphicsTypeBits::VECTOR2_BIT),
        INTEGER_VECTOR3                 = (uint32_t)(GraphicsTypeBits::INTEGER_32_BIT | GraphicsTypeBits::VECTOR3_BIT),
        INTEGER_VECTOR4                 = (uint32_t)(GraphicsTypeBits::INTEGER_32_BIT | GraphicsTypeBits::VECTOR4_BIT),
        INTEGER_64_VECTOR2              = (uint32_t)(GraphicsTypeBits::INTEGER_64_BIT | GraphicsTypeBits::VECTOR2_BIT),
        INTEGER_64_VECTOR3              = (uint32_t)(GraphicsTypeBits::INTEGER_64_BIT | GraphicsTypeBits::VECTOR3_BIT),
        INTEGER_64_VECTOR4              = (uint32_t)(GraphicsTypeBits::INTEGER_64_BIT | GraphicsTypeBits::VECTOR4_BIT),
        UNSIGNED_INTEGER_8_VECTOR2      = (uint32_t)(GraphicsTypeBits::UNSIGNED_BIT | GraphicsTypeBits::INTEGER_8_BIT | GraphicsTypeBits::VECTOR2_BIT),
        UNSIGNED_INTEGER_8_VECTOR3      = (uint32_t)(GraphicsTypeBits::UNSIGNED_BIT | GraphicsTypeBits::INTEGER_8_BIT | GraphicsTypeBits::VECTOR3_BIT),
        UNSIGNED_INTEGER_8_VECTOR4      = (uint32_t)(GraphicsTypeBits::UNSIGNED_BIT | GraphicsTypeBits::INTEGER_8_BIT | GraphicsTypeBits::VECTOR4_BIT),
        UNSIGNED_INTEGER_16_VECTOR2     = (uint32_t)(GraphicsTypeBits::UNSIGNED_BIT | GraphicsTypeBits::INTEGER_16_BIT | GraphicsTypeBits::VECTOR2_BIT),
        UNSIGNED_INTEGER_16_VECTOR3     = (uint32_t)(GraphicsTypeBits::UNSIGNED_BIT | GraphicsTypeBits::INTEGER_16_BIT | GraphicsTypeBits::VECTOR3_BIT),
        UNSIGNED_INTEGER_16_VECTOR4     = (uint32_t)(GraphicsTypeBits::UNSIGNED_BIT | GraphicsTypeBits::INTEGER_16_BIT | GraphicsTypeBits::VECTOR4_BIT),
        UNSIGNED_INTEGER_VECTOR2        = (uint32_t)(GraphicsTypeBits::UNSIGNED_BIT | GraphicsTypeBits::INTEGER_32_BIT | GraphicsTypeBits::VECTOR2_BIT),
        UNSIGNED_INTEGER_VECTOR3        = (uint32_t)(GraphicsTypeBits::UNSIGNED_BIT | GraphicsTypeBits::INTEGER_32_BIT | GraphicsTypeBits::VECTOR3_BIT),
        UNSIGNED_INTEGER_VECTOR4        = (uint32_t)(GraphicsTypeBits::UNSIGNED_BIT | GraphicsTypeBits::INTEGER_32_BIT | GraphicsTypeBits::VECTOR4_BIT),
        UNSIGNED_INTEGER_64_VECTOR2     = (uint32_t)(GraphicsTypeBits::UNSIGNED_BIT | GraphicsTypeBits::INTEGER_64_BIT | GraphicsTypeBits::VECTOR2_BIT),
        UNSIGNED_INTEGER_64_VECTOR3     = (uint32_t)(GraphicsTypeBits::UNSIGNED_BIT | GraphicsTypeBits::INTEGER_64_BIT | GraphicsTypeBits::VECTOR3_BIT),
        UNSIGNED_INTEGER_64_VECTOR4     = (uint32_t)(GraphicsTypeBits::UNSIGNED_BIT | GraphicsTypeBits::INTEGER_64_BIT | GraphicsTypeBits::VECTOR4_BIT),
        HALF_VECTOR2                    = (uint32_t)(GraphicsTypeBits::FLOAT_16_BIT | GraphicsTypeBits::VECTOR2_BIT),
        HALF_VECTOR3                    = (uint32_t)(GraphicsTypeBits::FLOAT_16_BIT | GraphicsTypeBits::VECTOR3_BIT),
        HALF_VECTOR4                    = (uint32_t)(GraphicsTypeBits::FLOAT_16_BIT | GraphicsTypeBits::VECTOR4_BIT),
        VECTOR2                   = (uint32_t)(GraphicsTypeBits::FLOAT_32_BIT | GraphicsTypeBits::VECTOR2_BIT),
        VECTOR3                   = (uint32_t)(GraphicsTypeBits::FLOAT_32_BIT | GraphicsTypeBits::VECTOR3_BIT),
        VECTOR4                   = (uint32_t)(GraphicsTypeBits::FLOAT_32_BIT | GraphicsTypeBits::VECTOR4_BIT),
        DOUBLE_VECTOR2                  = (uint32_t)(GraphicsTypeBits::FLOAT_64_BIT | GraphicsTypeBits::VECTOR2_BIT),
        DOUBLE_VECTOR3                  = (uint32_t)(GraphicsTypeBits::FLOAT_64_BIT | GraphicsTypeBits::VECTOR3_BIT),
        DOUBLE_VECTOR4                  = (uint32_t)(GraphicsTypeBits::FLOAT_64_BIT | GraphicsTypeBits::VECTOR4_BIT),
        INTEGER_8_MATRIX_2X2            = (uint32_t)(GraphicsTypeBits::INTEGER_8_BIT | GraphicsTypeBits::MATRIX2X_BIT | GraphicsTypeBits::VECTOR2_BIT),
        INTEGER_8_MATRIX_2X3            = (uint32_t)(GraphicsTypeBits::INTEGER_8_BIT | GraphicsTypeBits::MATRIX2X_BIT | GraphicsTypeBits::VECTOR3_BIT),
        INTEGER_8_MATRIX_2X4            = (uint32_t)(GraphicsTypeBits::INTEGER_8_BIT | GraphicsTypeBits::MATRIX2X_BIT | GraphicsTypeBits::VECTOR4_BIT),
        INTEGER_8_MATRIX_3X2            = (uint32_t)(GraphicsTypeBits::INTEGER_8_BIT | GraphicsTypeBits::MATRIX3X_BIT | GraphicsTypeBits::VECTOR2_BIT),
        INTEGER_8_MATRIX_3X3            = (uint32_t)(GraphicsTypeBits::INTEGER_8_BIT | GraphicsTypeBits::MATRIX3X_BIT | GraphicsTypeBits::VECTOR3_BIT),
        INTEGER_8_MATRIX_3X4            = (uint32_t)(GraphicsTypeBits::INTEGER_8_BIT | GraphicsTypeBits::MATRIX3X_BIT | GraphicsTypeBits::VECTOR4_BIT),
        INTEGER_8_MATRIX_4X2            = (uint32_t)(GraphicsTypeBits::INTEGER_8_BIT | GraphicsTypeBits::MATRIX4X_BIT | GraphicsTypeBits::VECTOR2_BIT),
        INTEGER_8_MATRIX_4X3            = (uint32_t)(GraphicsTypeBits::INTEGER_8_BIT | GraphicsTypeBits::MATRIX4X_BIT | GraphicsTypeBits::VECTOR3_BIT),
        INTEGER_8_MATRIX_4X4            = (uint32_t)(GraphicsTypeBits::INTEGER_8_BIT | GraphicsTypeBits::MATRIX4X_BIT | GraphicsTypeBits::VECTOR4_BIT),
        INTEGER_16_MATRIX_2X2           = (uint32_t)(GraphicsTypeBits::INTEGER_16_BIT | GraphicsTypeBits::MATRIX2X_BIT | GraphicsTypeBits::VECTOR2_BIT),
        INTEGER_16_MATRIX_2X3           = (uint32_t)(GraphicsTypeBits::INTEGER_16_BIT | GraphicsTypeBits::MATRIX2X_BIT | GraphicsTypeBits::VECTOR3_BIT),
        INTEGER_16_MATRIX_2X4           = (uint32_t)(GraphicsTypeBits::INTEGER_16_BIT | GraphicsTypeBits::MATRIX2X_BIT | GraphicsTypeBits::VECTOR4_BIT),
        INTEGER_16_MATRIX_3X2           = (uint32_t)(GraphicsTypeBits::INTEGER_16_BIT | GraphicsTypeBits::MATRIX3X_BIT | GraphicsTypeBits::VECTOR2_BIT),
        INTEGER_16_MATRIX_3X3           = (uint32_t)(GraphicsTypeBits::INTEGER_16_BIT | GraphicsTypeBits::MATRIX3X_BIT | GraphicsTypeBits::VECTOR3_BIT),
        INTEGER_16_MATRIX_3X4           = (uint32_t)(GraphicsTypeBits::INTEGER_16_BIT | GraphicsTypeBits::MATRIX3X_BIT | GraphicsTypeBits::VECTOR4_BIT),
        INTEGER_16_MATRIX_4X2           = (uint32_t)(GraphicsTypeBits::INTEGER_16_BIT | GraphicsTypeBits::MATRIX4X_BIT | GraphicsTypeBits::VECTOR2_BIT),
        INTEGER_16_MATRIX_4X3           = (uint32_t)(GraphicsTypeBits::INTEGER_16_BIT | GraphicsTypeBits::MATRIX4X_BIT | GraphicsTypeBits::VECTOR3_BIT),
        INTEGER_16_MATRIX_4X4           = (uint32_t)(GraphicsTypeBits::INTEGER_16_BIT | GraphicsTypeBits::MATRIX4X_BIT | GraphicsTypeBits::VECTOR4_BIT),
        INTEGER_MATRIX_2X2              = (uint32_t)(GraphicsTypeBits::INTEGER_32_BIT | GraphicsTypeBits::MATRIX2X_BIT | GraphicsTypeBits::VECTOR2_BIT),
        INTEGER_MATRIX_2X3              = (uint32_t)(GraphicsTypeBits::INTEGER_32_BIT | GraphicsTypeBits::MATRIX2X_BIT | GraphicsTypeBits::VECTOR3_BIT),
        INTEGER_MATRIX_2X4              = (uint32_t)(GraphicsTypeBits::INTEGER_32_BIT | GraphicsTypeBits::MATRIX2X_BIT | GraphicsTypeBits::VECTOR4_BIT),
        INTEGER_MATRIX_3X2              = (uint32_t)(GraphicsTypeBits::INTEGER_32_BIT | GraphicsTypeBits::MATRIX3X_BIT | GraphicsTypeBits::VECTOR2_BIT),
        INTEGER_MATRIX_3X3              = (uint32_t)(GraphicsTypeBits::INTEGER_32_BIT | GraphicsTypeBits::MATRIX3X_BIT | GraphicsTypeBits::VECTOR3_BIT),
        INTEGER_MATRIX_3X4              = (uint32_t)(GraphicsTypeBits::INTEGER_32_BIT | GraphicsTypeBits::MATRIX3X_BIT | GraphicsTypeBits::VECTOR4_BIT),
        INTEGER_MATRIX_4X2              = (uint32_t)(GraphicsTypeBits::INTEGER_32_BIT | GraphicsTypeBits::MATRIX4X_BIT | GraphicsTypeBits::VECTOR2_BIT),
        INTEGER_MATRIX_4X3              = (uint32_t)(GraphicsTypeBits::INTEGER_32_BIT | GraphicsTypeBits::MATRIX4X_BIT | GraphicsTypeBits::VECTOR3_BIT),
        INTEGER_MATRIX_4X4              = (uint32_t)(GraphicsTypeBits::INTEGER_32_BIT | GraphicsTypeBits::MATRIX4X_BIT | GraphicsTypeBits::VECTOR4_BIT),
        INTEGER_64_MATRIX_2X2           = (uint32_t)(GraphicsTypeBits::INTEGER_64_BIT | GraphicsTypeBits::MATRIX2X_BIT | GraphicsTypeBits::VECTOR2_BIT),
        INTEGER_64_MATRIX_2X3           = (uint32_t)(GraphicsTypeBits::INTEGER_64_BIT | GraphicsTypeBits::MATRIX2X_BIT | GraphicsTypeBits::VECTOR3_BIT),
        INTEGER_64_MATRIX_2X4           = (uint32_t)(GraphicsTypeBits::INTEGER_64_BIT | GraphicsTypeBits::MATRIX2X_BIT | GraphicsTypeBits::VECTOR4_BIT),
        INTEGER_64_MATRIX_3X2           = (uint32_t)(GraphicsTypeBits::INTEGER_64_BIT | GraphicsTypeBits::MATRIX3X_BIT | GraphicsTypeBits::VECTOR2_BIT),
        INTEGER_64_MATRIX_3X3           = (uint32_t)(GraphicsTypeBits::INTEGER_64_BIT | GraphicsTypeBits::MATRIX3X_BIT | GraphicsTypeBits::VECTOR3_BIT),
        INTEGER_64_MATRIX_3X4           = (uint32_t)(GraphicsTypeBits::INTEGER_64_BIT | GraphicsTypeBits::MATRIX3X_BIT | GraphicsTypeBits::VECTOR4_BIT),
        INTEGER_64_MATRIX_4X2           = (uint32_t)(GraphicsTypeBits::INTEGER_64_BIT | GraphicsTypeBits::MATRIX4X_BIT | GraphicsTypeBits::VECTOR2_BIT),
        INTEGER_64_MATRIX_4X3           = (uint32_t)(GraphicsTypeBits::INTEGER_64_BIT | GraphicsTypeBits::MATRIX4X_BIT | GraphicsTypeBits::VECTOR3_BIT),
        INTEGER_64_MATRIX_4X4           = (uint32_t)(GraphicsTypeBits::INTEGER_64_BIT | GraphicsTypeBits::MATRIX4X_BIT | GraphicsTypeBits::VECTOR4_BIT),
        UNSIGNED_INTEGER_8_MATRIX_2X2   = (uint32_t)(GraphicsTypeBits::UNSIGNED_BIT | GraphicsTypeBits::INTEGER_8_BIT | GraphicsTypeBits::MATRIX2X_BIT | GraphicsTypeBits::VECTOR2_BIT),
        UNSIGNED_INTEGER_8_MATRIX_2X3   = (uint32_t)(GraphicsTypeBits::UNSIGNED_BIT | GraphicsTypeBits::INTEGER_8_BIT | GraphicsTypeBits::MATRIX2X_BIT | GraphicsTypeBits::VECTOR3_BIT),
        UNSIGNED_INTEGER_8_MATRIX_2X4   = (uint32_t)(GraphicsTypeBits::UNSIGNED_BIT | GraphicsTypeBits::INTEGER_8_BIT | GraphicsTypeBits::MATRIX2X_BIT | GraphicsTypeBits::VECTOR4_BIT),
        UNSIGNED_INTEGER_8_MATRIX_3X2   = (uint32_t)(GraphicsTypeBits::UNSIGNED_BIT | GraphicsTypeBits::INTEGER_8_BIT | GraphicsTypeBits::MATRIX3X_BIT | GraphicsTypeBits::VECTOR2_BIT),
        UNSIGNED_INTEGER_8_MATRIX_3X3   = (uint32_t)(GraphicsTypeBits::UNSIGNED_BIT | GraphicsTypeBits::INTEGER_8_BIT | GraphicsTypeBits::MATRIX3X_BIT | GraphicsTypeBits::VECTOR3_BIT),
        UNSIGNED_INTEGER_8_MATRIX_3X4   = (uint32_t)(GraphicsTypeBits::UNSIGNED_BIT | GraphicsTypeBits::INTEGER_8_BIT | GraphicsTypeBits::MATRIX3X_BIT | GraphicsTypeBits::VECTOR4_BIT),
        UNSIGNED_INTEGER_8_MATRIX_4X2   = (uint32_t)(GraphicsTypeBits::UNSIGNED_BIT | GraphicsTypeBits::INTEGER_8_BIT | GraphicsTypeBits::MATRIX4X_BIT | GraphicsTypeBits::VECTOR2_BIT),
        UNSIGNED_INTEGER_8_MATRIX_4X3   = (uint32_t)(GraphicsTypeBits::UNSIGNED_BIT | GraphicsTypeBits::INTEGER_8_BIT | GraphicsTypeBits::MATRIX4X_BIT | GraphicsTypeBits::VECTOR3_BIT),
        UNSIGNED_INTEGER_8_MATRIX_4X4   = (uint32_t)(GraphicsTypeBits::UNSIGNED_BIT | GraphicsTypeBits::INTEGER_8_BIT | GraphicsTypeBits::MATRIX4X_BIT | GraphicsTypeBits::VECTOR4_BIT),
        UNSIGNED_INTEGER_16_MATRIX_2X2  = (uint32_t)(GraphicsTypeBits::UNSIGNED_BIT | GraphicsTypeBits::INTEGER_16_BIT | GraphicsTypeBits::MATRIX2X_BIT | GraphicsTypeBits::VECTOR2_BIT),
        UNSIGNED_INTEGER_16_MATRIX_2X3  = (uint32_t)(GraphicsTypeBits::UNSIGNED_BIT | GraphicsTypeBits::INTEGER_16_BIT | GraphicsTypeBits::MATRIX2X_BIT | GraphicsTypeBits::VECTOR3_BIT),
        UNSIGNED_INTEGER_16_MATRIX_2X4  = (uint32_t)(GraphicsTypeBits::UNSIGNED_BIT | GraphicsTypeBits::INTEGER_16_BIT | GraphicsTypeBits::MATRIX2X_BIT | GraphicsTypeBits::VECTOR4_BIT),
        UNSIGNED_INTEGER_16_MATRIX_3X2  = (uint32_t)(GraphicsTypeBits::UNSIGNED_BIT | GraphicsTypeBits::INTEGER_16_BIT | GraphicsTypeBits::MATRIX3X_BIT | GraphicsTypeBits::VECTOR2_BIT),
        UNSIGNED_INTEGER_16_MATRIX_3X3  = (uint32_t)(GraphicsTypeBits::UNSIGNED_BIT | GraphicsTypeBits::INTEGER_16_BIT | GraphicsTypeBits::MATRIX3X_BIT | GraphicsTypeBits::VECTOR3_BIT),
        UNSIGNED_INTEGER_16_MATRIX_3X4  = (uint32_t)(GraphicsTypeBits::UNSIGNED_BIT | GraphicsTypeBits::INTEGER_16_BIT | GraphicsTypeBits::MATRIX3X_BIT | GraphicsTypeBits::VECTOR4_BIT),
        UNSIGNED_INTEGER_16_MATRIX_4X2  = (uint32_t)(GraphicsTypeBits::UNSIGNED_BIT | GraphicsTypeBits::INTEGER_16_BIT | GraphicsTypeBits::MATRIX4X_BIT | GraphicsTypeBits::VECTOR2_BIT),
        UNSIGNED_INTEGER_16_MATRIX_4X3  = (uint32_t)(GraphicsTypeBits::UNSIGNED_BIT | GraphicsTypeBits::INTEGER_16_BIT | GraphicsTypeBits::MATRIX4X_BIT | GraphicsTypeBits::VECTOR3_BIT),
        UNSIGNED_INTEGER_16_MATRIX_4X4  = (uint32_t)(GraphicsTypeBits::UNSIGNED_BIT | GraphicsTypeBits::INTEGER_16_BIT | GraphicsTypeBits::MATRIX4X_BIT | GraphicsTypeBits::VECTOR4_BIT),
        UNSIGNED_INTEGER_MATRIX_2X2     = (uint32_t)(GraphicsTypeBits::UNSIGNED_BIT | GraphicsTypeBits::INTEGER_32_BIT | GraphicsTypeBits::MATRIX2X_BIT | GraphicsTypeBits::VECTOR2_BIT),
        UNSIGNED_INTEGER_MATRIX_2X3     = (uint32_t)(GraphicsTypeBits::UNSIGNED_BIT | GraphicsTypeBits::INTEGER_32_BIT | GraphicsTypeBits::MATRIX2X_BIT | GraphicsTypeBits::VECTOR3_BIT),
        UNSIGNED_INTEGER_MATRIX_2X4     = (uint32_t)(GraphicsTypeBits::UNSIGNED_BIT | GraphicsTypeBits::INTEGER_32_BIT | GraphicsTypeBits::MATRIX2X_BIT | GraphicsTypeBits::VECTOR4_BIT),
        UNSIGNED_INTEGER_MATRIX_3X2     = (uint32_t)(GraphicsTypeBits::UNSIGNED_BIT | GraphicsTypeBits::INTEGER_32_BIT | GraphicsTypeBits::MATRIX3X_BIT | GraphicsTypeBits::VECTOR2_BIT),
        UNSIGNED_INTEGER_MATRIX_3X3     = (uint32_t)(GraphicsTypeBits::UNSIGNED_BIT | GraphicsTypeBits::INTEGER_32_BIT | GraphicsTypeBits::MATRIX3X_BIT | GraphicsTypeBits::VECTOR3_BIT),
        UNSIGNED_INTEGER_MATRIX_3X4     = (uint32_t)(GraphicsTypeBits::UNSIGNED_BIT | GraphicsTypeBits::INTEGER_32_BIT | GraphicsTypeBits::MATRIX3X_BIT | GraphicsTypeBits::VECTOR4_BIT),
        UNSIGNED_INTEGER_MATRIX_4X2     = (uint32_t)(GraphicsTypeBits::UNSIGNED_BIT | GraphicsTypeBits::INTEGER_32_BIT | GraphicsTypeBits::MATRIX4X_BIT | GraphicsTypeBits::VECTOR2_BIT),
        UNSIGNED_INTEGER_MATRIX_4X3     = (uint32_t)(GraphicsTypeBits::UNSIGNED_BIT | GraphicsTypeBits::INTEGER_32_BIT | GraphicsTypeBits::MATRIX4X_BIT | GraphicsTypeBits::VECTOR3_BIT),
        UNSIGNED_INTEGER_MATRIX_4X4     = (uint32_t)(GraphicsTypeBits::UNSIGNED_BIT | GraphicsTypeBits::INTEGER_32_BIT | GraphicsTypeBits::MATRIX4X_BIT | GraphicsTypeBits::VECTOR4_BIT),
        UNSIGNED_INTEGER_64_MATRIX_2X2  = (uint32_t)(GraphicsTypeBits::UNSIGNED_BIT | GraphicsTypeBits::INTEGER_64_BIT | GraphicsTypeBits::MATRIX2X_BIT | GraphicsTypeBits::VECTOR2_BIT),
        UNSIGNED_INTEGER_64_MATRIX_2X3  = (uint32_t)(GraphicsTypeBits::UNSIGNED_BIT | GraphicsTypeBits::INTEGER_64_BIT | GraphicsTypeBits::MATRIX2X_BIT | GraphicsTypeBits::VECTOR3_BIT),
        UNSIGNED_INTEGER_64_MATRIX_2X4  = (uint32_t)(GraphicsTypeBits::UNSIGNED_BIT | GraphicsTypeBits::INTEGER_64_BIT | GraphicsTypeBits::MATRIX2X_BIT | GraphicsTypeBits::VECTOR4_BIT),
        UNSIGNED_INTEGER_64_MATRIX_3X2  = (uint32_t)(GraphicsTypeBits::UNSIGNED_BIT | GraphicsTypeBits::INTEGER_64_BIT | GraphicsTypeBits::MATRIX3X_BIT | GraphicsTypeBits::VECTOR2_BIT),
        UNSIGNED_INTEGER_64_MATRIX_3X3  = (uint32_t)(GraphicsTypeBits::UNSIGNED_BIT | GraphicsTypeBits::INTEGER_64_BIT | GraphicsTypeBits::MATRIX3X_BIT | GraphicsTypeBits::VECTOR3_BIT),
        UNSIGNED_INTEGER_64_MATRIX_3X4  = (uint32_t)(GraphicsTypeBits::UNSIGNED_BIT | GraphicsTypeBits::INTEGER_64_BIT | GraphicsTypeBits::MATRIX3X_BIT | GraphicsTypeBits::VECTOR4_BIT),
        UNSIGNED_INTEGER_64_MATRIX_4X2  = (uint32_t)(GraphicsTypeBits::UNSIGNED_BIT | GraphicsTypeBits::INTEGER_64_BIT | GraphicsTypeBits::MATRIX4X_BIT | GraphicsTypeBits::VECTOR2_BIT),
        UNSIGNED_INTEGER_64_MATRIX_4X3  = (uint32_t)(GraphicsTypeBits::UNSIGNED_BIT | GraphicsTypeBits::INTEGER_64_BIT | GraphicsTypeBits::MATRIX4X_BIT | GraphicsTypeBits::VECTOR3_BIT),
        UNSIGNED_INTEGER_64_MATRIX_4X4  = (uint32_t)(GraphicsTypeBits::UNSIGNED_BIT | GraphicsTypeBits::INTEGER_64_BIT | GraphicsTypeBits::MATRIX4X_BIT | GraphicsTypeBits::VECTOR4_BIT),
        HALF_MATRIX_2X2                 = (uint32_t)(GraphicsTypeBits::FLOAT_16_BIT | GraphicsTypeBits::MATRIX2X_BIT | GraphicsTypeBits::VECTOR2_BIT),
        HALF_MATRIX_2X3                 = (uint32_t)(GraphicsTypeBits::FLOAT_16_BIT | GraphicsTypeBits::MATRIX2X_BIT | GraphicsTypeBits::VECTOR3_BIT),
        HALF_MATRIX_2X4                 = (uint32_t)(GraphicsTypeBits::FLOAT_16_BIT | GraphicsTypeBits::MATRIX2X_BIT | GraphicsTypeBits::VECTOR4_BIT),
        HALF_MATRIX_3X2                 = (uint32_t)(GraphicsTypeBits::FLOAT_16_BIT | GraphicsTypeBits::MATRIX3X_BIT | GraphicsTypeBits::VECTOR2_BIT),
        HALF_MATRIX_3X3                 = (uint32_t)(GraphicsTypeBits::FLOAT_16_BIT | GraphicsTypeBits::MATRIX3X_BIT | GraphicsTypeBits::VECTOR3_BIT),
        HALF_MATRIX_3X4                 = (uint32_t)(GraphicsTypeBits::FLOAT_16_BIT | GraphicsTypeBits::MATRIX3X_BIT | GraphicsTypeBits::VECTOR4_BIT),
        HALF_MATRIX_4X2                 = (uint32_t)(GraphicsTypeBits::FLOAT_16_BIT | GraphicsTypeBits::MATRIX4X_BIT | GraphicsTypeBits::VECTOR2_BIT),
        HALF_MATRIX_4X3                 = (uint32_t)(GraphicsTypeBits::FLOAT_16_BIT | GraphicsTypeBits::MATRIX4X_BIT | GraphicsTypeBits::VECTOR3_BIT),
        HALF_MATRIX_4X4                 = (uint32_t)(GraphicsTypeBits::FLOAT_16_BIT | GraphicsTypeBits::MATRIX4X_BIT | GraphicsTypeBits::VECTOR4_BIT),
        MATRIX_2X2                      = (uint32_t)(GraphicsTypeBits::FLOAT_32_BIT | GraphicsTypeBits::MATRIX2X_BIT | GraphicsTypeBits::VECTOR2_BIT),
        MATRIX_2X3                      = (uint32_t)(GraphicsTypeBits::FLOAT_32_BIT | GraphicsTypeBits::MATRIX2X_BIT | GraphicsTypeBits::VECTOR3_BIT),
        MATRIX_2X4                      = (uint32_t)(GraphicsTypeBits::FLOAT_32_BIT | GraphicsTypeBits::MATRIX2X_BIT | GraphicsTypeBits::VECTOR4_BIT),
        MATRIX_3X2                      = (uint32_t)(GraphicsTypeBits::FLOAT_32_BIT | GraphicsTypeBits::MATRIX3X_BIT | GraphicsTypeBits::VECTOR2_BIT),
        MATRIX_3X3                      = (uint32_t)(GraphicsTypeBits::FLOAT_32_BIT | GraphicsTypeBits::MATRIX3X_BIT | GraphicsTypeBits::VECTOR3_BIT),
        MATRIX_3X4                      = (uint32_t)(GraphicsTypeBits::FLOAT_32_BIT | GraphicsTypeBits::MATRIX3X_BIT | GraphicsTypeBits::VECTOR4_BIT),
        MATRIX_4X2                      = (uint32_t)(GraphicsTypeBits::FLOAT_32_BIT | GraphicsTypeBits::MATRIX4X_BIT | GraphicsTypeBits::VECTOR2_BIT),
        MATRIX_4X3                      = (uint32_t)(GraphicsTypeBits::FLOAT_32_BIT | GraphicsTypeBits::MATRIX4X_BIT | GraphicsTypeBits::VECTOR3_BIT),
        MATRIX_4X4                      = (uint32_t)(GraphicsTypeBits::FLOAT_32_BIT | GraphicsTypeBits::MATRIX4X_BIT | GraphicsTypeBits::VECTOR4_BIT),
        DOUBLE_MATRIX_2X2               = (uint32_t)(GraphicsTypeBits::FLOAT_64_BIT | GraphicsTypeBits::MATRIX2X_BIT | GraphicsTypeBits::VECTOR2_BIT),
        DOUBLE_MATRIX_2X3               = (uint32_t)(GraphicsTypeBits::FLOAT_64_BIT | GraphicsTypeBits::MATRIX2X_BIT | GraphicsTypeBits::VECTOR3_BIT),
        DOUBLE_MATRIX_2X4               = (uint32_t)(GraphicsTypeBits::FLOAT_64_BIT | GraphicsTypeBits::MATRIX2X_BIT | GraphicsTypeBits::VECTOR4_BIT),
        DOUBLE_MATRIX_3X2               = (uint32_t)(GraphicsTypeBits::FLOAT_64_BIT | GraphicsTypeBits::MATRIX3X_BIT | GraphicsTypeBits::VECTOR2_BIT),
        DOUBLE_MATRIX_3X3               = (uint32_t)(GraphicsTypeBits::FLOAT_64_BIT | GraphicsTypeBits::MATRIX3X_BIT | GraphicsTypeBits::VECTOR3_BIT),
        DOUBLE_MATRIX_3X4               = (uint32_t)(GraphicsTypeBits::FLOAT_64_BIT | GraphicsTypeBits::MATRIX3X_BIT | GraphicsTypeBits::VECTOR4_BIT),
        DOUBLE_MATRIX_4X2               = (uint32_t)(GraphicsTypeBits::FLOAT_64_BIT | GraphicsTypeBits::MATRIX4X_BIT | GraphicsTypeBits::VECTOR2_BIT),
        DOUBLE_MATRIX_4X3               = (uint32_t)(GraphicsTypeBits::FLOAT_64_BIT | GraphicsTypeBits::MATRIX4X_BIT | GraphicsTypeBits::VECTOR3_BIT),
        DOUBLE_MATRIX_4X4               = (uint32_t)(GraphicsTypeBits::FLOAT_64_BIT | GraphicsTypeBits::MATRIX4X_BIT | GraphicsTypeBits::VECTOR4_BIT),
        STRUCT                          = (uint32_t)(GraphicsTypeBits::STRUCT_BIT)
    };

    inline GraphicsTypeBits operator|(GraphicsType lhs, GraphicsTypeBits rhs)
    {
        uint32_t l = static_cast<uint32_t>(lhs);
        uint32_t r = static_cast<uint32_t>(rhs);
        return static_cast<GraphicsTypeBits>(l | r);
    }

    inline GraphicsTypeBits operator&(GraphicsType lhs, GraphicsTypeBits rhs)
    {
        uint32_t l = static_cast<uint32_t>(lhs);
        uint32_t r = static_cast<uint32_t>(rhs);
        return static_cast<GraphicsTypeBits>(l & r);
    }

    inline GraphicsTypeBits operator^(GraphicsType lhs, GraphicsTypeBits rhs)
    {
        uint32_t l = static_cast<uint32_t>(lhs);
        uint32_t r = static_cast<uint32_t>(rhs);
        return static_cast<GraphicsTypeBits>(l ^ r);
    }

    inline GraphicsTypeBits operator~(GraphicsType lhs)
    {
        uint32_t l = static_cast<uint32_t>(lhs);
        return static_cast<GraphicsTypeBits>(~l);
    }

    inline GraphicsTypeBits operator!(GraphicsType lhs)
    {
        uint32_t l = static_cast<uint32_t>(lhs);
        return static_cast<GraphicsTypeBits>(!l);
    }

}
#endif //SLAG_GRAPHICSTYPES_H