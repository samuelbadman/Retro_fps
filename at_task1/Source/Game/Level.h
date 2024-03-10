#pragma once

#include "Renderer/DirectionalLight.h"
#include "Entity.h"
#include "Game/HUD.h"

class Level
{
public:
	using Super = Level;

	const Renderer::DirectionalLight& GetDirectionalLight() const { return DirectionalLight; }
	Renderer::DirectionalLight& GetDirectionalLight() { return DirectionalLight; }
	entt::registry& GetECSRegistry() { return ECSRegistry; }
	const entt::registry& GetECSRegistry() const { return ECSRegistry; }
	Entity* GetPossessedEntity() { return &PossessedEntity; }
	const Entity* GetPossessedEntity() const { return &PossessedEntity; }
	HUD* GetHUDClassInstance() const { return HUDClassInstance.get(); }

	virtual ~Level() = default;

	virtual bool Load() = 0;
	virtual void UnLoad() = 0;
	virtual void Begin();
	virtual void Tick(const float deltaTime);
	virtual void TickFixed() = 0;

protected:
	Entity* CreateEntity();
	void DestroyEntity(Entity* entity);
	// Possesses an entity if the entity has a transform and camera component. The camera will be used as the view camera
	// to view the level. Returns whether the entity was succesfully possessed
	bool PossessEntity(Entity* entity);
	//const std::vector<Entity>& GetEntities() const { return Entities; }
	//std::vector<Entity>& GetEntities() { return Entities; }

	std::unique_ptr<HUD> HUDClassInstance;

private:
	entt::registry ECSRegistry;
	std::vector<Entity> Entities;
	Renderer::DirectionalLight DirectionalLight{};
	Entity PossessedEntity{};
};