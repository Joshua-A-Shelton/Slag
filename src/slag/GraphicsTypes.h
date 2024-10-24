#ifndef SLAG_GRAPHICSTYPES_H
#define SLAG_GRAPHICSTYPES_H
#include <cstdint>
namespace slag
{
    struct GraphicsTypes
    {
        enum GraphicsTypeBits
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

        enum GraphicsType
        {
            UNKNOWN = NONE_BIT,
            BOOLEAN = BOOLEAN_BIT,
            INTEGER = INTEGER_BIT,
            UNSIGNED_INTEGER = UNSIGNED_INTEGER_BIT,
            FLOAT = FLOAT_BIT,
            DOUBLE = DOUBLE_BIT,
            VECTOR2 = VECTOR2_BIT | FLOAT_BIT,
            VECTOR3 = VECTOR3_BIT | FLOAT_BIT,
            VECTOR4 = VECTOR4_BIT | FLOAT_BIT,
            BOOLEAN_VECTOR2 = VECTOR2_BIT | BOOLEAN_BIT,
            BOOLEAN_VECTOR3 = VECTOR3_BIT | BOOLEAN_BIT,
            BOOLEAN_VECTOR4 = VECTOR4_BIT | BOOLEAN_BIT,
            INTEGER_VECTOR2 = VECTOR2_BIT | INTEGER_BIT,
            INTEGER_VECTOR3 = VECTOR3_BIT | INTEGER_BIT,
            INTEGER_VECTOR4 = VECTOR4_BIT | INTEGER_BIT,
            UNSIGNED_INTEGER_VECTOR2 = VECTOR2_BIT | UNSIGNED_INTEGER_BIT,
            UNSIGNED_INTEGER_VECTOR3 = VECTOR3_BIT | UNSIGNED_INTEGER_BIT,
            UNSIGNED_INTEGER_VECTOR4 = VECTOR4_BIT | UNSIGNED_INTEGER_BIT,
            DOUBLE_VECTOR2 = VECTOR2_BIT | DOUBLE_BIT,
            DOUBLE_VECTOR3 = VECTOR3_BIT | DOUBLE_BIT,
            DOUBLE_VECTOR4 = VECTOR4_BIT | DOUBLE_BIT,
            MATRIX_2X2 = MATRIX2N_BIT | VECTOR2_BIT | FLOAT_BIT,
            MATRIX_2X3 = MATRIX2N_BIT | VECTOR3_BIT | FLOAT_BIT,
            MATRIX_2X4 = MATRIX2N_BIT | VECTOR4_BIT | FLOAT_BIT,
            MATRIX_3X2 = MATRIX3N_BIT | VECTOR2_BIT | FLOAT_BIT,
            MATRIX_3X3 = MATRIX3N_BIT | VECTOR3_BIT | FLOAT_BIT,
            MATRIX_3X4 = MATRIX3N_BIT | VECTOR4_BIT | FLOAT_BIT,
            MATRIX_4X2 = MATRIX4N_BIT | VECTOR2_BIT | FLOAT_BIT,
            MATRIX_4X3 = MATRIX4N_BIT | VECTOR3_BIT | FLOAT_BIT,
            MATRIX_4X4 = MATRIX4N_BIT | VECTOR4_BIT | FLOAT_BIT,
            DOUBLE_MATRIX_2X2 = MATRIX2N_BIT | VECTOR2_BIT | DOUBLE_BIT,
            DOUBLE_MATRIX_2X3 = MATRIX2N_BIT | VECTOR3_BIT | DOUBLE_BIT,
            DOUBLE_MATRIX_2X4 = MATRIX2N_BIT | VECTOR4_BIT | DOUBLE_BIT,
            DOUBLE_MATRIX_3X2 = MATRIX3N_BIT | VECTOR2_BIT | DOUBLE_BIT,
            DOUBLE_MATRIX_3X3 = MATRIX3N_BIT | VECTOR3_BIT | DOUBLE_BIT,
            DOUBLE_MATRIX_3X4 = MATRIX3N_BIT | VECTOR4_BIT | DOUBLE_BIT,
            DOUBLE_MATRIX_4X2 = MATRIX4N_BIT | VECTOR2_BIT | DOUBLE_BIT,
            DOUBLE_MATRIX_4X3 = MATRIX4N_BIT | VECTOR3_BIT | DOUBLE_BIT,
            DOUBLE_MATRIX_4X4 = MATRIX4N_BIT | VECTOR4_BIT | DOUBLE_BIT,
            STRUCT = STRUCT_BIT
        };

        static uint32_t typeSize(GraphicsTypes::GraphicsType type)
        {
            if(type & STRUCT)
            {
                throw std::runtime_error("Struct does not have implicit size");
            }
            uint32_t dim1 = 1;
            uint32_t dim2 = 1;
            if (type & (MATRIX4N_BIT))
            {
                dim1 = 4;
            }
            else if (type & MATRIX3N_BIT)
            {
                dim1 = 3;
            }
            else if (type & MATRIX2N_BIT)
            {
                dim1 = 2;
            }
            if (type & (VECTOR4_BIT))
            {
                dim2 = 4;
            }
            else if (type & VECTOR3_BIT)
            {
                dim2 = 3;
            }
            else if (type & VECTOR2_BIT)
            {
                dim2 = 2;
            }
            uint32_t size = 0;
            switch (type & ANY_PRIMITIVE_BIT)
            {
                case BOOLEAN_BIT:
                    size = sizeof(bool);
                    break;
                case UNSIGNED_INTEGER_BIT:
                case INTEGER_BIT:
                    size = sizeof(int);
                    break;
                case FLOAT_BIT:
                    size = sizeof(float);
                    break;
                case DOUBLE_BIT:
                    size = sizeof(double);
                    break;
            }
            return size * dim1 * dim2;
        }

        static uint32_t alignmentSize(GraphicsTypes::GraphicsType type)
        {
            if(type & STRUCT)
            {
                throw std::runtime_error("Struct does not have implicit alignment size");
            }
            uint32_t dim1=1;
            uint32_t dim2=1;
            if(type & (MATRIX4N_BIT | MATRIX3N_BIT))
            {
                dim1=4;
            }
            else if(type & MATRIX2N_BIT)
            {
                dim1=2;
            }
            if(type & (VECTOR4_BIT | VECTOR3_BIT))
            {
                dim2=4;
            }
            else if(type & VECTOR2_BIT)
            {
                dim2=2;
            }
            uint32_t size = 0;
            switch (type & ANY_PRIMITIVE_BIT)
            {
                case BOOLEAN_BIT:
                    size = sizeof(bool);
                    break;
                case UNSIGNED_INTEGER_BIT:
                case INTEGER_BIT:
                    size = sizeof(int);
                    break;
                case FLOAT_BIT:
                    size = sizeof(float);
                    break;
                case DOUBLE_BIT:
                    size = sizeof(double);
                    break;
            }
            return size*dim1*dim2;
        }

    };

} // slag

#endif //SLAG_GRAPHICSTYPES_H
