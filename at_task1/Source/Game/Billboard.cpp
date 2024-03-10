#include "Pch.h"
#include "Billboard.h"
#include "Level.h"
#include "Maths/Maths.h"

#include "Game/World.h"
#include "Game/Components/TransformComponent.h"
#include "Game/Components/BillboardComponent.h"

#include "Console.h"

void Billboard::Update(Level& level)
{
	// Get the possessed entity
	auto* pPossessedEntity = level.GetPossessedEntity();

	// Check the possessed entity is valid
	if (!pPossessedEntity->Valid())
	{
		return;
	}

	// Make a copy of the possessed entity position
	auto possessedPosition = level.GetPossessedEntity()->GetComponent<TransformComponent>().Transform.Position;

	// Get level ECS registry
	auto& ecsRegistry = level.GetECSRegistry();

	// Create a view of entities with billboard and transform components
	auto billboardView = ecsRegistry.view<TransformComponent, BillboardComponent>();

	// For each entity in the view
	for (auto [billboardEntity, billboardEntityTransform, billboard] : billboardView.each())
	{
		// Skip this billboard component if it is not active
		if(!billboard.Active)
		{
			continue;
		}

		// Make a copy of the billboard entity position
		auto billboardPosition = billboardEntityTransform.Transform.Position;

		// Check if the billboard cannot lean
		if (!billboard.CanLean)
		{
			// Zero out the Y position of the billboard and possessed transforms
			billboardPosition.y = 0.0f;
			possessedPosition.y = 0.0f;
		}

		// Calculate euler rotation to make the billboard face the possessed transform
		billboardEntityTransform.Transform.Rotation = Maths::RotationMatrix4ToEuler(
			glm::mat4_cast(glm::quatLookAtLH(glm::normalize(billboardPosition - possessedPosition), World::GetWorldUpVector()))
		);

		// Flip billboard to correct orientation
		billboardEntityTransform.Transform.Rotation.z += 180.0f;
		billboardEntityTransform.Transform.Rotation.y *= -1.0f;

		if (billboard.CanLean)
		{
			billboardEntityTransform.Transform.Rotation.x *= -1.0f;
		}
	}
}
