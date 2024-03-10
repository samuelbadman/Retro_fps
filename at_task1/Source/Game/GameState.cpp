#include "Pch.h"
#include "GameState.h"
#include "GameAudio.h"
#include "World.h"

#include "Game/Levels/FPSLevel1.h"
#include "Game/Levels/FPSLevel2.h"

#include "Game/HUDs/FPSHUD.h"

static uint32_t CurrentLevel{ 0 };
static GameState::EState CurrentState{ GameState::EState::MAIN_MENU };
static float PlayerHealth{ 100.0f };

bool GameState::Init()
{
	// Init game audio
	return GameAudio::Init();
}

void GameState::Shutdown()
{
	GameAudio::Shutdown();
}

void GameState::Begin()
{

}

void GameState::Tick(const float deltaTime)
{
}

void GameState::LoadNextLevel()
{
	++CurrentLevel;
	switch (CurrentLevel)
	{
	case 1:
		World::LoadLevel(std::make_unique<FPSLevel1>(), false);
		break;

	case 2:
		World::LoadLevel(std::make_unique<FPSLevel2>(), false);
		break;

	case 3:
	{
		// Show the game complete screen
		auto& loadedLevel = World::GetLoadedLevel();

		auto* pFPSHUD = dynamic_cast<FPSHUD*>(loadedLevel.GetHUDClassInstance());
		assert(pFPSHUD != nullptr && "Could not cast to FPSHUD from loaded level HUD class instance.");

		pFPSHUD->ShowGameCompleteScreen();

		// Set game state to win screen
		GameState::SetCurrentState(GameState::EState::WIN_SCREEN);

		break;
	}
	}
}

GameState::EState GameState::GetCurrentState()
{
	return CurrentState;
}

void GameState::SetCurrentState(const GameState::EState newState)
{
	CurrentState = newState;
}

void GameState::ResetCurrentLevel()
{
	CurrentLevel = 0;
}

void GameState::ResetPlayerHealth()
{
	PlayerHealth = 100.0f;
}

bool GameState::DamagePlayer(const float amount)
{
	PlayerHealth -= amount;
	return PlayerHealth <= 0.0f;
}
