#pragma once

namespace GameAudio
{
	bool Init();
	void Shutdown();
	void StartBackgroundTrack();
	void StopBackgroundTrack();
	void PlayRandomGunshotSound();
	void StartMainMenuLoop();
	void StopMainMenuLoop();
	void PlayInjuredSound();
	void PlayEnemyAttackSound();
}