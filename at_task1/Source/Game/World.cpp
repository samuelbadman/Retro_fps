#include "Pch.h"
#include "Game/World.h"
#include "Renderer/Renderer.h"

constexpr glm::vec3 gWorldForward{ 0.0f, 0.0f, 1.0f };
constexpr glm::vec3 gWorldRight{ 1.0f, 0.0f, 0.0f };
constexpr glm::vec3 gWorldUp{ 0.0f, -1.0f, 0.0f };

static std::unique_ptr<Level> gLoadedLevel{};
static std::unique_ptr<Level> gScheduledLevel{};

static float gWorldDeltaTime{ 0.0f };
static float gWorldGravityScale{ 0.05f };

const glm::vec3& World::GetWorldForwardVector()
{
	return gWorldForward;
}

const glm::vec3& World::GetWorldRightVector()
{
	return gWorldRight;
}

const glm::vec3& World::GetWorldUpVector()
{
	return gWorldUp;
}

Level& World::GetLoadedLevel()
{
	return *gLoadedLevel.get();
}

float World::GetWorldDeltaTime()
{
	return gWorldDeltaTime;
}

void World::SetWorldDeltaTime(const float deltaTime)
{
	gWorldDeltaTime = deltaTime;
}

float World::GetWorldGravityScale()
{
	return gWorldGravityScale;
}

void World::SetWorldGravityScale(float scale)
{
	gWorldGravityScale = scale;
}

bool World::LoadLevel(std::unique_ptr<Level>&& level, const bool force)
{
	gScheduledLevel = std::move(level);

	if (force)
	{
		return LoadScheduledLevel();
	}

	return true;
}

bool World::IslevelScheduled()
{
	return gScheduledLevel != nullptr;
}

bool World::LoadScheduledLevel()
{
	if (!Renderer::WaitForIdle())
	{
		return false;
	}

	if (gLoadedLevel != nullptr)
	{
		gLoadedLevel->UnLoad();
	}

	gLoadedLevel = std::move(gScheduledLevel);
	gScheduledLevel = nullptr;

	if (!gLoadedLevel->Load())
	{
		return false;
	}

	// Update renderer descriptors with loaded textures
	Renderer::UpdateDescriptorSets();

	// Begin the loaded level
	World::GetLoadedLevel().Begin();

	return true;
}
