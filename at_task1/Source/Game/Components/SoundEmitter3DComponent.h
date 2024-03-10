#pragma once

#include "Audio/Audio.h"

struct SoundEmitter3DComponent
{
	SoundEmitter3DComponent()
	{
		const auto numChannels = Audio::GetMasteringVoiceDetails().InputChannels;

		DSPMatrix.resize(numChannels);
		DSPSettings.SrcChannelCount = 1;
		DSPSettings.DstChannelCount = numChannels;
		DSPSettings.pMatrixCoefficients = DSPMatrix.data();

		Emitter.ChannelCount = 1;
		Emitter.CurveDistanceScaler = 1.0f;
	}

	X3DAUDIO_EMITTER Emitter{};
	Audio::SoundSourceVoice SoundSourceVoice{};
	X3DAUDIO_DSP_SETTINGS DSPSettings{};
	std::vector<FLOAT32> DSPMatrix;
};