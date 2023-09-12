#version 450

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aUV;
layout (location = 2) in vec3 aNormal;

layout (location = 0) out vec3 fragColour;

layout (push_constant) uniform constants
{
    mat4 world;
    mat4 viewProj;
} PushConstants;

void main()
{
    gl_Position = PushConstants.viewProj * PushConstants.world * vec4(aPos, 1.0);
    fragColour = aNormal;
}