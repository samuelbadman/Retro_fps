#pragma once

class Window;

struct WindowDestroyedEvent
{
	Window* pWindow{ nullptr };
};