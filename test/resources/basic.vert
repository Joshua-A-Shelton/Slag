#version 450

layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec2 vUV;

layout (location = 0) out vec2 texCoord;

layout(set = 0, binding = 0) uniform offset
{
	vec3 offsetPosition;
}sharedData;

void main()
{
	gl_Position = vec4((vPosition+sharedData.offsetPosition),1.0f);
	texCoord = vUV;
}