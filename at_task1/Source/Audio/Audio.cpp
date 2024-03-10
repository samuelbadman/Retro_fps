#include "Pch.h"
#include "Audio.h"
#include "Maths/Maths.h"
#include "Game/Level.h"
#include "Audio/SoundSourceVoice.h"

#include "Game/World.h"
#include "Game/Components/SoundEmitter3DComponent.h"
#include "Game/Components/TransformComponent.h"
#include "Game/Components/RigidBodyComponent.h"

#define fourccRIFF 'FFIR'
#define fourccDATA 'atad'
#define fourccFMT ' tmf'
#define fourccWAVE 'EVAW'
#define fourccXWMA 'AMWX'
#define fourccDPDS 'sdpd'

static Microsoft::WRL::ComPtr<IXAudio2> gXAudio2;
static IXAudio2MasteringVoice* gpMasteringVoice{ nullptr };
static X3DAUDIO_HANDLE gX3DAudio{ NULL };
static X3DAUDIO_LISTENER gListener{};
static XAUDIO2_VOICE_DETAILS gMasteringVoiceDetails{};

constexpr size_t MAX_SOUND_BUFFER_COUNT{ 32 };
static std::array<Audio::Sound, MAX_SOUND_BUFFER_COUNT> gLoadedSounds;
static std::queue<uint32_t> gAvailableSoundIDs;
static std::vector<uint32_t> gUsedSoundIDs;

static HRESULT FindChunk(HANDLE hFile, DWORD fourcc, DWORD& dwChunkSize, DWORD& dwChunkDataPosition)
{
	HRESULT hr = S_OK;
	if (INVALID_SET_FILE_POINTER == SetFilePointer(hFile, 0, NULL, FILE_BEGIN))
		return HRESULT_FROM_WIN32(GetLastError());

	DWORD dwChunkType;
	DWORD dwChunkDataSize;
	DWORD dwRIFFDataSize = 0;
	DWORD dwFileType;
	DWORD bytesRead = 0;
	DWORD dwOffset = 0;

	while (hr == S_OK)
	{
		DWORD dwRead;
		if (0 == ReadFile(hFile, &dwChunkType, sizeof(DWORD), &dwRead, NULL))
			hr = HRESULT_FROM_WIN32(GetLastError());

		if (0 == ReadFile(hFile, &dwChunkDataSize, sizeof(DWORD), &dwRead, NULL))
			hr = HRESULT_FROM_WIN32(GetLastError());

		switch (dwChunkType)
		{
		case fourccRIFF:
			dwRIFFDataSize = dwChunkDataSize;
			dwChunkDataSize = 4;
			if (0 == ReadFile(hFile, &dwFileType, sizeof(DWORD), &dwRead, NULL))
				hr = HRESULT_FROM_WIN32(GetLastError());
			break;

		default:
			if (INVALID_SET_FILE_POINTER == SetFilePointer(hFile, dwChunkDataSize, NULL, FILE_CURRENT))
				return HRESULT_FROM_WIN32(GetLastError());
		}

		dwOffset += sizeof(DWORD) * 2;

		if (dwChunkType == fourcc)
		{
			dwChunkSize = dwChunkDataSize;
			dwChunkDataPosition = dwOffset;
			return S_OK;
		}

		dwOffset += dwChunkDataSize;

		if (bytesRead >= dwRIFFDataSize) return S_FALSE;

	}

	return S_OK;
}

static HRESULT ReadChunkData(HANDLE hFile, void* buffer, DWORD buffersize, DWORD bufferoffset)
{
	HRESULT hr = S_OK;
	if (INVALID_SET_FILE_POINTER == SetFilePointer(hFile, bufferoffset, NULL, FILE_BEGIN))
		return HRESULT_FROM_WIN32(GetLastError());
	DWORD dwRead;
	if (0 == ReadFile(hFile, buffer, buffersize, &dwRead, NULL))
		hr = HRESULT_FROM_WIN32(GetLastError());
	return hr;
}

bool Audio::Init()
{
	// Initialise XAudio2
	if (FAILED(XAudio2Create(&gXAudio2, 0, XAUDIO2_DEFAULT_PROCESSOR)))
	{
		return false;
	}

	// Create a mastering voice
	if (FAILED(gXAudio2->CreateMasteringVoice(&gpMasteringVoice)))
	{
		return false;
	}

	gpMasteringVoice->GetVoiceDetails(&gMasteringVoiceDetails);

	// Initialise X3DAudio
	DWORD dwChannelMask;
	gpMasteringVoice->GetChannelMask(&dwChannelMask);
	if (FAILED(X3DAudioInitialize(dwChannelMask, X3DAUDIO_SPEED_OF_SOUND, gX3DAudio)))
	{
		return false;
	}

	// Add available sound IDs
	for (uint32_t i = 0; i < MAX_SOUND_BUFFER_COUNT; ++i)
	{
		gAvailableSoundIDs.push(i);
	}

    return true;
}

void Audio::Shutdown()
{
	// Destroy all loaded sounds
	std::vector<uint32_t> usedIDs = gUsedSoundIDs;
	for(const auto& id : usedIDs)
	{
		Audio::DestroySound(id, nullptr, 0);
	}
}

void Audio::SetListener(const Maths::Transform& transform, const glm::vec3& velocity)
{
	// Calculate listener forward and up vectors
	auto listenerForward = glm::normalize(Maths::RotateVector(transform.Rotation, World::GetWorldForwardVector()));
	auto listenerUp = glm::normalize(Maths::RotateVector(transform.Rotation, World::GetWorldUpVector()));

	// Update listener properties
	gListener.OrientFront.x = listenerForward.x;
	gListener.OrientFront.y = listenerForward.y;
	gListener.OrientFront.z = listenerForward.z;
	gListener.OrientTop.x = listenerUp.x;
	gListener.OrientTop.y = -listenerUp.y; // Flip Y as Vulkan viewport is upside down
	gListener.OrientTop.z = listenerUp.z;
	gListener.Position.x = transform.Position.x;
	gListener.Position.y = transform.Position.y;
	gListener.Position.z = transform.Position.z;
	gListener.Velocity.x = velocity.x;
	gListener.Velocity.y = velocity.y;
	gListener.Velocity.z = velocity.z;
}

bool Audio::LoadSound(const std::string& filepath, bool looping, uint32_t* pID)
{
	// Get an available sound
	*pID = gAvailableSoundIDs.front();
	gAvailableSoundIDs.pop();
	auto& sound = gLoadedSounds[*pID];
	gUsedSoundIDs.push_back(*pID);

	// Load the riff
	auto file = CreateFile(std::wstring(filepath.begin(), filepath.end()).c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);

	if (INVALID_HANDLE_VALUE == file)
	{
		return false;
	}

	if (INVALID_SET_FILE_POINTER == SetFilePointer(file, 0, NULL, FILE_BEGIN))
	{
		return false;
	}

	DWORD dwChunkSize;
	DWORD dwChunkPosition;
	FindChunk(file, fourccRIFF, dwChunkSize, dwChunkPosition);
	DWORD filetype;
	ReadChunkData(file, &filetype, sizeof(DWORD), dwChunkPosition);
	if (filetype != fourccWAVE)
	{
		return false;
	}

	FindChunk(file, fourccFMT, dwChunkSize, dwChunkPosition);
	ReadChunkData(file, &sound.WFX, dwChunkSize, dwChunkPosition);

	FindChunk(file, fourccDATA, dwChunkSize, dwChunkPosition);
	sound.pDataBuffer = new BYTE[dwChunkSize];
	ReadChunkData(file, sound.pDataBuffer, dwChunkSize, dwChunkPosition);

	sound.Buffer.AudioBytes = dwChunkSize;
	sound.Buffer.pAudioData = sound.pDataBuffer;
	sound.Buffer.Flags = XAUDIO2_END_OF_STREAM;

	if (looping)
	{
		sound.Buffer.LoopCount = XAUDIO2_LOOP_INFINITE;
	}

	return true;
}

void Audio::DestroySound(uint32_t soundID, SoundSourceVoice** pSoundSourceVoices, const uint32_t sourceVoiceCount)
{
	for (uint32_t i = 0; i < sourceVoiceCount; ++i)
	{
		(*pSoundSourceVoices)->GetSourceVoice()->Stop();
	}

	auto& loadedSound = gLoadedSounds[soundID];
	delete[] loadedSound.pDataBuffer;
	loadedSound.pDataBuffer = nullptr;
	gUsedSoundIDs.erase(std::find(gUsedSoundIDs.begin(), gUsedSoundIDs.end(), soundID));
	gAvailableSoundIDs.push(soundID);
}

bool Audio::CreateSoundSourceVoice(const uint32_t soundID, SoundSourceVoice* pSoundSourceVoice)
{
	auto& sound = gLoadedSounds[soundID];

	if (FAILED(gXAudio2->CreateSourceVoice(pSoundSourceVoice->GetSourceVoiceAddress(), reinterpret_cast<WAVEFORMATEX*>(&sound.WFX))))
	{
		return false;
	}

	pSoundSourceVoice->SubmitBuffer(sound.Buffer);

	return true;
}

void Audio::Update3DEmitters(Level& level)
{
	// Get the level's ecs registry
	auto& ecsRegistry = level.GetECSRegistry();

	// Create a view of entities with sound emitter 3D and transform components
	auto emitterView = ecsRegistry.view<TransformComponent, SoundEmitter3DComponent>();

	// For each entity in the view
	for (auto [entity, transform, soundEmitter3D] : emitterView.each())
	{
		// Calculate emitter forward and up vector
		auto emitterForward = glm::normalize(Maths::RotateVector(transform.Transform.Rotation, World::GetWorldForwardVector()));
		auto emitterUp = glm::normalize(Maths::RotateVector(transform.Transform.Rotation, World::GetWorldUpVector()));

		// Update the emitter orientation
		soundEmitter3D.Emitter.OrientFront.x = emitterForward.x;
		soundEmitter3D.Emitter.OrientFront.y = emitterForward.y;
		soundEmitter3D.Emitter.OrientFront.z = emitterForward.z;
		soundEmitter3D.Emitter.OrientTop.x = emitterUp.x;
		soundEmitter3D.Emitter.OrientTop.y = emitterUp.y;
		soundEmitter3D.Emitter.OrientTop.z = emitterUp.z;
		soundEmitter3D.Emitter.Position.x = transform.Transform.Position.x;
		soundEmitter3D.Emitter.Position.y = transform.Transform.Position.y;
		soundEmitter3D.Emitter.Position.z = transform.Transform.Position.z;

		// Check if the emitter entity has a velocity
		if (ecsRegistry.any_of<RigidBodyComponent>(entity))
		{
			// Get the emitter's velocity
			const auto& velocity = ecsRegistry.get<RigidBodyComponent>(entity).Velocity;

			// Update the emitter's velocity
			soundEmitter3D.Emitter.Velocity.x = velocity.x;
			soundEmitter3D.Emitter.Velocity.y = velocity.y;
			soundEmitter3D.Emitter.Velocity.z = velocity.z;
		}
		else
		{
			// The emitter does not have velocity
			soundEmitter3D.Emitter.Velocity = X3DAUDIO_VECTOR(0.0f, 0.0f, 0.0f);
		}

		// Calculate 3D sound properties
		X3DAudioCalculate(gX3DAudio, &gListener, &soundEmitter3D.Emitter,
			X3DAUDIO_CALCULATE_MATRIX | X3DAUDIO_CALCULATE_DOPPLER | X3DAUDIO_CALCULATE_LPF_DIRECT | X3DAUDIO_CALCULATE_REVERB,
			&soundEmitter3D.DSPSettings);

		// Apply 3D sound properties to sound emitter source voice
		auto* pEmitterSourceVoice = soundEmitter3D.SoundSourceVoice.GetSourceVoice();

		if (FAILED(pEmitterSourceVoice->SetOutputMatrix(gpMasteringVoice, 1, Audio::GetMasteringVoiceDetails().InputChannels, soundEmitter3D.DSPSettings.pMatrixCoefficients)))
		{
			assert(false && "Failed to set emitter source voice output matrix.");
		}

		if (FAILED(pEmitterSourceVoice->SetFrequencyRatio(soundEmitter3D.DSPSettings.DopplerFactor)))
		{
			assert(false && "Failed to set emitter source voice frequency ratio.");
		}

		XAUDIO2_FILTER_PARAMETERS FilterParameters = { LowPassFilter, 2.0f * sinf(X3DAUDIO_PI / 6.0f * soundEmitter3D.DSPSettings.LPFDirectCoefficient), 1.0f };
		pEmitterSourceVoice->SetFilterParameters(&FilterParameters);
	}
}

const XAUDIO2_VOICE_DETAILS& Audio::GetMasteringVoiceDetails()
{
	return gMasteringVoiceDetails;
}
