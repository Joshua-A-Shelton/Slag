#version 450
layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec2 vUV;

void main()
{
    gl_Position = vec4(vPosition,1.0f);

}