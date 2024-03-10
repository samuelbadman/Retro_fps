#pragma once

#include "Level.h"

namespace World
{
	const glm::vec3& GetWorldForwardVector();
	const glm::vec3& GetWorldRightVector();
	const glm::vec3& GetWorldUpVector();
	Level& GetLoadedLevel();

	float GetWorldDeltaTime();
	void SetWorldDeltaTime(const float deltaTime);

	float GetWorldGravityScale();
	void SetWorldGravityScale(float scale);

	bool LoadLevel(std::unique_ptr<Level>&& level, const bool force);
	bool IslevelScheduled();
	bool LoadScheduledLevel();
}