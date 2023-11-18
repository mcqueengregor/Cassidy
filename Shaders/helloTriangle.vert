#version 450

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aUV;
layout (location = 2) in vec3 aNormal;

layout (location = 0) out vec2 uv;

layout (set = 0, binding = 0) uniform PerPassBuffer
{
    mat4 view;
    mat4 proj;
    mat4 viewProj;
    mat4 invViewProj;
} perPassBuffer;

layout (set = 1, binding = 0) uniform PerObjectBuffer
{
    mat4 world;
} perObjectBuffer;

layout (push_constant) uniform constants
{
    mat4 world;
    mat4 viewProj;
} PushConstants;

void main()
{
    gl_Position = perPassBuffer.viewProj * perObjectBuffer.world * vec4(aPos, 1.0);
    uv = aUV;
}