#pragma once

namespace GameState
{
	enum class EState : uint8_t
	{
		MAIN_MENU = 0,
		IN_GAME,
		WIN_SCREEN,
		LOSE_SCREEN
	};

	bool Init();
	void Shutdown();
	void Begin();
	void Tick(const float deltaTime);
	void LoadNextLevel();
	GameState::EState GetCurrentState();
	void SetCurrentState(const GameState::EState newState);
	void ResetCurrentLevel();
	void ResetPlayerHealth();
	// Applies damage to the player. Returns true if the player has run out of health
	bool DamagePlayer(const float amount);
}