#ifndef SLAG_GRAPHICSTYPES_H
#define SLAG_GRAPHICSTYPES_H

#include <cstdint>
#include <stdexcept>
namespace slag
{
    enum class GraphicsTypeBits: uint32_t
    {
        NONE_BIT                = 0b0000000000000000,
        BOOLEAN_BIT             = 0b0000000000000001,
        INTEGER_BIT             = 0b0000000000000010,
        UNSIGNED_INTEGER_BIT    = 0b0000000000000100,
        FLOAT_BIT               = 0b0000000000001000,
        DOUBLE_BIT              = 0b0000000000010000,
        ANY_PRIMITIVE_BIT       = 0b0000000000011111,
        VECTOR2_BIT             = 0b0000000100000000,
        VECTOR3_BIT             = 0b0000001000000000,
        VECTOR4_BIT             = 0b0000010000000000,
        ANY_VECTOR_BIT          = 0b0000011100000000,
        MATRIX2N_BIT            = 0b0000100000000000,
        MATRIX3N_BIT            = 0b0001000000000000,
        MATRIX4N_BIT            = 0b0010000000000000,
        ANY_MATRIX_BIT          = 0b0011100000000000,
        STRUCT_BIT              = 0b0100000000000000
    };

    inline GraphicsTypeBits operator|(GraphicsTypeBits a, GraphicsTypeBits b)
    {
        return static_cast<GraphicsTypeBits>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
    }

    inline GraphicsTypeBits operator&(GraphicsTypeBits a, GraphicsTypeBits b)
    {
        return static_cast<GraphicsTypeBits>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
    }

    inline GraphicsTypeBits operator~(GraphicsTypeBits a)
    {
        return static_cast<GraphicsTypeBits>(~static_cast<uint32_t>(a));
    }

    inline GraphicsTypeBits operator|=(GraphicsTypeBits& a, GraphicsTypeBits b)
    {
        a = a | b;
        return a;
    }

    inline GraphicsTypeBits operator&=(GraphicsTypeBits& a, GraphicsTypeBits b)
    {
        a = a & b;
        return a;
    }



    enum class GraphicsType: uint32_t
    {
        UNKNOWN = GraphicsTypeBits::NONE_BIT,
        BOOLEAN = GraphicsTypeBits::BOOLEAN_BIT,
        INTEGER = GraphicsTypeBits::INTEGER_BIT,
        UNSIGNED_INTEGER = GraphicsTypeBits::UNSIGNED_INTEGER_BIT,
        FLOAT = GraphicsTypeBits::FLOAT_BIT,
        DOUBLE = GraphicsTypeBits::DOUBLE_BIT,
        VECTOR2 = GraphicsTypeBits::VECTOR2_BIT | GraphicsTypeBits::FLOAT_BIT,
        VECTOR3 = GraphicsTypeBits::VECTOR3_BIT | GraphicsTypeBits::FLOAT_BIT,
        VECTOR4 = GraphicsTypeBits::VECTOR4_BIT | GraphicsTypeBits::FLOAT_BIT,
        BOOLEAN_VECTOR2 = GraphicsTypeBits::VECTOR2_BIT | GraphicsTypeBits::BOOLEAN_BIT,
        BOOLEAN_VECTOR3 = GraphicsTypeBits::VECTOR3_BIT | GraphicsTypeBits::BOOLEAN_BIT,
        BOOLEAN_VECTOR4 = GraphicsTypeBits::VECTOR4_BIT | GraphicsTypeBits::BOOLEAN_BIT,
        INTEGER_VECTOR2 = GraphicsTypeBits::VECTOR2_BIT | GraphicsTypeBits::INTEGER_BIT,
        INTEGER_VECTOR3 = GraphicsTypeBits::VECTOR3_BIT | GraphicsTypeBits::INTEGER_BIT,
        INTEGER_VECTOR4 = GraphicsTypeBits::VECTOR4_BIT | GraphicsTypeBits::INTEGER_BIT,
        UNSIGNED_INTEGER_VECTOR2 = GraphicsTypeBits::VECTOR2_BIT | GraphicsTypeBits::UNSIGNED_INTEGER_BIT,
        UNSIGNED_INTEGER_VECTOR3 = GraphicsTypeBits::VECTOR3_BIT | GraphicsTypeBits::UNSIGNED_INTEGER_BIT,
        UNSIGNED_INTEGER_VECTOR4 = GraphicsTypeBits::VECTOR4_BIT | GraphicsTypeBits::UNSIGNED_INTEGER_BIT,
        DOUBLE_VECTOR2 = GraphicsTypeBits::VECTOR2_BIT | GraphicsTypeBits::DOUBLE_BIT,
        DOUBLE_VECTOR3 = GraphicsTypeBits::VECTOR3_BIT | GraphicsTypeBits::DOUBLE_BIT,
        DOUBLE_VECTOR4 = GraphicsTypeBits::VECTOR4_BIT | GraphicsTypeBits::DOUBLE_BIT,
        MATRIX_2X2 = GraphicsTypeBits::MATRIX2N_BIT | GraphicsTypeBits::VECTOR2_BIT | GraphicsTypeBits::FLOAT_BIT,
        MATRIX_2X3 = GraphicsTypeBits::MATRIX2N_BIT | GraphicsTypeBits::VECTOR3_BIT | GraphicsTypeBits::FLOAT_BIT,
        MATRIX_2X4 = GraphicsTypeBits::MATRIX2N_BIT | GraphicsTypeBits::VECTOR4_BIT | GraphicsTypeBits::FLOAT_BIT,
        MATRIX_3X2 = GraphicsTypeBits::MATRIX3N_BIT | GraphicsTypeBits::VECTOR2_BIT | GraphicsTypeBits::FLOAT_BIT,
        MATRIX_3X3 = GraphicsTypeBits::MATRIX3N_BIT | GraphicsTypeBits::VECTOR3_BIT | GraphicsTypeBits::FLOAT_BIT,
        MATRIX_3X4 = GraphicsTypeBits::MATRIX3N_BIT | GraphicsTypeBits::VECTOR4_BIT | GraphicsTypeBits::FLOAT_BIT,
        MATRIX_4X2 = GraphicsTypeBits::MATRIX4N_BIT | GraphicsTypeBits::VECTOR2_BIT | GraphicsTypeBits::FLOAT_BIT,
        MATRIX_4X3 = GraphicsTypeBits::MATRIX4N_BIT | GraphicsTypeBits::VECTOR3_BIT | GraphicsTypeBits::FLOAT_BIT,
        MATRIX_4X4 = GraphicsTypeBits::MATRIX4N_BIT | GraphicsTypeBits::VECTOR4_BIT | GraphicsTypeBits::FLOAT_BIT,
        DOUBLE_MATRIX_2X2 = GraphicsTypeBits::MATRIX2N_BIT | GraphicsTypeBits::VECTOR2_BIT | GraphicsTypeBits::DOUBLE_BIT,
        DOUBLE_MATRIX_2X3 = GraphicsTypeBits::MATRIX2N_BIT | GraphicsTypeBits::VECTOR3_BIT | GraphicsTypeBits::DOUBLE_BIT,
        DOUBLE_MATRIX_2X4 = GraphicsTypeBits::MATRIX2N_BIT | GraphicsTypeBits::VECTOR4_BIT | GraphicsTypeBits::DOUBLE_BIT,
        DOUBLE_MATRIX_3X2 = GraphicsTypeBits::MATRIX3N_BIT | GraphicsTypeBits::VECTOR2_BIT | GraphicsTypeBits::DOUBLE_BIT,
        DOUBLE_MATRIX_3X3 = GraphicsTypeBits::MATRIX3N_BIT | GraphicsTypeBits::VECTOR3_BIT | GraphicsTypeBits::DOUBLE_BIT,
        DOUBLE_MATRIX_3X4 = GraphicsTypeBits::MATRIX3N_BIT | GraphicsTypeBits::VECTOR4_BIT | GraphicsTypeBits::DOUBLE_BIT,
        DOUBLE_MATRIX_4X2 = GraphicsTypeBits::MATRIX4N_BIT | GraphicsTypeBits::VECTOR2_BIT | GraphicsTypeBits::DOUBLE_BIT,
        DOUBLE_MATRIX_4X3 = GraphicsTypeBits::MATRIX4N_BIT | GraphicsTypeBits::VECTOR3_BIT | GraphicsTypeBits::DOUBLE_BIT,
        DOUBLE_MATRIX_4X4 = GraphicsTypeBits::MATRIX4N_BIT | GraphicsTypeBits::VECTOR4_BIT | GraphicsTypeBits::DOUBLE_BIT,
        STRUCT = GraphicsTypeBits::STRUCT_BIT
    };

    inline GraphicsTypeBits operator|(GraphicsType a, GraphicsTypeBits b)
    {
        return static_cast<GraphicsTypeBits>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
    }

    inline GraphicsTypeBits operator&(GraphicsType a, GraphicsTypeBits b)
    {
        return static_cast<GraphicsTypeBits>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
    }

    inline GraphicsTypeBits operator~(GraphicsType a)
    {
        return static_cast<GraphicsTypeBits>(~static_cast<uint32_t>(a));
    }

    static uint32_t graphicsTypeSize(GraphicsType type)
    {
        if(static_cast<bool>(type & GraphicsTypeBits::STRUCT_BIT))
        {
            throw std::runtime_error("Struct does not have implicit size");
        }
        uint32_t dim1 = 1;
        uint32_t dim2 = 1;
        if ((uint32_t)type & (uint32_t)(GraphicsTypeBits::MATRIX4N_BIT))
        {
            dim1 = 4;
        }
        else if ((uint32_t)type & (uint32_t)GraphicsTypeBits::MATRIX3N_BIT)
        {
            dim1 = 3;
        }
        else if ((uint32_t)type & (uint32_t)GraphicsTypeBits::MATRIX2N_BIT)
        {
            dim1 = 2;
        }
        if ((uint32_t)type & (uint32_t)GraphicsTypeBits::VECTOR4_BIT)
        {
            dim2 = 4;
        }
        else if ((uint32_t)type & (uint32_t)GraphicsTypeBits::VECTOR3_BIT)
        {
            dim2 = 3;
        }
        else if ((uint32_t)type & (uint32_t)GraphicsTypeBits::VECTOR2_BIT)
        {
            dim2 = 2;
        }
        uint32_t size = 0;
        switch ((uint32_t)type & (uint32_t)GraphicsTypeBits::ANY_PRIMITIVE_BIT)
        {
            case GraphicsTypeBits::BOOLEAN_BIT:
                size = sizeof(bool);
                break;
            case GraphicsTypeBits::UNSIGNED_INTEGER_BIT:
            case GraphicsTypeBits::INTEGER_BIT:
                size = sizeof(int);
                break;
            case GraphicsTypeBits::FLOAT_BIT:
                size = sizeof(float);
                break;
            case GraphicsTypeBits::DOUBLE_BIT:
                size = sizeof(double);
                break;
        }
        return size * dim1 * dim2;
    }

    uint32_t graphicsTypeAlignmentSize(GraphicsType type)
    {
        if(static_cast<bool>(type & GraphicsTypeBits::STRUCT_BIT))
        {
            throw std::runtime_error("Struct does not have implicit alignment size");
        }
        uint32_t dim1=1;
        uint32_t dim2=1;
        if(static_cast<bool>(type & (GraphicsTypeBits::MATRIX4N_BIT | GraphicsTypeBits::MATRIX3N_BIT)))
        {
            dim1=4;
        }
        else if((uint32_t)type & (uint32_t)GraphicsTypeBits::MATRIX2N_BIT)
        {
            dim1=2;
        }
        if((uint32_t)type & (uint32_t)(GraphicsTypeBits::VECTOR4_BIT | GraphicsTypeBits::VECTOR3_BIT))
        {
            dim2=4;
        }
        else if((uint32_t)type & (uint32_t)GraphicsTypeBits::VECTOR2_BIT)
        {
            dim2=2;
        }
        uint32_t size = 0;
        switch ((uint32_t)type & (uint32_t)GraphicsTypeBits::ANY_PRIMITIVE_BIT)
        {
            case GraphicsTypeBits::BOOLEAN_BIT:
                size = sizeof(bool);
                break;
            case GraphicsTypeBits::UNSIGNED_INTEGER_BIT:
            case GraphicsTypeBits::INTEGER_BIT:
                size = sizeof(int);
                break;
            case GraphicsTypeBits::FLOAT_BIT:
                size = sizeof(float);
                break;
            case GraphicsTypeBits::DOUBLE_BIT:
                size = sizeof(double);
                break;
        }
        return size*dim1*dim2;
    }

} // slag


#endif //SLAG_GRAPHICSTYPES_H