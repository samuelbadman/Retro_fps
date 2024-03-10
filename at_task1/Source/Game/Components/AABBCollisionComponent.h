#pragma once

struct AABBCollisionComponent
{
	bool CollisionEnabled{ true };
	bool CanCollideWithSphereCollisions{ true };
	glm::vec3 Extent{ 1.0f, 1.0f, 1.0f };
};