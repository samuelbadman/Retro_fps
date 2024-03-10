#include "Pch.h"
#include "GameInput.h"
#include "Entity.h"
#include "EventSystem/EventStructures/InputEvent.h"
#include "Input/InputCodes.h"
#include "Maths/Maths.h"
#include "Console.h"

#include "Game/GameEvents.h"
#include "Game/GameState.h"
#include "Game/GameAudio.h"

#include "Game/World.h"
#include "Game/Components/TransformComponent.h"
#include "Game/Components/RigidBodyComponent.h"
#include "Game/Components/AABBCollisionComponent.h"
#include "Game/Components/BillboardComponent.h"

#include "Game/CollisionDetection.h"

#include "Game/Levels/MainMenuLevel.h"

constexpr int16_t InteractInputKey{ InputCodes::E };
constexpr int16_t InteractInputGamepadButton{ InputCodes::Gamepad_Face_Button_Bottom };
constexpr int16_t ConfirmMenuInputKey{ InputCodes::Space_Bar };
constexpr int16_t ConfirmMenuGamepadButton{ InputCodes::Gamepad_Special_Right };

constexpr int16_t fireInputKey{ InputCodes::Left_Mouse_Button };
constexpr int16_t fireInputGamepadButton{ InputCodes::Gamepad_Right_Trigger };

constexpr float possessedEntityMouseRotationRate{ 0.005f };
constexpr float possessedEntityStickRotationRate{ 0.2f };
constexpr float possessedEntitySpeed{ 0.006f };
constexpr float possessedEntityJumpForce{ 0.9f };

constexpr float controlPitchClampMax{ 85.f };
constexpr float controlPitchClampMin{ -85.0f };

static void AddMovementInput(Entity* pPossessedEntity, const glm::vec3& direction, const float scale)
{
	// Get the possessed entity's rigidbody component
	auto& possessedEntityRigidBodyComponent = pPossessedEntity->GetComponent<RigidBodyComponent>();

	// Add scaled velocity in the direction
	possessedEntityRigidBodyComponent.Velocity += scale * direction * World::GetWorldDeltaTime();
}

static void MoveForward(Entity* pPossessedEntity, const float scale)
{
	// Get the possessed entity's transform component
	auto& possessedEntityTransformComponent = pPossessedEntity->GetComponent<TransformComponent>();

	// Calculate the entity's right vector
	auto entityForwardVector = glm::normalize(Maths::RotateVector(
		{ 0.0f, possessedEntityTransformComponent.Transform.Rotation.y, possessedEntityTransformComponent.Transform.Rotation.z },
		World::GetWorldForwardVector()));

	// Add movement
	AddMovementInput(pPossessedEntity, entityForwardVector, scale);
}

static void MoveRight(Entity* pPossessedEntity, const float scale)
{
	// Get the possessed entity's transform component
	auto& possessedEntityTransformComponent = pPossessedEntity->GetComponent<TransformComponent>();

	// Calculate the entity's right vector
	auto entityRightVector = glm::normalize(Maths::RotateVector(
		{ 0.0f, possessedEntityTransformComponent.Transform.Rotation.y, possessedEntityTransformComponent.Transform.Rotation.z },
		World::GetWorldRightVector()));

	// Add movement
	AddMovementInput(pPossessedEntity, entityRightVector, scale);
}

static void AddPitchDelta(Entity* pPossessedEntity, const float delta, const float minClamp, const float maxClamp)
{
	// Get the possessed entity's transform
	auto& possessedEntityTransform = pPossessedEntity->GetComponent<TransformComponent>().Transform;

	// Calculate pitch delta
	auto pitchDelta =  delta * World::GetWorldDeltaTime();

	// Calculate new pitch clamped
	auto newPitch = possessedEntityTransform.Rotation.x + pitchDelta;

	// Clamp pitch
	newPitch = glm::clamp(newPitch, minClamp, maxClamp);

	// Add pitch delta to the player rotation if the clamp has not been reached
	possessedEntityTransform.Rotation.x = newPitch;
}

static void AddYawDelta(Entity* pPossessedEntity, const float delta)
{
	// Get the possessed entity's transform
	auto& possessedEntityTransform = pPossessedEntity->GetComponent<TransformComponent>().Transform;

	// Add yaw delta
	possessedEntityTransform.Rotation.y +=  delta * World::GetWorldDeltaTime();
}

static bool IsInputPressed(int16_t key)
{
	return 0x80000000 & GetAsyncKeyState(key);
}

static void HandleInGameInputEventForPossessedEntity(Entity* pPossessedEntity, const InputEvent& event)
{
	if (event.Input == InputCodes::Gamepad_Left_Thumbstick_X_Axis)
	{
		// Move the possessed entity right
		MoveRight(pPossessedEntity, possessedEntitySpeed * event.Data);
	}
	else if (event.Input == InputCodes::Gamepad_Left_Thumbstick_Y_Axis)
	{
		// Move the possessed entity right
		MoveForward(pPossessedEntity, possessedEntitySpeed * event.Data);
	}
	else if (event.Input == InputCodes::Mouse_X)
	{
		// Check the right mouse button is pressed
		//if (IsInputPressed(InputCodes::Right_Mouse_Button))
		//{
			AddYawDelta(pPossessedEntity, possessedEntityMouseRotationRate * event.Data);
		//}
	}
	else if (event.Input == InputCodes::Mouse_Y)
	{
		// Check the right mouse button is pressed
		//if (IsInputPressed(InputCodes::Right_Mouse_Button))
		//{
			// Disabled for retro feel
			//AddPitchDelta(pPossessedEntity, -possessedEntityMouseRotationRate * event.Data, controlPitchClampMin, controlPitchClampMax);
		//}
	}
	else if (event.Input == InputCodes::Gamepad_Right_Thumbstick_X_Axis)
	{
		AddYawDelta(pPossessedEntity, possessedEntityStickRotationRate * event.Data);
	}
	else if (event.Input == InputCodes::Gamepad_Right_Thumbstick_Y_Axis)
	{
		// Disabled for retro feel
		//AddPitchDelta(pPossessedEntity, possessedEntityStickRotationRate * event.Data, controlPitchClampMin, controlPitchClampMax);
	}
	else if (((event.Input == InteractInputKey) || (event.Input == InteractInputGamepadButton)) && (event.Data == 1.0f) && (!event.RepeatedKey))
	{
		// Call player interacted event
		GameEvents::PlayerInteracted();
	}
	else if ((event.Input == fireInputKey || event.Input == fireInputGamepadButton) && (event.Data == 1.0f) && (!event.RepeatedKey))
	{
		// Call player fired event
		GameEvents::PlayerFired();
	}
}

static void HandleInMenuInputEvent(const InputEvent& event)
{
	// Check menu confirm input pressed
	if (!(((event.Input == ConfirmMenuInputKey) || (event.Input == ConfirmMenuGamepadButton)) && (event.Data == 1.0f) && (!event.RepeatedKey)))
	{
		return;
	}

	switch (GameState::GetCurrentState())
	{
	case GameState::EState::IN_GAME:
		// Do nothing
		break;

	case GameState::EState::MAIN_MENU:
		// Open level 1
		GameState::LoadNextLevel();
		// Update the game state
		GameState::SetCurrentState(GameState::EState::IN_GAME);
		GameAudio::StartBackgroundTrack();
		break;

	case GameState::EState::WIN_SCREEN:
	case GameState::EState::LOSE_SCREEN:
		// Open main menu
		if (!World::LoadLevel(std::make_unique<MainMenuLevel>(), false))
		{
			assert(false && "Failed to load main menu level from win/lose screen.");
		}
		break;
	}
}

void GameInput::HandleInputEvent(InputEvent&& event)
{
	switch (GameState::GetCurrentState())
	{
	case GameState::EState::IN_GAME:
	{
		// Get the possessed entity
		auto* pPossessedEntity = World::GetLoadedLevel().GetPossessedEntity();

		// Handle the event for the possessed entity if it is valid
		if (pPossessedEntity->Valid())
		{
			HandleInGameInputEventForPossessedEntity(pPossessedEntity, event);
		}

		break;
	}

	case GameState::EState::MAIN_MENU:
	case GameState::EState::WIN_SCREEN:
	case GameState::EState::LOSE_SCREEN:
		HandleInMenuInputEvent(event);
		break;
	}
}

void GameInput::PollInputs()
{
	// Check if the game state is in game
	if (GameState::GetCurrentState() != GameState::EState::IN_GAME)
	{
		return;
	}

	// Get the possessed entity in the level
	auto* pPossessedEntity = World::GetLoadedLevel().GetPossessedEntity();

	// Check the possessed entity is valid
	if (!pPossessedEntity->Valid())
	{
		return;
	}

	if (IsInputPressed(InputCodes::S))
	{
		MoveForward(pPossessedEntity, possessedEntitySpeed * -1.0f);
	}

	if (IsInputPressed(InputCodes::W))
	{
		MoveForward(pPossessedEntity, possessedEntitySpeed * 1.0f);
	}

	if (IsInputPressed(InputCodes::D))
	{
		MoveRight(pPossessedEntity, possessedEntitySpeed * 1.0f);
	}

	if (IsInputPressed(InputCodes::A))
	{
		MoveRight(pPossessedEntity, possessedEntitySpeed * -1.0f);
	}
}
