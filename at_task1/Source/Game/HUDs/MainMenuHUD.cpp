#include "Pch.h"
#include "MainMenuHUD.h"
#include "Renderer/Renderer.h"

#include "Game/Components/TransformComponent.h"
#include "Game/Components/StaticMeshComponent.h"

bool MainMenuHUD::Load()
{
	if (!Super::Load())
	{
		return false;
	}

	// Load main menu texture
	if (!Renderer::LoadTexture("Assets/Game/Textures/main_menu.png", false, &MainMenuTexture))
	{
		return false;
	}

	// Create image entity
	auto* pMainMenuImageEntity = CreateHUDEntity();

	auto& mainMenuImageTransformComponent = pMainMenuImageEntity->AddComponent<TransformComponent>();
	mainMenuImageTransformComponent.Transform.Position = { 0.0f, -0.2f, 2.0f };
	mainMenuImageTransformComponent.Transform.Rotation = { 0.0f, 0.0f, 0.0f };
	mainMenuImageTransformComponent.Transform.Scale = { 3.0f, 2.0f, 1.0f };

	auto& mainMenuImageStaticMeshComponent = pMainMenuImageEntity->AddComponent<StaticMeshComponent>();
	mainMenuImageStaticMeshComponent.GeometryID = ImageGeometryID;
	mainMenuImageStaticMeshComponent.Material.SetSamplerID(Renderer::ESampler::NEAREST_NEIGHBOUR_FILTER);
	mainMenuImageStaticMeshComponent.Material.AlphaBlended = true;
	mainMenuImageStaticMeshComponent.Material.TextureID = MainMenuTexture;

    return true;
}

void MainMenuHUD::UnLoad()
{
	Super::UnLoad();

	Renderer::DestroyTexture(MainMenuTexture);
}

void MainMenuHUD::Begin()
{
}

void MainMenuHUD::Tick(const float deltaTime)
{
}
