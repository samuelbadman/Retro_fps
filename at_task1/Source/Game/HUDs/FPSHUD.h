#pragma once

#include "Game/HUD.h"

struct StaticMeshComponent;

class FPSHUD : public HUD
{
	enum class PromptIcons : uint8_t
	{
		PLAYSTATION,
		XBOX
	};

public:
	bool Load() final;
	void UnLoad() final;
	void Begin() final;
	void Tick(const float deltaTime) final;
	void SetInteractPromptVisible(const bool visible);
	void ShowGameCompleteScreen();
	void ShowPlayerDefeatedScreen();

private:
	static constexpr PromptIcons promptIcons{ PromptIcons::XBOX };

	uint32_t PlayerArmsTextureID{ 0 };
	uint32_t CrosshairTextureID{ 0 };
	uint32_t PromptTextureID{ 0 };
	uint32_t PlayerDefeatedScreenTexture{ 0 };
	uint32_t GameCompleteScreenTexture{ 0 };
	StaticMeshComponent* pInteractPromptStaticMeshComponent{ nullptr };
	StaticMeshComponent* pGameCompleteScreenStaticMeshComponent{ nullptr };
	StaticMeshComponent* pPlayerDefeatedScreenStaticMeshComponent{ nullptr };
};