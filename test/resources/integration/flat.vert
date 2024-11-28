#version 450

layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec2 vUV;

layout (location = 0) out vec2 texCoord;


void main()
{
    gl_Position = vec4(vPosition+sharedData,1.0f);
    texCoord = vUV;
}