#version 450

// Definitions
#define SAMPLER_COUNT 1
#define MAX_TEXTURE_COUNT 32

// Input
layout(location = 0) in vec2 textureCoord;
layout(location = 1) flat in int inSamplerID;
layout(location = 2) flat in int inTextureID;
layout(location = 3) in vec3 worldSpaceNormal;
layout(location = 4) in vec3 directionalLightColor;
layout(location = 5) in vec3 directionalLightWorldSpaceDirection;
layout(location = 6) in vec3 worldSpaceCameraVector;
layout(location = 7) in vec2 textureScale;

// Uniforms
layout(binding = 3) uniform sampler samplers[SAMPLER_COUNT];
layout(binding = 4) uniform texture2D textures[MAX_TEXTURE_COUNT];

// Output
layout(location = 0) out vec4 outColor;

// Applies gamma to the color
 vec3 GammaCorrect(vec3 color, float gamma)
 { 
	float exponent = 1.0f / gamma;
	return pow(color, vec3(exponent));
 }

void main()
{
	// Sample the texture at inTextureID with the sampler at inSamplerID. Sampled color is in linear space
	vec4 baseColor = texture(sampler2D(textures[inTextureID], samplers[inSamplerID]), vec2(textureCoord.r, -textureCoord.g) * textureScale);

	// Calculate ambient light contribution
	const vec3 ambient = vec3(0.05f, 0.05f, 0.05f);

	// Calculate diffuse light contribution
	const float directionalDiffuseLight = clamp(dot(worldSpaceNormal, -directionalLightWorldSpaceDirection), 0.0f, 1.0f);
	vec3 diffuse = directionalLightColor * directionalDiffuseLight;

	// Calculate specular light contribution
	vec3 halfAngle = normalize(-directionalLightWorldSpaceDirection + worldSpaceCameraVector);
	float normalDotHalfAngle = clamp(dot(worldSpaceNormal, halfAngle), 0.0f, 1.0f);
	float gloss = 4.0f;
	const vec3 specularColor = vec3(1.0f, 1.0f, 1.0f);
	float specularPower = pow(normalDotHalfAngle, gloss);
	vec3 specular = specularColor * specularPower;

	// Calculate total lighting contribtion
	vec3 lighting = ambient + diffuse + specular;

	// Calculate final color with lighting contributions
	vec4 finalColor = vec4(lighting * baseColor.rgb, baseColor.a);

	// Output final color transformed to gamma space
	outColor = vec4(GammaCorrect(finalColor.rgb, 2.2f), finalColor.a);
}