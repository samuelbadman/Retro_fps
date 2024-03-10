#pragma once

#include "Game/GameLevel.h"

class FPSLevel2 : public GameLevel
{
public:
	bool Load() final;
	void UnLoad() final;
	void Begin() final;
	void Tick(const float deltaTime) final;
	void TickFixed() final;

private:

};