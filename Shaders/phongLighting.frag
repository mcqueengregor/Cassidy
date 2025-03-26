#version 450

layout (location = 0) in vec2 uv;
layout (location = 1) in vec3 normalWS;
layout (location = 2) in vec3 positionWS;

layout (location = 0) out vec4 outColour;

layout (set = 0, binding = 0) uniform MatrixBuffer
{
    mat4 view;
    mat4 proj;
    mat4 viewProj;
    mat4 invViewProj;
} u_matrixBuffer;

struct DirectionalLight
{
	vec4 directionWS;
	vec3 colour;
	float ambient;
};

#define NUM_LIGHTS 4

layout (set = 0, binding = 1) uniform LightBuffer
{
	uvec4 numActiveLights;
	DirectionalLight dirLights[NUM_LIGHTS];
} u_lightBuffer;

layout (set = 2, binding = 0) uniform sampler2D albedoTex;
layout (set = 2, binding = 1) uniform sampler2D specularTex;
layout (set = 2, binding = 2) uniform sampler2D normalTex;

void main()
{
	vec3 albedoColour = texture(albedoTex, uv).rgb;
	float specularColour = texture(specularTex, uv).r;
	vec3 normalColour = texture(normalTex, uv).rgb;

	const vec3 normal = normalize(normalWS);
	const vec3 camPos = u_matrixBuffer.view[3].xyz;
	const vec3 viewDir = normalize(camPos - positionWS);

	vec3 lighting = vec3(0.0);

	for (uint i = 0; i < u_lightBuffer.numActiveLights.x; ++i)
	{
		const DirectionalLight dirLight = u_lightBuffer.dirLights[i];
	
		const vec3 lightDir = normalize(dirLight.directionWS.xyz);
		const vec3 halfVec = normalize(lightDir + viewDir);

		const float NdotL = max(dot(normal, lightDir), 0.0);
		const float NdotH = max(dot(normal, halfVec), 0.0);

		const float diff = NdotL;
		const float spec = pow(NdotH, 64.0) * specularColour;

		lighting += (diff + spec + dirLight.ambient) * dirLight.colour.rgb;
	}

	lighting *= albedoColour;
	
	outColour = vec4(pow(lighting.rgb, vec3(1.0 / 2.2)), 1.0);
}