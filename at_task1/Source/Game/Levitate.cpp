#include "Pch.h"
#include "Levitate.h"
#include "Level.h"
#include "Maths/Maths.h"
#include "Console.h"

#include "Game/World.h"

#include "Game/Components/TransformComponent.h"
#include "Game/Components/LevitateComponent.h"

void Levitate::Update(Level& level)
{
	// Get level's ecs registry
	auto& ecsRegistry = level.GetECSRegistry();

	// Create a view of entities with a transform and levitate component
	auto entityView = ecsRegistry.view<TransformComponent, LevitateComponent>();

	// For each entity in the view
	for (auto [entity, transform, levitate] : entityView.each())
	{
		auto deltaTime = World::GetWorldDeltaTime();

		if (levitate.MovingUp)
		{
			transform.Transform.Position.y -= levitate.LevitateRate * deltaTime;

			if (transform.Transform.Position.y < (levitate.OriginalHeight - levitate.MaxHeightDelta))
			{
				levitate.MovingUp = false;
			}
		}
		else
		{
			transform.Transform.Position.y += levitate.LevitateRate * deltaTime;

			if (transform.Transform.Position.y > (levitate.OriginalHeight))
			{
				levitate.MovingUp = true;
			}
		}

		transform.Transform.Rotation += levitate.RotationDelta * deltaTime;
	}
}
