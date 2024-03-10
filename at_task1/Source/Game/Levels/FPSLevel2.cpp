#include "Pch.h"
#include "FPSLevel2.h"
#include "Maths/Transform.h"

#include "Game/HUDS/FPSHUD.h"

bool FPSLevel2::Load()
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

    SetupPlayer({ 0.1f, 0.0f, -5.0f });
    AddFloor({ 200.0f, 200.0f }, { 100.0f, 100.0f });
    AddRoof({ 200.0f, 200.0f }, { 100.0f, 100.0f });

    AddWall({ glm::vec3(0.0f, 0.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f) , glm::vec3(12.0f, 1.0f, 0.5f) }, glm::vec2(1.2f, 5.0f));
    AddWall({ glm::vec3(0.0f, 0.0f, -12.0f), glm::vec3(0.0f, 0.0f, 0.0f) , glm::vec3(12.0f, 1.0f, 0.5f) }, glm::vec2(1.2f, 5.0f));
    AddWall({ glm::vec3(-6.0f, 0.0f, -9.0f), glm::vec3(0.0f, 90.0f, 0.0f) , glm::vec3(6.0f, 1.0f, 0.5f) }, glm::vec2(1.4f, 5.0f));
    AddWall({ glm::vec3(-6.0f, 0.0f, 0.0f), glm::vec3(0.0f, 90.0f, 0.0f) , glm::vec3(6.0f, 1.0f, 0.5f) }, glm::vec2(1.4f, 5.0f));
    AddWall({ glm::vec3(6.0f, 0.0f, -9.0f), glm::vec3(0.0f, 90.0f, 0.0f) , glm::vec3(6.0f, 1.0f, 0.5f) }, glm::vec2(1.4f, 5.0f));
    AddWall({ glm::vec3(6.0f, 0.0f, 0.0f), glm::vec3(0.0f, 90.0f, 0.0f) , glm::vec3(6.0f, 1.0f, 0.5f) }, glm::vec2(1.4f, 5.0f));
    AddWall({ glm::vec3(12.0f, 0.0f, -2.7f), glm::vec3(0.0f, 0.0f, 0.0f) , glm::vec3(12.0f, 1.0f, 0.5f) }, glm::vec2(1.2f, 5.0f));
    AddWall({ glm::vec3(15.5f, 0.0f, -6.2f), glm::vec3(0.0f, 0.0f, 0.0f) , glm::vec3(19.0f, 1.0f, 0.5f) }, glm::vec2(1.2f, 5.0f));
    AddWall({ glm::vec3(25.0f, 0.0f, -6.2f), glm::vec3(0.0f, 90.0f, 0.0f) , glm::vec3(12.0f, 1.0f, 0.5f) }, glm::vec2(1.2f, 5.0f));
    AddWall({ glm::vec3(21.0f, 0.0f, 6.5f), glm::vec3(0.0f, 90.0f, 0.0f) , glm::vec3(19.0f, 1.0f, 0.5f) }, glm::vec2(1.2f, 5.0f));
    AddWall({ glm::vec3(18.0f, 0.0f, 4.6f), glm::vec3(0.0f, 90.0f, 0.0f) , glm::vec3(15.0f, 1.0f, 0.5f) }, glm::vec2(1.2f, 5.0f));
    AddWall({ glm::vec3(27.0f, 0.0f, -2.7f), glm::vec3(0.0f, 0.0f, 0.0f) , glm::vec3(12.0f, 1.0f, 0.5f) }, glm::vec2(1.2f, 5.0f));
    AddWall({ glm::vec3(10.0f, 0.0f, 15.0f), glm::vec3(0.0f, 0.0f, 0.0f) , glm::vec3(22.0f, 1.0f, 0.5f) }, glm::vec2(1.2f, 5.0f));
    AddWall({ glm::vec3(8.5f, 0.0f, 12.0f), glm::vec3(0.0f, 0.0f, 0.0f) , glm::vec3(19.0f, 1.0f, 0.5f) }, glm::vec2(1.2f, 5.0f));
    AddWall({ glm::vec3(-1.0f, 0.0f, 7.0f), glm::vec3(0.0f, 90.0f, 0.0f) , glm::vec3(10.0f, 1.0f, 0.5f) }, glm::vec2(1.2f, 5.0f));
    AddWall({ glm::vec3(-5.0f, 0.0f, 7.0f), glm::vec3(0.0f, 90.0f, 0.0f) , glm::vec3(10.0f, 1.0f, 0.5f) }, glm::vec2(1.2f, 5.0f));
    AddWall({ glm::vec3(-3.0f, 0.0f, 27.0f), glm::vec3(0.0f, 0.0f, 0.0f) , glm::vec3(15.0f, 1.0f, 0.5f) }, glm::vec2(1.2f, 5.0f));
    AddWall({ glm::vec3(-1.0f, 0.0f, 17.0f), glm::vec3(0.0f, 90.0f, 0.0f) , glm::vec3(4.0f, 1.0f, 0.5f) }, glm::vec2(2.0f, 5.0f));
    AddWall({ glm::vec3(-5.0f, 0.0f, 17.0f), glm::vec3(0.0f, 90.0f, 0.0f) , glm::vec3(4.0f, 1.0f, 0.5f) }, glm::vec2(2.0f, 5.0f));
    AddWall({ glm::vec3(-7.0f, 0.0f, 19.0f), glm::vec3(0.0f, 0.0f, 0.0f) , glm::vec3(4.0f, 1.0f, 0.5f) }, glm::vec2(2.0f, 5.0f));
    AddWall({ glm::vec3(1.0f, 0.0f, 19.0f), glm::vec3(0.0f, 0.0f, 0.0f) , glm::vec3(4.0f, 1.0f, 0.5f) }, glm::vec2(2.0f, 5.0f));
    AddWall({ glm::vec3(3.0f, 0.0f, 23.0f), glm::vec3(0.0f, 90.0f, 0.0f) , glm::vec3(9.0f, 1.0f, 0.5f) }, glm::vec2(2.0f, 5.0f));
    AddWall({ glm::vec3(-9.0f, 0.0f, 23.0f), glm::vec3(0.0f, 90.0f, 0.0f) , glm::vec3(9.0f, 1.0f, 0.5f) }, glm::vec2(2.0f, 5.0f));
    AddWall({ glm::vec3(-7.5f, 0.0f, -2.7f), glm::vec3(0.0f, 0.0f, 0.0f) , glm::vec3(3.0f, 1.0f, 0.5f) }, glm::vec2(2.0f, 5.0f));
    AddWall({ glm::vec3(-7.5f, 0.0f, -6.3f), glm::vec3(0.0f, 0.0f, 0.0f) , glm::vec3(3.0f, 1.0f, 0.5f) }, glm::vec2(2.0f, 5.0f));
    AddWall({ glm::vec3(-8.8f, 0.0f, -7.1f), glm::vec3(0.0f, 90.0f, 0.0f) , glm::vec3(2.0f, 1.0f, 0.5f) }, glm::vec2(2.5f, 5.0f));
    AddWall({ glm::vec3(-12.0f, 0.0f, -8.0f), glm::vec3(0.0f, 0.0f, 0.0f) , glm::vec3(12.0f, 1.0f, 0.5f) }, glm::vec2(1.2f, 5.0f));
    AddWall({ glm::vec3(-18.0f, 0.0f, -2.0f), glm::vec3(0.0f, 90.0f, 0.0f) , glm::vec3(12.0f, 1.0f, 0.5f) }, glm::vec2(1.2f, 5.0f));
    AddWall({ glm::vec3(-15.0f, 0.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f) , glm::vec3(5.0f, 1.0f, 0.5f) }, glm::vec2(2.0f, 6.0f));
    AddWall({ glm::vec3(-8.65f, 0.0f, 3.2), glm::vec3(0.0f, 90.0f, 0.0f) , glm::vec3(12.0f, 1.0f, 0.5f) }, glm::vec2(2.5f, 5.0f));
    AddWall({ glm::vec3(-15.0f, 0.0f, 9.0f), glm::vec3(0.0f, 90.0f, 0.0f) , glm::vec3(14.0f, 1.0f, 0.5f) }, glm::vec2(2.0f, 6.0f));
    AddWall({ glm::vec3(-10.0f, 0.0f, 15.0f), glm::vec3(0.0f, 0.0f, 0.0f) , glm::vec3(10.0f, 1.0f, 0.5f) }, glm::vec2(2.0f, 6.0f));
    AddWall({ glm::vec3(-6.8f, 0.0f, 8.8f), glm::vec3(0.0f, 0.0f, 0.0f) , glm::vec3(4.0f, 1.0f, 0.5f) }, glm::vec2(2.0f, 6.0f));

    AddEnemy({ -11.0f, 0.0f, 0.0f });
    AddEnemy({ -10.0f, 0.0f, 8.0f });
    AddEnemy({ -13.0f, 0.0f, 8.5f });
    AddEnemy({ 1.5f, 0.0f, 13.5f });
    AddEnemy({ 19.5f, 0.0f, 2.0f });

    AddLevelGoal({ -3.0f, 0.0f, 23.0f });

    AddBarrel({ 24.0f, 0.0f, -3.5f });
    AddBarrel({ 24.0f, 0.0f, -4.3f });

    AddBarrel({ 2.0f, 0.0f, 26.1f });

    AddBarrel({ -2.5f, 0.0f, 3.0f });
    AddBarrel({ -3.4f, 0.0f, 3.0f });

    return true;
}

void FPSLevel2::UnLoad()
{
    Super::UnLoad();
}

void FPSLevel2::Begin()
{
}

void FPSLevel2::Tick(const float deltaTime)
{
}

void FPSLevel2::TickFixed()
{
}
