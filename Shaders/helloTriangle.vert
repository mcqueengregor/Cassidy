#version 450

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aUV;
layout (location = 2) in vec3 aNormal;

layout (location = 0) out vec2 uv;
layout (location = 1) out vec3 normalWS;
layout (location = 2) out vec3 positionWS;

layout (set = 0, binding = 0) uniform MatrixBuffer
{
    mat4 view;
    mat4 proj;
    mat4 viewProj;
    mat4 invViewProj;
} u_matrixBuffer;

layout (set = 1, binding = 0) uniform PerObjectBuffer
{
    mat4 world;
} u_perObjectBuffer;

void main()
{
    positionWS = (u_perObjectBuffer.world * vec4(aPos, 1.0)).xyz;
    gl_Position = u_matrixBuffer.viewProj * vec4(positionWS, 1.0);
    uv = aUV;
    normalWS = (u_perObjectBuffer.world * vec4(aNormal, 0.0)).xyz;
}