#include "Pch.h"
#include "LevelGoal.h"
#include "Level.h"
#include "Console.h"

#include "Game/Tags.h"
#include "Game/GameEvents.h"

#include "Game/Components/TransformComponent.h"
#include "Game/Components/SphereCollisionComponent.h"
#include "Game/Components/TagComponent.h"

void LevelGoal::Update(Level& level)
{
	// Get the level's possessed entity
	auto* pPossessedEntity = level.GetPossessedEntity();

	// Early exit if the possessed entity is invalid
	if (!pPossessedEntity->Valid())
	{
		return;
	}

	// Get the level's ecs registry
	const auto& ecsRegistry = level.GetECSRegistry();

	// Create a view of entities with transform, tag and sphere collision components
	auto registryView = ecsRegistry.view<TransformComponent, SphereCollisionComponent, TagComponent>();

	// For each entity in the view
	for (auto [entity, transformComponent, sphereCollisionComponent, tagComponent] : registryView.each())
	{
		// Calculate the distance between the level goal and the player
		auto distance = glm::length(pPossessedEntity->GetComponent<TransformComponent>().Transform.Position - transformComponent.Transform.Position);

		// Check if the player is within interact range of the level goal
		GameEvents::SetPlayerWithinLevelGoalRadius(distance <= sphereCollisionComponent.Radius);
	}
}
