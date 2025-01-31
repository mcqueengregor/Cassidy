#version 450

layout (location = 0) in vec2 uv;

layout (location = 0) out vec4 outColour;

layout (set = 2, binding = 0) uniform sampler2D albedoTex;
layout (set = 2, binding = 1) uniform sampler2D specularTex;
layout (set = 2, binding = 2) uniform sampler2D normalTex;

struct DirectionalLight
{
	vec4 direction;
	vec4 colour;
};

layout (push_constant) uniform constants
{
	layout(offset = 128) uint texToDisplay;	// Dealing with 'DefaultPushConsants' in vertex shader (two 4x4 matrices)
	DirectionalLight directionalLightDir;
} u_pushConstants;

#define USE_ALBEDO_TEX 0
#define USE_SPECULAR_TEX 1
#define USE_NORMAL_TEX 2

void main()
{
	vec3 albedoColour = texture(albedoTex, uv).rgb;
	float specularColour = texture(specularTex, uv).r;
	vec3 normalColour = texture(normalTex, uv).rgb;

	switch (u_pushConstants.texToDisplay.x)
	{
		case USE_ALBEDO_TEX:
			outColour = vec4(pow(albedoColour, vec3(1 / 2.2)), 1.0);
			break;
		case USE_SPECULAR_TEX:
			outColour = vec4(specularColour.rrr, 1.0);
			break;
		case USE_NORMAL_TEX:
			outColour = vec4(normalColour, 1.0);
			break;
		default:
			outColour = vec4(1.0, 0.0, 1.0, 1.0);	// Magenta error colour
			break;
	}
}