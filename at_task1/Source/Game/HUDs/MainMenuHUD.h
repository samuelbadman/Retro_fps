#pragma once

#include "Game/HUD.h"

class MainMenuHUD : public HUD
{
public:
	bool Load() final;
	void UnLoad() final;
	void Begin() final;
	void Tick(const float deltaTime) final;

private:
	uint32_t MainMenuTexture{ 0 };
};