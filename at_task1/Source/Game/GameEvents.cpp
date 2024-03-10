#include "Pch.h"
#include "GameEvents.h"
#include "Maths/Maths.h"
#include "Console.h"

#include "Game/World.h"
#include "Game/CollisionDetection.h"
#include "Game/GameAudio.h"
#include "Game/Tags.h"
#include "Game/GameState.h"
#include "Game/HUDs/FPSHUD.h"

#include "Game/Components/TransformComponent.h"
#include "Game/Components/BillboardComponent.h"
#include "Game/Components/AABBCollisionComponent.h"
#include "Game/Components/SoundEmitter3DComponent.h"
#include "Game/Components/StaticMeshComponent.h"
#include "Game/Components/TagComponent.h"
#include "Game/Components/EnemyAIComponent.h"

constexpr float GunFireRateSeconds{ 0.25f };

static std::chrono::high_resolution_clock::time_point gTimeLastFired = std::chrono::high_resolution_clock::now();
static bool gFirstFire{ true };

static bool gPlayerWithinLevelGoalRadius{ false };

static void OnEnemyShot(entt::registry& ecsRegistry, const entt::entity enemyEntity)
{
	// Play the enemy death sound emitter at the enemy's position
	ecsRegistry.get<SoundEmitter3DComponent>(enemyEntity).SoundSourceVoice.GetSourceVoice()->Start();

	// Destroy the enemy entity. Cannot destroy here as other entity components are being used to calculate 3D audio
	//ecsRegistry.destroy(entity);

	// For now, disable other components on the entity
	ecsRegistry.get<AABBCollisionComponent>(enemyEntity).CollisionEnabled = false;
	ecsRegistry.get<StaticMeshComponent>(enemyEntity).Visible = false;
	ecsRegistry.get<BillboardComponent>(enemyEntity).Active = false;
	ecsRegistry.get<EnemyAIComponent>(enemyEntity).Active = false;
}

static void PlayerEnteredLevelGoalInteractRadius()
{
	// Get the loaded level's hud class instance as FPS HUD
	auto* fpsHUD = dynamic_cast<FPSHUD*>(World::GetLoadedLevel().GetHUDClassInstance());

	// Check FPS HUD is valid
	if (fpsHUD == nullptr)
	{
		return;
	}

	// Show the interact prompt in the hud
	fpsHUD->SetInteractPromptVisible(true);
}

static void PlayerLeftLevelGoalInteractRadius()
{
	// Get the loaded level's hud class instance as FPS HUD
	auto* fpsHUD = dynamic_cast<FPSHUD*>(World::GetLoadedLevel().GetHUDClassInstance());

	// Check FPS HUD is valid
	if (fpsHUD == nullptr)
	{
		return;
	}

	// Hide the interact prompt in the hud
	fpsHUD->SetInteractPromptVisible(false);
}

void GameEvents::Init()
{
}

void GameEvents::PlayerInteracted()
{
	if (gPlayerWithinLevelGoalRadius)
	{
		GameState::LoadNextLevel();
	}
}

void GameEvents::PlayerFired()
{
	auto timeNow = std::chrono::high_resolution_clock::now();
	std::chrono::duration<float, std::milli> elapsed = (timeNow - gTimeLastFired);
	auto fElapsed = elapsed.count();

	if (gFirstFire)
	{
		fElapsed = GunFireRateSeconds;
		gFirstFire = false;
	}

	if ((elapsed.count() * 0.001f) >= GunFireRateSeconds)
	{
		gTimeLastFired = timeNow;

		// Play gunshot sound
		GameAudio::PlayRandomGunshotSound();

		// Get the loaded level
		const auto& loadedLevel = World::GetLoadedLevel();

		// Get possessed transform from level
		const auto& possessedTransform = loadedLevel.GetPossessedEntity()->GetComponent<TransformComponent>().Transform;

		// Get ECS registry from loaded level
		auto& ecsRegistry = World::GetLoadedLevel().GetECSRegistry();

		// Create a view of entities with transform, billboard and AABB collision components
		auto aabbView = ecsRegistry.view<TransformComponent, AABBCollisionComponent>();

		struct HitPositionAndEntity
		{
			HitPositionAndEntity(const glm::vec3& pos, const entt::entity entity) : Position(pos), Entity(entity) {}

			glm::vec3 Position;
			entt::entity Entity;
		};

		// For each entity in the view
		std::vector<HitPositionAndEntity> hits;
		hits.reserve(aabbView.size_hint());
		for (auto [entity, entityTransformComponent, aabb] : aabbView.each())
		{
			// Check if the aabb component is enabled
			if (aabb.CollisionEnabled)
			{
				// Test if the player's shot hits an entity by raycasting (a line from the player forwards into the scene) for aabbs
				CollisionDetection::CollisionTestResult testResult{};
				if (CollisionDetection::TestLineAABB(possessedTransform.Position,
					possessedTransform.Position + (glm::normalize(Maths::RotateVector(possessedTransform.Rotation, World::GetWorldForwardVector()))),
					entityTransformComponent.Transform.Position,
					aabb.Extent,
					testResult))
				{
					// Store the hit position and entity
					hits.emplace_back(testResult.HitPosition, entity);
				}
			}
		}

		// Sort hits based on their distance from the camera
		std::sort(hits.begin(), hits.end(), [&cameraPosition = possessedTransform.Position](const auto& lhs, const auto& rhs) {
			auto lhsDistance = glm::length(cameraPosition - lhs.Position);
			auto rhsDistance = glm::length(cameraPosition - rhs.Position);

			return lhsDistance < rhsDistance;
		});

		// Check if the first hit was an enemy
		const auto& firstHit = hits[0];
		if (ecsRegistry.all_of<TagComponent>(firstHit.Entity))
		{
			if (ecsRegistry.get<TagComponent>(firstHit.Entity).Tag == Tags::ENEMY_TAG)
			{
				OnEnemyShot(ecsRegistry, firstHit.Entity);

				// An enemy was shot and checking hits can stop
				return;
			}
		}

		// For each other hit along the ray
		for (size_t i = 1; i < hits.size(); ++i)
		{
			const auto& hit = hits[i];

			// Check if the hit entity was an enemy.
			if (ecsRegistry.all_of<TagComponent>(hit.Entity))
			{
				if (ecsRegistry.get<TagComponent>(hit.Entity).Tag == Tags::ENEMY_TAG)
				{
					OnEnemyShot(ecsRegistry, hit.Entity);

					// An enemy was shot and checking hits can stop
					break;
				}
			}
			else
			{
				// Something else was hit and checking hits can stop
				// Spawn decal, bullet impact particles, impact sound emitter etc... at hit position
				break;
			}
		}
	}
}

void GameEvents::SetPlayerWithinLevelGoalRadius(bool withinRadius)
{
	// Check if within radius state has changed
	if (gPlayerWithinLevelGoalRadius != withinRadius)
	{
		if (withinRadius)
		{
			PlayerEnteredLevelGoalInteractRadius();
		}
		else
		{
			PlayerLeftLevelGoalInteractRadius();
		}
	}

	gPlayerWithinLevelGoalRadius = withinRadius;
}

void GameEvents::PlayerDefeated()
{
	// Get the loaded level
	auto& loadedLevel = World::GetLoadedLevel();

	// Get the level's hud instance as FPSHUD
	auto* pFPSHUD = dynamic_cast<FPSHUD*>(loadedLevel.GetHUDClassInstance());
	assert(pFPSHUD != nullptr && "Failed to cast level hud class instance to FPSHUD in player defeated event.");

	// Show the player defeated screen
	pFPSHUD->ShowPlayerDefeatedScreen();

	// Set the game state to player defeated
	GameState::SetCurrentState(GameState::EState::LOSE_SCREEN);
}

void GameEvents::PlayerDamaged()
{
	GameAudio::PlayInjuredSound();
}
