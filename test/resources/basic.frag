#version 450

layout (location = 0) in vec2 texCoord;

layout (location = 0) out vec4 outFragColor;

layout(set = 0, binding = 0) uniform details
{
 	vec4 color1;
	vec4 color2;
} Details;

layout(set = 0, binding = 1) uniform secondDetails
{
	vec4 color3;
}Deets;

void main()
{
	outFragColor = (Details.color1+Deets.color3)*Details.color2;
}