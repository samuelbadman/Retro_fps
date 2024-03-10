#pragma once

namespace Audio
{
	class SoundSourceVoice
	{
	public:
		void SubmitBuffer(const XAUDIO2_BUFFER& buffer);
		IXAudio2SourceVoice* GetSourceVoice() const { return pSourceVoice; }
		IXAudio2SourceVoice** GetSourceVoiceAddress() { return &pSourceVoice; }

	private:
		IXAudio2SourceVoice* pSourceVoice{ nullptr };
	};
}