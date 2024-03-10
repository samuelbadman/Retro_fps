#include "Pch.h"
#include "SoundSourceVoice.h"

void Audio::SoundSourceVoice::SubmitBuffer(const XAUDIO2_BUFFER& buffer)
{
	pSourceVoice->SubmitSourceBuffer(&buffer);
}
