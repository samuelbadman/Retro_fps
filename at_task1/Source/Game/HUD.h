#pragma once

#include "Renderer/CameraSettings.h"
#include "Game/Entity.h"

class HUD
{
public:
	using Super = HUD;

	const entt::registry& GetECSRegistry() const { return ECSRegistry; }
	entt::registry& GetECSRegistry() { return ECSRegistry; }
	const glm::vec3& GetHUDCameraPosition() const { return HUDCameraPosition; }
	const glm::vec3& GetHUDCameraRotation() const { return HUDCameraRotation; }
	const Renderer::CameraSettings& GetHUDCameraSettings() const { return HUDCameraSettings; }

	virtual bool Load();
	virtual void UnLoad();
	virtual void Begin() = 0;
	virtual void Tick(const float deltaTime) = 0;

protected:
	Entity* CreateHUDEntity();

	glm::vec3 HUDCameraPosition{ 0.0f, 0.0f, 0.0f };
	glm::vec3 HUDCameraRotation{ 0.0f, 0.0f, 0.0f };
	Renderer::CameraSettings HUDCameraSettings{};
	uint32_t ImageGeometryID{ std::numeric_limits<uint32_t>::max() };

private:
	entt::registry ECSRegistry;
	std::vector<Entity> HUDEntities;
};