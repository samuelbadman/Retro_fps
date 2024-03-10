#include "Pch.h"
#include "Level01.h"
#include "Renderer/Renderer.h"
#include "EventSystem/EventSystem.h"
#include "Maths/Maths.h"
#include "Console.h"

#include "Game/Tags.h"
#include "Game/GameAudio.h"
#include "Game/World.h"
#include "Game/Components/TransformComponent.h"
#include "Game/Components/StaticMeshComponent.h"
#include "Game/Components/CameraComponent.h"
#include "Game/Components/SphereCollisionComponent.h"
#include "Game/Components/AABBCollisionComponent.h"
#include "Game/Components/RigidBodyComponent.h"
#include "Game/Components/BillboardComponent.h"
#include "Game/Components/SoundEmitter3DComponent.h"
#include "Game/Components/TagComponent.h"

#include "Game/CollisionDetection.h"

#include "Game/HUDs/FPSHUD.h"

bool Level01::Load()
{
    // Create level hud class instance
    HUDClassInstance = std::make_unique<FPSHUD>();
    if (!HUDClassInstance->Load())
    {
        return false;
    }

    // Load assets
    if (!Renderer::LoadCubeGeometryPrimitive(1.0f, &CubeGeometryID))
    {
        return false;
    }

    if (!Renderer::LoadPlaneGeometryPrimitive(1.0f, &PlaneGeometryID))
    {
        return false;
    }

    if (!Renderer::LoadTexture("Assets/Game/Textures/monster.png", true, &MonsterTextureID))
    {
        return false;
    }

    if (!Renderer::LoadTexture("Assets/Game/Textures/wall.png", true, &WallTextureID))
    {
        return false;
    }

    if (!Renderer::LoadTexture("Assets/Game/Textures/floor.png", true, &FloorTextureID))
    {
        return false;
    }

    if (!Audio::LoadSound("Assets/Game/Sounds/monsterDeath.wav", false, &MonsterDeathSoundID))
    {
        return false;
    }

    // Setup player entity
    PlayerEntity = *CreateEntity();

    auto& playerTransformComponent = PlayerEntity.AddComponent<TransformComponent>();
    playerTransformComponent.Transform.Position = { 0.0f, 0.0f, -5.0f };

    auto& playerCameraComponent = PlayerEntity.AddComponent<CameraComponent>();

    auto& playerSphereCollisionComponent = PlayerEntity.AddComponent<SphereCollisionComponent>();
    playerSphereCollisionComponent.Radius = 0.2f;

    auto& playerRigidBodyComponent = PlayerEntity.AddComponent<RigidBodyComponent>();
    playerRigidBodyComponent.ApplyGravity = true;

    if (!PossessEntity(&PlayerEntity))
    {
        return false;
    }

    // Setup floor entity
    auto* pFloorEntity = CreateEntity();

    auto& floorTransformComponent = pFloorEntity->AddComponent<TransformComponent>();
    floorTransformComponent.Transform.Position = { 0.0f, 1.5f, 0.0f };
    floorTransformComponent.Transform.Rotation = { -90.0f, 0.0f, 0.0f };
    floorTransformComponent.Transform.Scale = { 50.0f, 50.0f, 1.0f };

    auto& floorStaticMeshComponent = pFloorEntity->AddComponent<StaticMeshComponent>();
    floorStaticMeshComponent.GeometryID = PlaneGeometryID;
    floorStaticMeshComponent.Material.TextureScale = { 25.0f, 25.0f };
    floorStaticMeshComponent.Material.TextureID = FloorTextureID;
    floorStaticMeshComponent.Material.SetSamplerID(Renderer::ESampler::NEAREST_NEIGHBOUR_FILTER);

    auto& floorAABBComponent = pFloorEntity->AddComponent<AABBCollisionComponent>();
    floorAABBComponent.Extent = Maths::CalculateAABBExtent({ 0.5f, 1.0f, 0.5f }, floorTransformComponent.Transform);

    auto& floorTagComponent = pFloorEntity->AddComponent<TagComponent>();
    floorTagComponent.Tag = Tags::FLOOR_TAG;

    // Wall 1
    auto* pWallEntity1 = CreateEntity();

    auto& WallTransformComponent1 = pWallEntity1->AddComponent<TransformComponent>();
    WallTransformComponent1.Transform.Scale = { 5.0f, 3.0f, 0.5f };

    auto& wallStaticMeshComponent1 = pWallEntity1->AddComponent<StaticMeshComponent>();
    wallStaticMeshComponent1.GeometryID = CubeGeometryID;
    wallStaticMeshComponent1.Material.TextureID = WallTextureID;
    wallStaticMeshComponent1.Material.SetSamplerID(Renderer::ESampler::NEAREST_NEIGHBOUR_FILTER);
    wallStaticMeshComponent1.Material.TextureScale = { 5.0f, 5.0f };

    auto& wallAABBCollisionComponent1 = pWallEntity1->AddComponent<AABBCollisionComponent>();
    wallAABBCollisionComponent1.Extent = Maths::CalculateAABBExtent(glm::vec3(0.5f, 0.5f, 0.5f), WallTransformComponent1.Transform);

    // Wall 2
    auto* pWallEntity2 = CreateEntity();

    auto& wallTransformComponent2 = pWallEntity2->AddComponent<TransformComponent>();
    wallTransformComponent2.Transform.Position = { 2.5f, 0.0f, -2.5f };
    wallTransformComponent2.Transform.Rotation = { 0.0f, 90.0f, 0.0f };
    wallTransformComponent2.Transform.Scale = glm::vec3(5.0f, 3.0f, 0.5f);

    auto& wallStaticMeshComponent2 = pWallEntity2->AddComponent<StaticMeshComponent>();
    wallStaticMeshComponent2.GeometryID = CubeGeometryID;
    wallStaticMeshComponent2.Material.TextureID = WallTextureID;
    wallStaticMeshComponent2.Material.SetSamplerID(Renderer::ESampler::NEAREST_NEIGHBOUR_FILTER);
    wallStaticMeshComponent2.Material.TextureScale = { 5.0f, 5.0f };

    auto& wallAABBCollisionComponent2 = pWallEntity2->AddComponent<AABBCollisionComponent>();
    wallAABBCollisionComponent2.Extent = Maths::CalculateAABBExtent(glm::vec3(0.5f, 0.5f, 0.5f), wallTransformComponent2.Transform);

    // Enemy
    auto* pEnemyEntity = &EnemyEntities.emplace_back(*CreateEntity());

    auto& enemyTransformComponent = pEnemyEntity->AddComponent<TransformComponent>();
    enemyTransformComponent.Transform.Position = { 0.0f, 0.75f, -2.0f };
    enemyTransformComponent.Transform.Scale = { 1.0f, 1.5f, 1.0f };

    auto& enemyStaticMeshComponent = pEnemyEntity->AddComponent<StaticMeshComponent>();
    enemyStaticMeshComponent.GeometryID = PlaneGeometryID;
    enemyStaticMeshComponent.Material.TextureID = MonsterTextureID;
    enemyStaticMeshComponent.Material.SetSamplerID(Renderer::ESampler::NEAREST_NEIGHBOUR_FILTER);
    enemyStaticMeshComponent.Material.AlphaBlended = true;

    auto& enemyBillboardComponent = pEnemyEntity->AddComponent<BillboardComponent>();
    enemyBillboardComponent.CanLean = false;

    auto& enemyAABBComponent = pEnemyEntity->AddComponent<AABBCollisionComponent>();
    enemyAABBComponent.Extent = Maths::CalculateAABBExtent({ 0.5f, 0.5f, 0.1f }, enemyTransformComponent.Transform);
    enemyAABBComponent.CanCollideWithSphereCollisions = false;

    auto& enemySoundEmitter3D = pEnemyEntity->AddComponent<SoundEmitter3DComponent>();
    Audio::CreateSoundSourceVoice(MonsterDeathSoundID, &enemySoundEmitter3D.SoundSourceVoice);
    enemySoundSourceVoices[0] = &enemySoundEmitter3D.SoundSourceVoice;
    enemySoundEmitter3D.SoundSourceVoice.GetSourceVoice()->SetVolume(MonsterDeathSoundVolume);

    auto& enemyTagComponent = pEnemyEntity->AddComponent<TagComponent>();
    enemyTagComponent.Tag = Tags::ENEMY_TAG;

    // Enemy 2
    auto* pEnemyEntity2 = &EnemyEntities.emplace_back(*CreateEntity());

    auto& enemyTransformComponent2 = pEnemyEntity2->AddComponent<TransformComponent>();
    enemyTransformComponent2.Transform.Position = { -10.0f, 0.75f, -7.0f };
    enemyTransformComponent2.Transform.Scale = { 1.0f, 1.5f, 1.0f };

    auto& enemyStaticMeshComponent2 = pEnemyEntity2->AddComponent<StaticMeshComponent>();
    enemyStaticMeshComponent2.GeometryID = PlaneGeometryID;
    enemyStaticMeshComponent2.Material.TextureID = MonsterTextureID;
    enemyStaticMeshComponent2.Material.SetSamplerID(Renderer::ESampler::NEAREST_NEIGHBOUR_FILTER);
    enemyStaticMeshComponent2.Material.AlphaBlended = true;

    auto& enemyBillboardComponent2 = pEnemyEntity2->AddComponent<BillboardComponent>();
    enemyBillboardComponent2.CanLean = false;

    auto& enemyAABBComponent2 = pEnemyEntity2->AddComponent<AABBCollisionComponent>();
    enemyAABBComponent2.Extent = Maths::CalculateAABBExtent({ 0.5f, 0.5f, 0.1f }, enemyTransformComponent2.Transform);
    enemyAABBComponent2.CanCollideWithSphereCollisions = false;

    auto& enemySoundEmitter3D2 = pEnemyEntity2->AddComponent<SoundEmitter3DComponent>();
    Audio::CreateSoundSourceVoice(MonsterDeathSoundID, &enemySoundEmitter3D2.SoundSourceVoice);
    enemySoundSourceVoices[1] = &enemySoundEmitter3D2.SoundSourceVoice;
    enemySoundEmitter3D2.SoundSourceVoice.GetSourceVoice()->SetVolume(MonsterDeathSoundVolume);

    auto& enemyTagComponent2 = pEnemyEntity2->AddComponent<TagComponent>();
    enemyTagComponent2.Tag = Tags::ENEMY_TAG;

    return true;
}

void Level01::UnLoad()
{
    Renderer::DestroyGeometry(CubeGeometryID);
    Renderer::DestroyGeometry(PlaneGeometryID);

    Renderer::DestroyTexture(MonsterTextureID);
    Renderer::DestroyTexture(WallTextureID);
    Renderer::DestroyTexture(FloorTextureID);

    Audio::DestroySound(MonsterDeathSoundID, enemySoundSourceVoices.data(), static_cast<uint32_t>(enemySoundSourceVoices.size()));

    HUDClassInstance->UnLoad();
}

void Level01::Begin()
{
    Super::Begin();
}

void Level01::Tick(const float deltaTime)
{
    Super::Tick(deltaTime);
}

void Level01::TickFixed()
{
}
