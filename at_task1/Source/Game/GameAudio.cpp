#include "Pch.h"
#include "GameAudio.h"
#include "Audio/Audio.h"

constexpr size_t GUNSHOT_SOUND_COUNT{ 6 };
constexpr float GUNSHOT_SOUND_VOLUME{ 1.2f };
constexpr float BACKGROUND_TRACK_VOLUME{ 1.0f };
constexpr float MAIN_MENU_LOOP_VOLUME{ 0.5f };
constexpr float INJURED_SOUND_VOLUME{ 6.0f };
constexpr float ENEMY_ATTACK_SOUND_VOLUME{ 1.0f };

static std::vector<uint32_t> gGunshotSoundIDs;

static uint32_t gBackgroundTrackID{ 0 };
static Audio::SoundSourceVoice gBackgroundTrackSourceVoice{};

uint32_t gMainMenuLoopID{ 0 };
Audio::SoundSourceVoice gMainMenuLoopSourceVoice{};

uint32_t gInjuredSoundID{ 0 };

uint32_t gEnemyAttackSoundID{ 0 };

bool GameAudio::Init()
{
	// Load gunshot sounds
	gGunshotSoundIDs.resize(GUNSHOT_SOUND_COUNT);

	for (int32_t i = 0; i < GUNSHOT_SOUND_COUNT; ++i)
	{
		auto& gunshotSoundID = gGunshotSoundIDs[0];

		if(!Audio::LoadSound("Assets/Game/Sounds/pistol0" + std::to_string(i + 1) + ".wav", false, &gunshotSoundID))
		{
			return false;
		}
	}

	// Load background track
	if (!Audio::LoadSound("Assets/Game/Sounds/carmack.wav", true, &gBackgroundTrackID))
	{
		return false;
	}

	if (!Audio::CreateSoundSourceVoice(gBackgroundTrackID, &gBackgroundTrackSourceVoice))
	{
		return false;
	}

	gBackgroundTrackSourceVoice.GetSourceVoice()->SetVolume(BACKGROUND_TRACK_VOLUME);

	// Load main menu loop
	if (!Audio::LoadSound("Assets/Game/Sounds/main_menu_loop.wav", true, &gMainMenuLoopID))
	{
		return false;
	}

	if (!Audio::CreateSoundSourceVoice(gMainMenuLoopID, &gMainMenuLoopSourceVoice))
	{
		return false;
	}

	gMainMenuLoopSourceVoice.GetSourceVoice()->SetVolume(MAIN_MENU_LOOP_VOLUME);

	// Load injured sound
	if (!Audio::LoadSound("Assets/Game/Sounds/injured_grunt.wav", false, &gInjuredSoundID))
	{
		return false;
	}

	// Load enemy attack sound
	if (!Audio::LoadSound("Assets/Game/Sounds/enemy_attack.wav", false, &gEnemyAttackSoundID))
	{
		return false;
	}

	return true;
}

void GameAudio::Shutdown()
{
	StopBackgroundTrack();
	StopMainMenuLoop();
}

void GameAudio::StartBackgroundTrack()
{
	if (!Audio::CreateSoundSourceVoice(gBackgroundTrackID, &gBackgroundTrackSourceVoice))
	{
		assert(false && "Failed to create sound source voice when starting background track.");
	}
	gBackgroundTrackSourceVoice.GetSourceVoice()->Start();
}

void GameAudio::StopBackgroundTrack()
{
	gBackgroundTrackSourceVoice.GetSourceVoice()->Stop();
	gBackgroundTrackSourceVoice.GetSourceVoice()->FlushSourceBuffers();
}

void GameAudio::PlayRandomGunshotSound()
{
	// Play a random gunshot sound
	Audio::SoundSourceVoice sourceVoice;
	Audio::CreateSoundSourceVoice(gGunshotSoundIDs[glm::linearRand(0, static_cast<int32_t>(GUNSHOT_SOUND_COUNT) - 1)], &sourceVoice);
	auto* xAudioSourceVoice = sourceVoice.GetSourceVoice();
	xAudioSourceVoice->SetVolume(GUNSHOT_SOUND_VOLUME);
	xAudioSourceVoice->Start();
}

void GameAudio::StartMainMenuLoop()
{
	if (!Audio::CreateSoundSourceVoice(gMainMenuLoopID, &gMainMenuLoopSourceVoice))
	{
		assert(false && "Failed to create sound source voice when starting main menu loop track.");
	}
	gMainMenuLoopSourceVoice.GetSourceVoice()->Start();
}

void GameAudio::StopMainMenuLoop()
{
	gMainMenuLoopSourceVoice.GetSourceVoice()->Stop();
	gMainMenuLoopSourceVoice.GetSourceVoice()->FlushSourceBuffers();
}

void GameAudio::PlayInjuredSound()
{
	Audio::SoundSourceVoice sourceVoice;
	Audio::CreateSoundSourceVoice(gInjuredSoundID, &sourceVoice);
	auto* xAudioSourceVoice = sourceVoice.GetSourceVoice();
	xAudioSourceVoice->SetVolume(INJURED_SOUND_VOLUME);
	xAudioSourceVoice->Start();
}

void GameAudio::PlayEnemyAttackSound()
{
	Audio::SoundSourceVoice sourceVoice;
	Audio::CreateSoundSourceVoice(gEnemyAttackSoundID, &sourceVoice);
	auto* xAudioSourceVoice = sourceVoice.GetSourceVoice();
	xAudioSourceVoice->SetVolume(ENEMY_ATTACK_SOUND_VOLUME);
	xAudioSourceVoice->Start();
}
