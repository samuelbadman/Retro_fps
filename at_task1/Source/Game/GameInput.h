#pragma once

class Entity;
struct InputEvent;

namespace GameInput
{
	void HandleInputEvent(InputEvent&& event);
	void PollInputs();
}