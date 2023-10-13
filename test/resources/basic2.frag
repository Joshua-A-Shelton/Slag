#version 450

layout (location = 0) in vec2 texCoord;

layout (location = 0) out vec4 outFragColor;

layout(set = 1, binding = 0) uniform details
{
    vec4 color1;
} Details;

void main()
{
    outFragColor = Details.color1;
}