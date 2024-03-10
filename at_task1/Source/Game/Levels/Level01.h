#pragma once

#include "Game/Level.h"
#include "Audio/Audio.h"

class Level01 : public Level
{
public:
	bool Load() final;
	void UnLoad() final;
	void Begin() final;
	void Tick(const float deltaTime) final;
	void TickFixed() final;

private:
	static constexpr float MonsterDeathSoundVolume{ 15.0f };

	uint32_t CubeGeometryID{ std::numeric_limits<uint32_t>::max() };
	uint32_t PlaneGeometryID{ std::numeric_limits<uint32_t>::max() };
	uint32_t MonsterTextureID{ 0 };
	uint32_t WallTextureID{ 0 };
	uint32_t FloorTextureID{ 0 };
	uint32_t MonsterDeathSoundID{ std::numeric_limits<uint32_t>::max() };

	std::array<Audio::SoundSourceVoice*, 2>  enemySoundSourceVoices;

	Entity PlayerEntity;
	std::vector<Entity> EnemyEntities;
};