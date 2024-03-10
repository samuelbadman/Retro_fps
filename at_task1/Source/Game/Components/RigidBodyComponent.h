#pragma once

struct RigidBodyComponent
{
	bool ApplyGravity{ true };
	glm::vec3 Velocity{ 0.0f, 0.0f, 0.0f };
};