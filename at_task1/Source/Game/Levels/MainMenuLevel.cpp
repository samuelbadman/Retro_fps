#include "Pch.h"
#include "MainMenuLevel.h"
#include "Maths/Transform.h"

#include "Game/HUDs/MainMenuHUD.h"

#include "Game/GameAudio.h"
#include "Game/GameState.h"

bool MainMenuLevel::Load()
{
    if (!Super::Load())
    {
        return false;
    }

    // Create main menu hud instance
    HUDClassInstance = std::make_unique<MainMenuHUD>();
    if (!HUDClassInstance->Load())
    {
        return false;
    }

    AddFloor({ 50.0f, 50.0f }, { 25.0f, 25.0f });
    AddWall({ {0.0f, 0.0f, 8.0f }, {0.0f, 0.0f, 0.0f}, {15.0f, 1.0f, 1.0f} }, { 1.5f, 5.0f });
    AddRoof({ 50.0f, 50.0f }, { 25.0f, 25.0f });
    AddLevelGoal({ 2.0f, 0.0f, 5.5f });

    return true;
}

void MainMenuLevel::UnLoad()
{
    Super::UnLoad();
    GameAudio::StopMainMenuLoop();
}

void MainMenuLevel::Begin()
{
    GameAudio::StopBackgroundTrack();
    GameState::SetCurrentState(GameState::EState::MAIN_MENU);
    GameState::ResetCurrentLevel();
    GameState::ResetPlayerHealth();
    GameAudio::StartMainMenuLoop();
}

void MainMenuLevel::Tick(const float deltaTime)
{
}

void MainMenuLevel::TickFixed()
{
}
