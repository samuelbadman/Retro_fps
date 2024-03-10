#include "Pch.h"
#include "GamepadManager.h"
#include "Input/Input.h"
#include "EventSystem/EventSystem.h"

constexpr float GamepadLeftStickDeadzoneRadius = 0.24f;
constexpr float GamepadRightStickDeadzoneRadius = 0.24f;
constexpr int16_t GamepadMaxStickMagnitude = 32767;
constexpr int16_t GamepadMaxTriggerMagnitude = 255;
static XINPUT_STATE PrevStates[XUSER_MAX_COUNT] = {};

static void ApplyCircularDeadzone(float& axisX, float& axisY, float deadzoneRadius)
{
	float normX{ std::max(-1.0f, axisX / static_cast<float>(GamepadMaxStickMagnitude)) };
	float normY{ std::max(-1.0f, axisY / static_cast<float>(GamepadMaxStickMagnitude)) };

	float absNormX{ std::abs(normX) };
	float absNormY{ std::abs(normY) };
	axisX = (absNormX < deadzoneRadius ? 0.0f : (absNormX - deadzoneRadius) * (normX / absNormX));
	axisY = (absNormY < deadzoneRadius ? 0.0f : (absNormY - deadzoneRadius) * (normY / absNormY));

	if (deadzoneRadius > 0.0f)
	{
		axisX /= 1.0f - deadzoneRadius;
		axisY /= 1.0f - deadzoneRadius;
	}
}

static void RefreshButton(const XINPUT_STATE& state, const XINPUT_STATE& prevState, uint32_t port, int16_t button)
{
	// Get the current and previous states of the button
	auto currentButtonState{ state.Gamepad.wButtons & static_cast<WORD>(button) };
	auto prevButtonState{ prevState.Gamepad.wButtons & static_cast<WORD>(button) };

	// Return if the current state of the button is the same as its previous state
	if (currentButtonState == prevButtonState)
	{
		return;
	}

	// Check button state
	if (currentButtonState != 0)
	{
		// Button pressed
		EventSystem::SendEventImmediate(InputEvent{ false, button, port, 1.0f });
	}
	else
	{
		// Button released
		EventSystem::SendEventImmediate(InputEvent{ false, button, port, 0.0f });
	}
}

static void RefreshButtons(const XINPUT_STATE& state, const XINPUT_STATE& prevState, uint32_t port)
{
	RefreshButton(state, prevState, port, InputCodes::Gamepad_Face_Button_Bottom);
	RefreshButton(state, prevState, port, InputCodes::Gamepad_Face_Button_Top);
	RefreshButton(state, prevState, port, InputCodes::Gamepad_Face_Button_Left);
	RefreshButton(state, prevState, port, InputCodes::Gamepad_Face_Button_Right);
	RefreshButton(state, prevState, port, InputCodes::Gamepad_D_Pad_Down);
	RefreshButton(state, prevState, port, InputCodes::Gamepad_D_Pad_Up);
	RefreshButton(state, prevState, port, InputCodes::Gamepad_D_Pad_Left);
	RefreshButton(state, prevState, port, InputCodes::Gamepad_D_Pad_Right);
	RefreshButton(state, prevState, port, InputCodes::Gamepad_Special_Left);
	RefreshButton(state, prevState, port, InputCodes::Gamepad_Special_Right);
	RefreshButton(state, prevState, port, InputCodes::Gamepad_Left_Shoulder);
	RefreshButton(state, prevState, port, InputCodes::Gamepad_Right_Shoulder);
	RefreshButton(state, prevState, port, InputCodes::Gamepad_Left_Thumbstick_Button);
	RefreshButton(state, prevState, port, InputCodes::Gamepad_Right_Thumbstick_Button);
}

static void RefreshThumbsticks(const XINPUT_STATE& state, const XINPUT_STATE& prevState, uint32_t port)
{
	// Left
	// Axis
	// Calculate normalized axis value after a deadzone has been applied
	auto thumbLX{ static_cast<float>(state.Gamepad.sThumbLX) };
	auto thumbLY{ static_cast<float>(state.Gamepad.sThumbLY) };
	ApplyCircularDeadzone(
		thumbLX,
		thumbLY,
		GamepadLeftStickDeadzoneRadius
	);

	// Submit inputs for the stick
	EventSystem::SendEventImmediate(InputEvent{ false, InputCodes::Gamepad_Left_Thumbstick_X_Axis, port, thumbLX });
	EventSystem::SendEventImmediate(InputEvent{ false, InputCodes::Gamepad_Left_Thumbstick_Y_Axis, port, thumbLY });

	// Action
	// Get the current and previous states of the stick
	auto currLX{ state.Gamepad.sThumbLX / GamepadMaxStickMagnitude };
	auto prevLX{ prevState.Gamepad.sThumbLX / GamepadMaxStickMagnitude };

	auto currLY{ state.Gamepad.sThumbLY / GamepadMaxStickMagnitude };
	auto prevLY{ prevState.Gamepad.sThumbLY / GamepadMaxStickMagnitude };

	// Check if the state of the stick has changed
	if (currLX != prevLX)
	{
		// Check if the stick is pushed right
		if (currLX > 0)
		{
			// Submit input
			EventSystem::SendEventImmediate(InputEvent{ false, InputCodes::Gamepad_Left_Thumbstick_Right, port, 1.0f });
		}
		// Check if the stick is pushed left
		else if (currLX < 0)
		{
			// Submit input
			EventSystem::SendEventImmediate(InputEvent{ false, InputCodes::Gamepad_Left_Thumbstick_Left, port, 1.0f });
		}
	}

	// Check if the state of the stick has changed
	if (currLY != prevLY)
	{
		// Check if the stick is pushed up
		if (currLY > 0)
		{
			// Submit input
			EventSystem::SendEventImmediate(InputEvent{ false, InputCodes::Gamepad_Left_Thumbstick_Up, port, 1.0f });
		}
		// Check if the stick is pushed down
		else if (currLY < 0)
		{
			// Submit input
			EventSystem::SendEventImmediate(InputEvent{ false, InputCodes::Gamepad_Left_Thumbstick_Down, port, 1.0f });
		}
	}

	// Right
	// Axis
	// Calculate normalized axis value after a deadzone has been applied
	auto thumbRX{ static_cast<float>(state.Gamepad.sThumbRX) };
	auto thumbRY{ static_cast<float>(state.Gamepad.sThumbRY) };
	ApplyCircularDeadzone(
		thumbRX,
		thumbRY,
		GamepadRightStickDeadzoneRadius
	);

	// Submit inputs for the stick
	EventSystem::SendEventImmediate(InputEvent{ false, InputCodes::Gamepad_Right_Thumbstick_X_Axis, port, thumbRX });
	EventSystem::SendEventImmediate(InputEvent{ false, InputCodes::Gamepad_Right_Thumbstick_Y_Axis, port, thumbRY });

	// Action
	// Get the current and previous states of the stick
	auto currRX{ state.Gamepad.sThumbRX / GamepadMaxStickMagnitude };
	auto prevRX{ prevState.Gamepad.sThumbRX / GamepadMaxStickMagnitude };

	auto currRY{ state.Gamepad.sThumbRY / GamepadMaxStickMagnitude };
	auto prevRY{ prevState.Gamepad.sThumbRY / GamepadMaxStickMagnitude };

	// Check if the state of the stick has changed
	if (currRX != prevRX)
	{
		// Check if the stick is pushed right
		if (currRX > 0)
		{
			// Submit input
			EventSystem::SendEventImmediate(InputEvent{ false, InputCodes::Gamepad_Right_Thumbstick_Right, port, 1.0f });
		}
		// Check if the stick is pushed left
		else if (currRX < 0)
		{
			// Submit input
			EventSystem::SendEventImmediate(InputEvent{ false, InputCodes::Gamepad_Right_Thumbstick_Left, port, 1.0f });
		}
	}

	// Check if the state of the stick has changed
	if (currRY != prevRY)
	{
		// Check if the stick is pushed up
		if (currRY > 0)
		{
			// Submit input
			EventSystem::SendEventImmediate(InputEvent{ false, InputCodes::Gamepad_Right_Thumbstick_Up, port, 1.0f });
		}
		// Check if the stick is pushed down
		else if (currRY < 0)
		{
			// Submit input
			EventSystem::SendEventImmediate(InputEvent{ false, InputCodes::Gamepad_Right_Thumbstick_Down, port, 1.0f });
		}
	}
}

static void RefreshTriggers(const XINPUT_STATE& state, const XINPUT_STATE& prevState, uint32_t port)
{
	// Action
	// Left
	auto currL{ state.Gamepad.bLeftTrigger / GamepadMaxTriggerMagnitude };
	auto prevL{ prevState.Gamepad.bLeftTrigger / GamepadMaxTriggerMagnitude };
	if (prevL != currL)
	{
		EventSystem::SendEventImmediate(InputEvent{ false, InputCodes::Gamepad_Left_Trigger, port, static_cast<float>(currL) });
	}

	// Right
	auto currR{ state.Gamepad.bRightTrigger / GamepadMaxTriggerMagnitude };
	auto prevR{ prevState.Gamepad.bRightTrigger / GamepadMaxTriggerMagnitude };
	if (prevR != currR)
	{
		EventSystem::SendEventImmediate(InputEvent{ false, InputCodes::Gamepad_Right_Trigger, port, static_cast<float>(currR) });
	}

	// Axis
	// Left
	EventSystem::SendEventImmediate(InputEvent{ false, InputCodes::Gamepad_Left_Trigger_Axis, port,
		static_cast<float>(state.Gamepad.bLeftTrigger) / static_cast<float>(GamepadMaxTriggerMagnitude) });

	// Right
	EventSystem::SendEventImmediate(InputEvent{ false, InputCodes::Gamepad_Right_Trigger_Axis, port,
	static_cast<float>(state.Gamepad.bRightTrigger) / static_cast<float>(GamepadMaxTriggerMagnitude) });
}

void GamepadManager::RefreshGamepadPorts()
{
	for (size_t i = 0; i < XUSER_MAX_COUNT; ++i)
	{
		XINPUT_STATE state;
		if (!XInputGetState(static_cast<DWORD>(i), &state) == ERROR_SUCCESS)
		{
			continue;
		}

		RefreshButtons(state, PrevStates[i], static_cast<uint32_t>(i));
		RefreshThumbsticks(state, PrevStates[i], static_cast<uint32_t>(i));
		RefreshTriggers(state, PrevStates[i], static_cast<uint32_t>(i));

		PrevStates[i] = state;
	}
}
