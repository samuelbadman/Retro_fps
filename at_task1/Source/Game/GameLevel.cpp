#include "Pch.h"
#include "GameLevel.h"
#include "Maths/Maths.h"
#include "Renderer/Renderer.h"

#include "Game/Tags.h"

#include "Game/Components/TransformComponent.h"
#include "Game/Components/TransformComponent.h"
#include "Game/Components/StaticMeshComponent.h"
#include "Game/Components/CameraComponent.h"
#include "Game/Components/SphereCollisionComponent.h"
#include "Game/Components/AABBCollisionComponent.h"
#include "Game/Components/RigidBodyComponent.h"
#include "Game/Components/BillboardComponent.h"
#include "Game/Components/SoundEmitter3DComponent.h"
#include "Game/Components/TagComponent.h"
#include "Game/Components/EnemyAIComponent.h"
#include "Game/Components/LevitateComponent.h"

bool GameLevel::Load()
{
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

    if (!Renderer::LoadCylinderGeometryPrimitive(1.0f, 1.0f, 1.0f, 6, 6, &CylinderGeometryID))
    {
        return false;
    }

    if (!Renderer::LoadTexture("Assets/Game/Textures/level_goal.png", true, &LevelGoalTextureID))
    {
        return false;
    }

    if (!Renderer::LoadTexture("Assets/Game/Textures/power_cell.png", true, &PowerCellTextureID))
    {
        return false;
    }

    if (!Renderer::LoadTexture("Assets/Game/Textures/barrel.png", true, &BarrelTextureID))
    {
        return false;
    }

    return true;
}

void GameLevel::UnLoad()
{
    Renderer::DestroyGeometry(CubeGeometryID);
    Renderer::DestroyGeometry(PlaneGeometryID);

    Renderer::DestroyTexture(MonsterTextureID);
    Renderer::DestroyTexture(WallTextureID);
    Renderer::DestroyTexture(FloorTextureID);

    Audio::DestroySound(MonsterDeathSoundID, enemySoundSourceVoices.data(), static_cast<uint32_t>(enemySoundSourceVoices.size()));

    Renderer::DestroyGeometry(CylinderGeometryID);
    Renderer::DestroyTexture(LevelGoalTextureID);

    Renderer::DestroyTexture(PowerCellTextureID);

    Renderer::DestroyTexture(BarrelTextureID);

    if (HUDClassInstance)
    {
        HUDClassInstance->UnLoad();
    }
}

bool GameLevel::SetupPlayer(const glm::vec3& startPosition)
{
    PlayerEntity = *CreateEntity();

    auto& playerTransformComponent = PlayerEntity.AddComponent<TransformComponent>();
    playerTransformComponent.Transform.Position = startPosition;

    auto& playerCameraComponent = PlayerEntity.AddComponent<CameraComponent>();

    auto& playerSphereCollisionComponent = PlayerEntity.AddComponent<SphereCollisionComponent>();
    playerSphereCollisionComponent.Radius = 0.2f;

    auto& playerRigidBodyComponent = PlayerEntity.AddComponent<RigidBodyComponent>();
    playerRigidBodyComponent.ApplyGravity = true;

    return PossessEntity(&PlayerEntity);
}

void GameLevel::AddFloor(const glm::vec2& scale, const glm::vec2& textureScale)
{
    auto* pFloorEntity = CreateEntity();

    auto& floorTransformComponent = pFloorEntity->AddComponent<TransformComponent>();
    floorTransformComponent.Transform.Position = { 0.0f, FloorYPosition, 0.0f };
    floorTransformComponent.Transform.Rotation = { -90.0f, 0.0f, 0.0f };
    floorTransformComponent.Transform.Scale = { scale.x, scale.y, 1.0f };

    auto& floorStaticMeshComponent = pFloorEntity->AddComponent<StaticMeshComponent>();
    floorStaticMeshComponent.GeometryID = PlaneGeometryID;
    floorStaticMeshComponent.Material.TextureScale = textureScale;
    floorStaticMeshComponent.Material.TextureID = FloorTextureID;
    floorStaticMeshComponent.Material.SetSamplerID(Renderer::ESampler::NEAREST_NEIGHBOUR_FILTER);

    auto& floorAABBComponent = pFloorEntity->AddComponent<AABBCollisionComponent>();
    floorAABBComponent.Extent = Maths::CalculateAABBExtent({ 0.5f, 1.0f, 0.5f }, floorTransformComponent.Transform);

    auto& floorTagComponent = pFloorEntity->AddComponent<TagComponent>();
    floorTagComponent.Tag = Tags::FLOOR_TAG;
}

void GameLevel::AddRoof(const glm::vec2& scale, const glm::vec2& textureScale)
{
    auto* pFloorEntity = CreateEntity();

    auto& floorTransformComponent = pFloorEntity->AddComponent<TransformComponent>();
    floorTransformComponent.Transform.Position = { 0.0f, RoofYPosition, 0.0f };
    floorTransformComponent.Transform.Rotation = { 90.0f, 0.0f, 0.0f };
    floorTransformComponent.Transform.Scale = { scale.x, scale.y, 1.0f };

    auto& floorStaticMeshComponent = pFloorEntity->AddComponent<StaticMeshComponent>();
    floorStaticMeshComponent.GeometryID = PlaneGeometryID;
    floorStaticMeshComponent.Material.TextureScale = textureScale;
    floorStaticMeshComponent.Material.TextureID = FloorTextureID;
    floorStaticMeshComponent.Material.SetSamplerID(Renderer::ESampler::NEAREST_NEIGHBOUR_FILTER);
}

void GameLevel::AddWall(const Maths::Transform& transform, const glm::vec2& textureScale)
{
    auto* pWallEntity = CreateEntity();

    auto& WallTransformComponent = pWallEntity->AddComponent<TransformComponent>();
    WallTransformComponent.Transform = transform;
    WallTransformComponent.Transform.Position.y = WallYPosition;
    WallTransformComponent.Transform.Scale.y = WallYScale;

    auto& wallStaticMeshComponent = pWallEntity->AddComponent<StaticMeshComponent>();
    wallStaticMeshComponent.GeometryID = CubeGeometryID;
    wallStaticMeshComponent.Material.TextureID = WallTextureID;
    wallStaticMeshComponent.Material.SetSamplerID(Renderer::ESampler::NEAREST_NEIGHBOUR_FILTER);
    wallStaticMeshComponent.Material.TextureScale = textureScale;

    auto& wallAABBCollisionComponent = pWallEntity->AddComponent<AABBCollisionComponent>();
    wallAABBCollisionComponent.Extent = Maths::CalculateAABBExtent(glm::vec3(0.5f, 0.5f, 0.5f), WallTransformComponent.Transform);
}

void GameLevel::AddEnemy(const glm::vec3& position)
{
    auto* pEnemyEntity = CreateEntity();

    auto& enemyTransformComponent = pEnemyEntity->AddComponent<TransformComponent>();
    enemyTransformComponent.Transform.Position = position;
    enemyTransformComponent.Transform.Position.y = EnemyYPosition;
    enemyTransformComponent.Transform.Scale = { 1.0f, 1.5f, 1.0f };

    auto& enemyStaticMeshComponent = pEnemyEntity->AddComponent<StaticMeshComponent>();
    enemyStaticMeshComponent.GeometryID = PlaneGeometryID;
    enemyStaticMeshComponent.Material.TextureID = MonsterTextureID;
    enemyStaticMeshComponent.Material.SetSamplerID(Renderer::ESampler::NEAREST_NEIGHBOUR_FILTER);
    enemyStaticMeshComponent.Material.AlphaBlended = true;

    auto& enemyBillboardComponent = pEnemyEntity->AddComponent<BillboardComponent>();
    enemyBillboardComponent.CanLean = false;

    auto& enemyAABBComponent = pEnemyEntity->AddComponent<AABBCollisionComponent>();
    enemyAABBComponent.Extent = Maths::CalculateAABBExtent({ 1.0f, 1.0f, 1.0f }, enemyTransformComponent.Transform);
    enemyAABBComponent.CanCollideWithSphereCollisions = false;

    auto& enemySoundEmitter3D = pEnemyEntity->AddComponent<SoundEmitter3DComponent>();
    Audio::CreateSoundSourceVoice(MonsterDeathSoundID, &enemySoundEmitter3D.SoundSourceVoice);
    enemySoundSourceVoices.push_back(&enemySoundEmitter3D.SoundSourceVoice);
    enemySoundEmitter3D.SoundSourceVoice.GetSourceVoice()->SetVolume(MonsterDeathSoundVolume);

    auto& enemyTagComponent = pEnemyEntity->AddComponent<TagComponent>();
    enemyTagComponent.Tag = Tags::ENEMY_TAG;

    auto& enemyAIComponent = pEnemyEntity->AddComponent<EnemyAIComponent>();
    enemyAIComponent.AttackRadius = EnemyAttackRadius;
}

void GameLevel::AddLevelGoal(const glm::vec3& position)
{
    // Cylinder
    auto* pCylinderEntity = CreateEntity();

    auto& cylinderTransformComponent = pCylinderEntity->AddComponent<TransformComponent>();
    cylinderTransformComponent.Transform.Position = position;
    cylinderTransformComponent.Transform.Position.y = LevelGoalYPosition;
    cylinderTransformComponent.Transform.Rotation = { 90.0f, 0.0f, 0.0f };
    cylinderTransformComponent.Transform.Scale = { 1.5f, 1.5f, 1.0f };

    auto& cylinderMeshComponent = pCylinderEntity->AddComponent<StaticMeshComponent>();
    cylinderMeshComponent.GeometryID = CylinderGeometryID;
    cylinderMeshComponent.Material.SetSamplerID(Renderer::ESampler::NEAREST_NEIGHBOUR_FILTER);
    cylinderMeshComponent.Material.TextureID = LevelGoalTextureID;
    cylinderMeshComponent.Material.TextureScale = { 1.0f, 1.0f };

    auto& cylinderSphereComponent = pCylinderEntity->AddComponent<SphereCollisionComponent>();
    cylinderSphereComponent.Radius = 4.0f;

    auto& cylinderTagComponent = pCylinderEntity->AddComponent<TagComponent>();
    cylinderTagComponent.Tag = Tags::LEVEL_GOAL_TAG;

    auto& cylinderAABBComponent = pCylinderEntity->AddComponent<AABBCollisionComponent>();
    cylinderAABBComponent.Extent = Maths::CalculateAABBExtent({ 1.0f, 1.0f, 1.0f }, cylinderTransformComponent.Transform);

    // Power cell
    auto* pConeEntity = CreateEntity();

    auto& coneTransformComponent = pConeEntity->AddComponent<TransformComponent>();
    coneTransformComponent.Transform.Position = position;
    coneTransformComponent.Transform.Position.y = LevelGoalYPosition + LevelGoalDecorationYPosition;
    coneTransformComponent.Transform.Rotation = { 90.0f, 0.0f, 0.0f };
    coneTransformComponent.Transform.Scale = { 0.3f, 0.3f, 0.8f };

    auto& coneMeshComponent = pConeEntity->AddComponent<StaticMeshComponent>();
    coneMeshComponent.GeometryID = CylinderGeometryID;
    coneMeshComponent.Material.AlphaBlended = false;
    coneMeshComponent.Material.TextureID = PowerCellTextureID;
    coneMeshComponent.Material.SetSamplerID(Renderer::ESampler::NEAREST_NEIGHBOUR_FILTER);
    coneMeshComponent.Material.TextureScale = { 1.0f, 1.0f };

    auto& coneLevitateComponent = pConeEntity->AddComponent<LevitateComponent>();
    coneLevitateComponent.OriginalHeight = coneTransformComponent.Transform.Position.y;
    coneLevitateComponent.MaxHeightDelta = 0.5f;
    coneLevitateComponent.RotationDelta = { 0.004f, 0.01f, 0.006f };
}

void GameLevel::AddBarrel(const glm::vec3& position)
{
    auto* pBarrelEntity = CreateEntity();

    auto& barrelTransform = pBarrelEntity->AddComponent<TransformComponent>();
    barrelTransform.Transform.Position = position;
    barrelTransform.Transform.Position.y = BarrelYPosition;
    barrelTransform.Transform.Rotation = { 90.0f, 0.0f, 0.0f };
    barrelTransform.Transform.Scale = { 0.4f, 0.4f, 0.9f };

    auto& barrelStaticMesh = pBarrelEntity->AddComponent<StaticMeshComponent>();
    barrelStaticMesh.GeometryID = CylinderGeometryID;
    barrelStaticMesh.Material.AlphaBlended = false;
    barrelStaticMesh.Material.SetSamplerID(Renderer::ESampler::NEAREST_NEIGHBOUR_FILTER);
    barrelStaticMesh.Material.TextureID = BarrelTextureID;

    auto& barrelAABBCollision = pBarrelEntity->AddComponent<AABBCollisionComponent>();
    barrelAABBCollision.Extent = Maths::CalculateAABBExtent({ 2.0f, 2.0f, 2.0f }, barrelTransform.Transform);
}
