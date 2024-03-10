#version 450

// Input
layout(location = 0) in vec3 localSpacePosition;
layout(location = 1) in vec2 textureCoord;
layout(location = 2) in vec3 vertexNormal;

// Uniforms
layout(binding = 0) uniform PerObjectUniforms
{
	mat4 WorldMatrix;
	mat4 NormalMatrix;
	vec4 Data1;
	int SamplerID;
	int TextureID;
};

layout(binding = 1) uniform PerFrameUniforms
{
	vec4 DirectionalLightColor;
	vec4 DirectionalLightWorldSpaceDirection;
};

layout(binding = 2) uniform PerRenderPassUniforms
{
	mat4 ViewMatrix;
	mat4 ProjectionMatrix;
	vec4 CameraWorldSpacePosition;
};

// Output
layout(location = 0) out vec2 outTextureCoord;
layout(location = 1) out int outSamplerID;
layout(location = 2) out int outTextureID;
layout(location = 3) out vec3 outWorldSpaceNormal;
layout(location = 4) out vec3 outDirectionalLightColor;
layout(location = 5) out vec3 outDirectionalLightWorldSpaceDirection;
layout(location = 6) out vec3 outWorldSpaceCameraVector;
layout(location = 7) out vec2 outTextureScale;

void main()
{
	// Transform local space position to world space
	vec4 worldSpacePosition = WorldMatrix * vec4(localSpacePosition, 1.0f);

	// Transform world space position to view space
	vec4 viewSpacePosition = ViewMatrix * worldSpacePosition;

	// Transform view space position to projection space and write to gl_Position
	gl_Position = ProjectionMatrix * viewSpacePosition;
	// Write outputs to fragment shader
	outTextureCoord = textureCoord;
	outSamplerID = SamplerID;
	outTextureID = TextureID;
	// Transform vertex normal to world space normal
	outWorldSpaceNormal = normalize((NormalMatrix * vec4(vertexNormal, 0.0f)).xyz);
	outDirectionalLightColor = DirectionalLightColor.rgb;
	outDirectionalLightWorldSpaceDirection = normalize(DirectionalLightWorldSpaceDirection.xyz);
	outWorldSpaceCameraVector = normalize(CameraWorldSpacePosition.xyz - worldSpacePosition.xyz);
	outTextureScale = vec2(Data1.r, Data1.g);
}