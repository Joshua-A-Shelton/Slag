#version 450

layout (location = 0) in vec2 texCoord;

layout (location = 0) out vec4 outFragColor;

layout(set = 0, binding = 0) uniform details
{
 	vec4 color1;
	vec4 color2;
} Details;

void main()
{
	outFragColor = Details.color1*Details.color2;
}