#pragma once

#include "SoundSourceVoice.h"

class Level;

namespace Maths
{
	struct Transform;
}

namespace Audio
{
	struct Sound
	{
		BYTE* pDataBuffer{ nullptr };
		WAVEFORMATEXTENSIBLE WFX{};
		XAUDIO2_BUFFER Buffer{};
	};

	bool Init();
	void Shutdown();
	void SetListener(const Maths::Transform& transform, const glm::vec3& velocity);
	bool LoadSound(const std::string& filepath, bool looping, uint32_t* pID);
	// Destroys the sound at soundID and takes an array of sound source voices that must be stopped before the sound is destroyed
	void DestroySound(uint32_t soundID, SoundSourceVoice** pSoundSourceVoices, const uint32_t sourceVoiceCount);
	bool CreateSoundSourceVoice(const uint32_t soundID, SoundSourceVoice* pSoundSourceVoice);
	void Update3DEmitters(Level& level);
	const XAUDIO2_VOICE_DETAILS& GetMasteringVoiceDetails();
}