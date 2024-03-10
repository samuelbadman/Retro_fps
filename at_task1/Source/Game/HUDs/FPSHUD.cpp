#include "Pch.h"
#include "FPSHUD.h"
#include "Renderer/Renderer.h"

#include "Game/Components/TransformComponent.h"
#include "Game/Components/StaticMeshComponent.h"

#include "Game/GameState.h"

bool FPSHUD::Load()
{
	if (!Super::Load())
	{
		return false;
	}

	// Load player arms texture
	if (!Renderer::LoadTexture("Assets/Game/Textures/player_arms.png", true, &PlayerArmsTextureID))
	{
		return false;
	}

	// Load crosshairs texture
	if (!Renderer::LoadTexture("Assets/Game/Textures/crosshair.png", false, &CrosshairTextureID))
	{
		return false;
	}

	// Load prompt texture
	std::string promptTextureAssetPath = "Assets/Game/Textures/";

	switch (promptIcons)
	{
	case PromptIcons::PLAYSTATION:
		promptTextureAssetPath += "playstation_prompt.png";
		break;

	case PromptIcons::XBOX:
		promptTextureAssetPath += "xbox_prompt.png";
		break;
	}

	if (!Renderer::LoadTexture(promptTextureAssetPath, false, &PromptTextureID))
	{
		return false;
	}

	// Load game complete texture
	if (!Renderer::LoadTexture("Assets/Game/Textures/game_complete_screen.png", false, &GameCompleteScreenTexture))
	{
		return false;
	}

	// Load player defeated texture
	if (!Renderer::LoadTexture("Assets/Game/Textures/player_defeated_screen.png", false, &PlayerDefeatedScreenTexture))
	{
		return false;
	}

	// Set the HUD camera's projection mode to orthographic
	HUDCameraSettings.ProjectionMode = Renderer::EProjectionMode::ORTHOGRAPHIC;

	// Crosshair image
	auto* pCrosshairImageEntity = CreateHUDEntity();

	auto& crosshairTransformComponent = pCrosshairImageEntity->AddComponent<TransformComponent>();
	crosshairTransformComponent.Transform.Position = { 0.0f, 0.0f, 2.0f };
	crosshairTransformComponent.Transform.Rotation = { 0.0f, 0.0f, 0.0f };
	crosshairTransformComponent.Transform.Scale = { 0.3f, 0.3f, 1.0f };

	auto& crosshairStaticMeshComponent = pCrosshairImageEntity->AddComponent<StaticMeshComponent>();
	crosshairStaticMeshComponent.GeometryID = ImageGeometryID;
	crosshairStaticMeshComponent.Material.SetSamplerID(Renderer::ESampler::NEAREST_NEIGHBOUR_FILTER);
	crosshairStaticMeshComponent.Material.AlphaBlended = true;
	crosshairStaticMeshComponent.Material.TextureID = CrosshairTextureID;

	// Player arms image
	auto* pPlayerArmsImageEntity = CreateHUDEntity();

	auto& playerArmsTransformComponent = pPlayerArmsImageEntity->AddComponent<TransformComponent>();
	playerArmsTransformComponent.Transform.Position = { 4.0f, 5.0f, 1.0f };
	playerArmsTransformComponent.Transform.Rotation = { 0.0f, 0.0f, 0.0f };
	playerArmsTransformComponent.Transform.Scale = { 8.0f, 8.0f, 1.0f };

	auto& playerArmsStaticMeshComponent = pPlayerArmsImageEntity->AddComponent<StaticMeshComponent>();
	playerArmsStaticMeshComponent.GeometryID = ImageGeometryID;
	playerArmsStaticMeshComponent.Material.SetSamplerID(Renderer::ESampler::NEAREST_NEIGHBOUR_FILTER);
	playerArmsStaticMeshComponent.Material.AlphaBlended = true;
	playerArmsStaticMeshComponent.Material.TextureID = PlayerArmsTextureID;

	// Interact prompt image
	auto* pInteractPromptImageEntity = CreateHUDEntity();

	auto& interactPromptTransformComponent = pInteractPromptImageEntity->AddComponent<TransformComponent>();
	interactPromptTransformComponent.Transform.Position = { 0.0f, 5.0f, 0.0f };
	interactPromptTransformComponent.Transform.Scale = { 2.0f, 2.0f, 1.0f };

	pInteractPromptStaticMeshComponent = &pInteractPromptImageEntity->AddComponent<StaticMeshComponent>();
	pInteractPromptStaticMeshComponent->GeometryID = ImageGeometryID;
	pInteractPromptStaticMeshComponent->Material.AlphaBlended = true;
	pInteractPromptStaticMeshComponent->Material.SetSamplerID(Renderer::ESampler::LINEAR_FILTER);
	pInteractPromptStaticMeshComponent->Material.TextureID = PromptTextureID;
	pInteractPromptStaticMeshComponent->Visible = false;

	// Game complete image
	auto* pGameCompleteImageEntity = CreateHUDEntity();

	auto& GameCompleteImageTransformComponent = pGameCompleteImageEntity->AddComponent<TransformComponent>();
	GameCompleteImageTransformComponent.Transform.Position = { 0.0f, -0.2f, 2.0f };
	GameCompleteImageTransformComponent.Transform.Rotation = { 0.0f, 0.0f, 0.0f };
	GameCompleteImageTransformComponent.Transform.Scale = { 28.0f, 27.0f, 1.0f };

	pGameCompleteScreenStaticMeshComponent = &pGameCompleteImageEntity->AddComponent<StaticMeshComponent>();
	pGameCompleteScreenStaticMeshComponent->GeometryID = ImageGeometryID;
	pGameCompleteScreenStaticMeshComponent->Material.SetSamplerID(Renderer::ESampler::NEAREST_NEIGHBOUR_FILTER);
	pGameCompleteScreenStaticMeshComponent->Material.AlphaBlended = true;
	pGameCompleteScreenStaticMeshComponent->Material.TextureID = GameCompleteScreenTexture;
	pGameCompleteScreenStaticMeshComponent->Visible = false;

	// Player defeated image
	auto* pPlayerDefeatedImageEntity = CreateHUDEntity();

	auto& PlayerDefeatedImageTransformComponent = pPlayerDefeatedImageEntity->AddComponent<TransformComponent>();
	PlayerDefeatedImageTransformComponent.Transform.Position = { 0.0f, -0.2f, 2.0f };
	PlayerDefeatedImageTransformComponent.Transform.Rotation = { 0.0f, 0.0f, 0.0f };
	PlayerDefeatedImageTransformComponent.Transform.Scale = { 28.0f, 27.0f, 1.0f };

	pPlayerDefeatedScreenStaticMeshComponent = &pPlayerDefeatedImageEntity->AddComponent<StaticMeshComponent>();
	pPlayerDefeatedScreenStaticMeshComponent->GeometryID = ImageGeometryID;
	pPlayerDefeatedScreenStaticMeshComponent->Material.SetSamplerID(Renderer::ESampler::NEAREST_NEIGHBOUR_FILTER);
	pPlayerDefeatedScreenStaticMeshComponent->Material.AlphaBlended = true;
	pPlayerDefeatedScreenStaticMeshComponent->Material.TextureID = PlayerDefeatedScreenTexture;
	pPlayerDefeatedScreenStaticMeshComponent->Visible = false;

    return true;
}

void FPSHUD::UnLoad()
{
	Super::UnLoad();

	Renderer::DestroyTexture(PlayerArmsTextureID);
	Renderer::DestroyTexture(CrosshairTextureID);
	Renderer::DestroyTexture(PromptTextureID);
	Renderer::DestroyTexture(GameCompleteScreenTexture);
	Renderer::DestroyTexture(PlayerDefeatedScreenTexture);
}

void FPSHUD::Begin()
{
}

void FPSHUD::Tick(const float deltaTime)
{
}

void FPSHUD::SetInteractPromptVisible(const bool visible)
{
	pInteractPromptStaticMeshComponent->Visible = visible;
}

void FPSHUD::ShowGameCompleteScreen()
{
	pGameCompleteScreenStaticMeshComponent->Visible = true;
}

void FPSHUD::ShowPlayerDefeatedScreen()
{
	pPlayerDefeatedScreenStaticMeshComponent->Visible = true;
}
