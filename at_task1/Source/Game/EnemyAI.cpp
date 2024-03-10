#include "Pch.h"
#include "EnemyAI.h"
#include "Level.h"
#include "Console.h"

#include "Game/GameState.h"
#include "Game/GameEvents.h"
#include "Game/GameAudio.h"

#include "Game/Components/TransformComponent.h"
#include "Game/Components/EnemyAIComponent.h"

void EnemyAI::Update(Level& level)
{
	auto attackPlayer = [](const glm::vec3& enemyPosition, const glm::vec3& playerPosition, const EnemyAIComponent& aiComponent) {
		// Calculate the distance between the possessed entity and the enemy
		auto distance = glm::length(enemyPosition - playerPosition);

		// Check if the distance is smaller than the enemy's attack range
		if (distance < aiComponent.AttackRadius)
		{
			// Play enemy attack sound
			GameAudio::PlayEnemyAttackSound();

			// Damage the player
			GameEvents::PlayerDamaged();
			if (GameState::DamagePlayer(aiComponent.AttackDamage))
			{
				// The player has been defeated
				GameEvents::PlayerDefeated();
			}
		}
	};

	// Get the level's possessed entity
	const auto* pPossessedEntity = level.GetPossessedEntity();

	// Check the possessed entity is valid
	if (!pPossessedEntity->Valid())
	{
		return;
	}

	// Get the level's ecs registry
	auto& ecsRegistry = level.GetECSRegistry();

	// Create a view of entities with transform and enemy AI components
	auto entityView = ecsRegistry.view<TransformComponent, EnemyAIComponent>();

	// For each entity in the view
	for (auto [entity, transformComponent, aiComponent] : entityView.each())
	{
		// Check if the enemy ai is inactive
		if (!aiComponent.Active)
		{
			// Skip this entity if it is inactive
			continue;
		}

		auto timeNow = std::chrono::high_resolution_clock::now();

		if (aiComponent.FirstAttack)
		{
			aiComponent.FirstAttack = false;

			attackPlayer(transformComponent.Transform.Position, pPossessedEntity->GetComponent<TransformComponent>().Transform.Position, aiComponent);

			// Update the ai's last attack time
			aiComponent.LastAttackTime = timeNow;

			continue;
		}

		// Check if enough time has elapsed since the enemy's last attack
		std::chrono::duration<float, std::milli> elapsed = (timeNow - aiComponent.LastAttackTime);
		if ((elapsed.count() * 0.001f) >= aiComponent.AttackRateSeconds)
		{
			attackPlayer(transformComponent.Transform.Position, pPossessedEntity->GetComponent<TransformComponent>().Transform.Position, aiComponent);

			// Update the ai's last attack time
			aiComponent.LastAttackTime = timeNow;
		}
	}
}
