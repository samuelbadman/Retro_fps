#pragma once

#include "Vertex1Pos1UV1Norm.h"
#include "DrawItem.h"
#include "CameraSettings.h"
#include "DirectionalLight.h"

class Level;
class HUD;

namespace Renderer
{
	enum class EVulkanDebugReportLevel : uint8_t
	{
		ALL = 0,
		DEBUG = 0,
		INFO,
		WARNING,
		PERFORMANCE_WARNING,
		ERR,
		NONE
	};

	enum class ESampler : uint8_t
	{
		LINEAR_FILTER = 0,
		NEAREST_NEIGHBOUR_FILTER = 1,
	};

	bool Init(const glm::vec2& windowClientAreaResolution, HWND windowHandle);
	void UpdateDescriptorSets();
	bool Shutdown();
	void SetVulkanDebugReportLevel(const EVulkanDebugReportLevel level);
	bool BeginFrame(const Renderer::DirectionalLight& directionalLight);
	void BeginRenderPass(const glm::vec3& viewPosition,
		const glm::vec3& viewRotation,
		const Renderer::CameraSettings& cameraSettings);
	bool EndFrame();
	bool Submit(
		const Renderer::DrawItem* drawItems, 
		uint32_t drawItemCount);
	bool SubmitLevel(Level& level);
	bool SubmitHUD(HUD& hud);
	bool LoadGeometry(
		const Vertex1Pos1UV1Norm* vertices,
		const uint32_t vertexCount, 
		const uint32_t* indices,
		const uint32_t indexCount,
		uint32_t* pID);
	bool LoadPlaneGeometryPrimitive(const float width, uint32_t* pID);
	bool LoadCubeGeometryPrimitive(const float width, uint32_t* pID);
	bool LoadSphereGeometryPrimitive(const float radius, const int32_t sectors, const int32_t stacks, uint32_t* pID);
	bool LoadCylinderGeometryPrimitive(const float baseRadius, const float topRadius, const float height, const int32_t sectors, const int32_t stacks, uint32_t* pID);
	bool LoadConeGeometryPrimitive(const float baseRadius, const float height, const int32_t sectors, const int32_t stacks, uint32_t* pID);
	bool LoadTexture(const std::string& textureAssetFilepath, const bool generateMipmaps, uint32_t* pID);
	void DestroyGeometry(const uint32_t id);
	void DestroyTexture(const uint32_t id);
	bool WaitForIdle();
}