#version 450

layout (location = 0) in vec2 uv;
layout (location = 1) in vec3 normalWS;
layout (location = 2) in vec3 positionWS;

layout (location = 0) out vec4 outColour;

layout (set = 0, binding = 0) uniform PerPassBuffer
{
    mat4 view;
    mat4 proj;
    mat4 viewProj;
    mat4 invViewProj;
} u_perPassBuffer;

layout (set = 2, binding = 0) uniform sampler2D albedoTex;
layout (set = 2, binding = 1) uniform sampler2D specularTex;
layout (set = 2, binding = 2) uniform sampler2D normalTex;

struct DirectionalLight
{
	vec4 directionWS;
	vec3 colour;
	float ambient;
};

layout (push_constant) uniform constants
{
	layout(offset = 128) DirectionalLight directionalLight;	// Dealing with 'DefaultPushConsants' in vertex shader (two 4x4 matrices)
} u_pushConstants;

void main()
{
	const DirectionalLight dirLight = u_pushConstants.directionalLight;
	const vec3 camPos = u_perPassBuffer.view[3].xyz;

	const vec3 lightDir = normalize(dirLight.directionWS.xyz);
	const vec3 viewDir = normalize(camPos - positionWS);
	const vec3 halfVec = normalize(lightDir + viewDir);
	const vec3 normal = normalize(normalWS);

	const float NdotL = max(dot(normal, lightDir), 0.0);
	const float NdotH = max(dot(normal, halfVec), 0.0);

	vec3 albedoColour = texture(albedoTex, uv).rgb;
	float specularColour = texture(specularTex, uv).r;
	vec3 normalColour = texture(normalTex, uv).rgb;
	
	const float diff = NdotL;
	const float spec = pow(NdotH, 64.0) * specularColour;

	vec3 lightingResults = (diff + spec + dirLight.ambient) * albedoColour * dirLight.colour.rgb;
	
	outColour = vec4(pow(lightingResults.rgb, vec3(1.0 / 2.2)), 1.0);
}