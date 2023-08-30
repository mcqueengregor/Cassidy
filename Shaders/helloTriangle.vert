#version 450

//layout (location = 0) in vec3 aPos;
//layout (location = 1) in vec2 aUV;
//layout (location = 2) in vec3 aNormal;

layout (location = 0) out vec3 fragColour;

vec2 positions[3] = vec2[](
    vec2(0.0, -0.5),
    vec2(0.5, 0.5),
    vec2(-0.5, 0.5)
);

vec3 colours[3] = vec3[](
    vec3(1.0, 0.0, 0.0),
    vec3(0.0, 1.0, 0.0),
    vec3(0.0, 0.0, 1.0)
);

void main()
{
    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
    fragColour = colours[gl_VertexIndex];
}