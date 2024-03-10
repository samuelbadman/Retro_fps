#include "Pch.h"
#include "Level.h"
#include "Game/Components/TransformComponent.h"
#include "Game/Components/CameraComponent.h"

void Level::Begin()
{
	// Check a hud class is set
	if (HUDClassInstance)
	{
		// Begin the hud
		HUDClassInstance->Begin();
	}
}

void Level::Tick(const float deltaTime)
{
	// Check a hud class is set
	if (HUDClassInstance)
	{
		// Tick the hud
		HUDClassInstance->Tick(deltaTime);
	}
}

Entity* Level::CreateEntity()
{
	return &Entities.emplace_back(ECSRegistry.create(), &ECSRegistry);
}

void Level::DestroyEntity(Entity* entity)
{
	Entities.erase(std::find(Entities.begin(), Entities.end(), *entity));
}

bool Level::PossessEntity(Entity* entity)
{
	// Check if the entity has a transform and camera component
	if (entity->HasAllComponents<TransformComponent, CameraComponent>())
	{
		PossessedEntity = *entity;
		return true;
	}

	return false;
}
