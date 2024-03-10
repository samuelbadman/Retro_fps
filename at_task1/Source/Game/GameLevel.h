#pragma once

#include "Level.h"

namespace Maths
{
	struct Transform;
}

namespace Audio
{
	class SoundSourceVoice;
}

class GameLevel : public Level
{
public:
	using Super = GameLevel;

	bool Load() override;
	void UnLoad() override;

protected:
	bool SetupPlayer(const glm::vec3& startPosition);
	const Entity& GetPlayerEntity() const { return PlayerEntity; }
	Entity& GetPlayerEntity() { return PlayerEntity; }
	void AddFloor(const glm::vec2& scale, const glm::vec2& textureScale);
	void AddRoof(const glm::vec2& scale, const glm::vec2& textureScale);
	void AddWall(const Maths::Transform& transform, const glm::vec2& textureScale);
	void AddEnemy(const glm::vec3& position);
	void AddLevelGoal(const glm::vec3& position);
	void AddBarrel(const glm::vec3& position);

private:
	static constexpr float FloorYPosition{ 1.5f };
	static constexpr float RoofYPosition{ -1.5f };
	static constexpr float WallYPosition{ 0.0f };
	static constexpr float WallYScale{ 3.0f };
	static constexpr float EnemyYPosition{ 0.75f };
	static constexpr float MonsterDeathSoundVolume{ 15.0f };
	static constexpr float LevelGoalYPosition{ 1.2f };
	static constexpr float EnemyAttackRadius{ 3.0f };
	static constexpr float LevelGoalDecorationYPosition{ -1.5f };
	static constexpr float BarrelYPosition{ 1.05f };

	Entity PlayerEntity{};
	uint32_t CubeGeometryID{ std::numeric_limits<uint32_t>::max() };
	uint32_t PlaneGeometryID{ std::numeric_limits<uint32_t>::max() };
	uint32_t MonsterTextureID{ 0 };
	uint32_t WallTextureID{ 0 };
	uint32_t FloorTextureID{ 0 };
	uint32_t MonsterDeathSoundID{ std::numeric_limits<uint32_t>::max() };
	std::vector<Audio::SoundSourceVoice*>  enemySoundSourceVoices;
	uint32_t CylinderGeometryID{ std::numeric_limits<uint32_t>::max() };
	uint32_t LevelGoalTextureID{ 0 };
	uint32_t PowerCellTextureID{ 0 };
	uint32_t BarrelTextureID{ 0 };
};