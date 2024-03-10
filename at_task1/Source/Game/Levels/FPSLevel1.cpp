#include "Pch.h"
#include "FPSLevel1.h"
#include "Renderer/Renderer.h"
#include "Maths/Maths.h"

#include "Game/Tags.h"

#include "Game/Components/TransformComponent.h"
#include "Game/Components/StaticMeshComponent.h"
#include "Game/Components/SphereCollisionComponent.h"
#include "Game/Components/TagComponent.h"
#include "Game/Components/AABBCollisionComponent.h"

#include "Game/HUDS/FPSHUD.h"

bool FPSLevel1::Load()
{
    if (!Super::Load())
    {
        return false;
    }

    // Create fps hud class instance
    HUDClassInstance = std::make_unique<FPSHUD>();
    if (!HUDClassInstance->Load())
    {
        return false;
    }

    SetupPlayer({ 1.0f, 0.0f, -8.0f });
    AddFloor({ 100.0f, 100.0f }, { 50.0f, 50.0f });
    AddRoof({ 100.0f, 100.0f }, { 50.0f, 50.0f });
    AddWall({glm::vec3(0.0f, 0.0f, -10.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(8.0f, 1.0f, 0.5f)}, { 2.5f, 5.0f});
    AddWall({glm::vec3(4.0f, 0.0f, -7.0f), glm::vec3(0.0f, 90.0f, 0.0f), glm::vec3(8.0f, 1.0f, 0.5f)}, { 2.5f, 5.0f});
    AddWall({glm::vec3(-4.0f, 0.0f, -7.0f), glm::vec3(0.0f, 90.0f, 0.0f), glm::vec3(8.0f, 1.0f, 0.5f)}, {2.5f, 5.0f});
    AddWall({glm::vec3(-3.35f, 0.0f, -3.4f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(4.0f, 1.0f, 0.5f)}, {2.1f, 5.0f});
    AddWall({glm::vec3( 3.35f, 0.0f, -3.4f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(4.0f, 1.0f, 0.5f)}, {2.1f, 5.0f});
    AddWall({ glm::vec3(1.59f, 0.0f, 2.8f), glm::vec3(0.0f, 90.0f, 0.0f), glm::vec3(12.0f, 1.0f, 0.5f) }, { 1.5f, 5.0f });
    AddWall({ glm::vec3(-1.59f, 0.0f, 2.8f), glm::vec3(0.0f, 90.0f, 0.0f), glm::vec3(12.0f, 1.0f, 0.5f) }, { 1.5f, 5.0f });
    AddWall({ glm::vec3(-3.35f, 0.0f, 8.6f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(4.0f, 1.0f, 0.5f) }, { 2.1f, 5.0f });
    AddWall({ glm::vec3(5.5f, 0.0f, 8.5f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(8.0f, 1.0f, 0.5f) }, { 2.1f, 5.0f });
    AddWall({ glm::vec3(0.0f, 0.0f, 20.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(8.0f, 1.0f, 0.5f) }, { 2.5f, 5.0f });
    AddWall({ glm::vec3(-4.0f, 0.0f, 14.0f), glm::vec3(0.0f, 90.0f, 0.0f), glm::vec3(12.0f, 1.0f, 0.5f) }, { 2.1f, 5.0f });
    AddWall({ glm::vec3(3.35f, 0.0f, 16.0f), glm::vec3(0.0f, 90.0f, 0.0f), glm::vec3(8.0f, 1.0f, 0.5f) }, { 2.1f, 5.0f });
    AddWall({ glm::vec3(6.7f, 0.0f, 12.255f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(7.0f, 1.0f, 0.5f) }, { 2.5f, 5.0f });
    AddWall({ glm::vec3(14.0f, 0.0f, 8.5f), glm::vec3(0.0f, 90.0f, 0.0f), glm::vec3(16.0f, 1.0f, 0.5f) }, { 1.4f, 5.0f });
    AddWall({ glm::vec3(9.9f, 0.0f, 14.5f), glm::vec3(0.0f, 90.0f, 0.0f), glm::vec3(4.0f, 1.0f, 0.5f) }, { 1.4f, 5.0f });
    AddWall({ glm::vec3(12.0f, 0.0f, 21.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(10.0f, 1.0f, 0.5f) }, { 2.1f, 5.0f });
    AddWall({ glm::vec3(7.6f, 0.0f, 16.23f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(5.0f, 1.0f, 0.5f) }, { 2.1f, 5.0f });
    AddWall({ glm::vec3(16.6f, 0.0f, 16.23f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(5.0f, 1.0f, 0.5f) }, { 2.1f, 5.0f });
    AddWall({ glm::vec3(7.6f, 0.0f, 19.0f), glm::vec3(0.0f, 90.0f, 0.0f), glm::vec3(5.0f, 1.0f, 0.5f) }, { 2.1f, 5.0f });
    AddWall({ glm::vec3(16.6f, 0.0f, 19.0f), glm::vec3(0.0f, 90.0f, 0.0f), glm::vec3(5.0f, 1.0f, 0.5f) }, { 2.1f, 5.0f });
    AddWall({ glm::vec3(6.0f, 0.0f, 4.5f), glm::vec3(0.0f, 90.0f, 0.0f), glm::vec3(8.0f, 1.0f, 0.5f) }, { 2.1f, 5.0f });
    AddWall({ glm::vec3(10.0f, 0.0f, 1.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(10.0f, 1.0f, 0.5f) }, { 2.5f, 5.0f });

    AddEnemy({ 0.0f, 0.0f, 8.0f });
    AddEnemy({ 2.0f, 0.0f, 12.0f });
    AddEnemy({ 12.0f, 0.0f, 13.0f });

    AddLevelGoal({ 10.5f, 0.0f, 4.0f });

    AddBarrel({ 16.0f, 0.0f, 20.5f });
    AddBarrel({ 15.1f, 0.0f, 20.5f });
    AddBarrel({ 9.0f, 0.0f, 20.5f });
    AddBarrel({ 13.0f, 0.0f, 2.0f });

    return true;
}

void FPSLevel1::UnLoad()
{
    Super::UnLoad();
}

void FPSLevel1::Begin()
{
}

void FPSLevel1::Tick(const float deltaTime)
{
}

void FPSLevel1::TickFixed()
{
}
