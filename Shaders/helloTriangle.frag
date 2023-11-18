#version 450

layout (location = 0) in vec2 uv;

layout (location = 0) out vec4 outColour;

layout (set = 1, binding = 1) uniform sampler2D albedoTex;

void main()
{
	vec4 albedoColour = texture(albedoTex, uv);
	outColour = vec4(pow(albedoColour.rgb, vec3(1 / 2.2)), 1.0);
}