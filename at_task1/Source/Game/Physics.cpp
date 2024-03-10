#include "Pch.h"
#include "Physics.h"

#include "Game/CollisionDetection.h"
#include "Game/World.h"
#include "Game/Level.h"
#include "Game/Components/RigidBodyComponent.h"
#include "Game/Components/TransformComponent.h"
#include "Game/Components/SphereCollisionComponent.h"
#include "Game/Components/AABBCollisionComponent.h"

#include "Console.h"

void Physics::Update(Level& level)
{
	// Get level ECS registry
	auto& ecsRegistry = level.GetECSRegistry();

	// Create a view of entities with rigidbody and transform components
	auto rigidBodyView = ecsRegistry.view<TransformComponent, RigidBodyComponent>();

	// Create a view of entities with AABBCollisionComponents and transform components
	auto aabbView = ecsRegistry.view<TransformComponent, AABBCollisionComponent>();

	// For each entity with a rigidbody
	for (auto [rigidBodyEntity, rigidBodyEntityTransform, rigidBody] : rigidBodyView.each())
	{
		// Check if gravity should be applied to the rigidbody
		if (rigidBody.ApplyGravity)
		{
			// Add gravity to rigidbody velocity
			rigidBody.Velocity += -World::GetWorldUpVector() * World::GetWorldGravityScale();
		}

		// Compute next position of the entity
		auto nextPosition = rigidBodyEntityTransform.Transform.Position + rigidBody.Velocity;

		// Check if the entity has a sphere collision component
		if (ecsRegistry.any_of<SphereCollisionComponent>(rigidBodyEntity))
		{
			// Test sphere collision against all AABB collisions in the level
			for (auto [aabbEntity, aabbEntityTransform, aabb] : aabbView.each())
			{
				// Check if the aabb can collide with sphere collisions and collisions are enabled
				if (aabb.CanCollideWithSphereCollisions && aabb.CollisionEnabled)
				{
					// Prevent collisions with self
					if (aabbEntity != rigidBodyEntity)
					{
						CollisionDetection::CollisionTestResult testResult;
						if (CollisionDetection::TestSphereAABB(nextPosition, ecsRegistry.get<SphereCollisionComponent>(rigidBodyEntity),
							aabbEntityTransform.Transform.Position, aabb, testResult))
						{
							// Remove collision normal from rigidbody velocity
							rigidBody.Velocity -= testResult.HitSurfaceNormal * glm::dot(rigidBody.Velocity, testResult.HitSurfaceNormal);
						}
					}
				}
			}
		}

		// Apply velocity to transform
		rigidBodyEntityTransform.Transform.Position += rigidBody.Velocity;

		// Zero velocity
		rigidBody.Velocity = { 0.0f, 0.0f, 0.0f };
	}
}
