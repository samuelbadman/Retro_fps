#include "Pch.h"
#include "Window.h"
#include "EventSystem/EventSystem.h"
#include "Input/Input.h"

static LRESULT CALLBACK WindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	// Get the window pointer instance
	auto* const pWindow = reinterpret_cast<Window*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

	// Dispatch correct event for the message
	switch (msg)
	{
	case WM_MOUSEWHEEL:
	{
		const auto delta{ GET_WHEEL_DELTA_WPARAM(wParam) };
		if (delta > 0)
		{
			// Mouse wheel up
			EventSystem::SendEventImmediate(InputEvent{ false, InputCodes::Mouse_Wheel_Up, Input::MOUSE_KEYBOARD_PORT, 1.0f });
		}
		else if (delta < 0)
		{
			// Mouse wheel down
			EventSystem::SendEventImmediate(InputEvent{ false, InputCodes::Mouse_Wheel_Down, Input::MOUSE_KEYBOARD_PORT, 1.0f });
		}

		return 0;
	}

	case WM_INPUT:
	{
		// Initialize raw data size
		UINT dataSize{ 0 };

		// Retrieve the size of the raw data
		::GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, NULL, &dataSize, sizeof(RAWINPUTHEADER));

		// Return if there was no raw data
		if (dataSize == 0)
		{
			return 0;
		}

		// Initialize raw data storage
		std::unique_ptr<BYTE[]> rawData{ std::make_unique<BYTE[]>(dataSize) };

		// Retreive raw data and store it. Return if the retreived data is the not same size
		if (::GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, rawData.get(), &dataSize, sizeof(RAWINPUTHEADER)) != dataSize)
		{
			return 0;
		}

		// Cast byte to raw input
		RAWINPUT* raw{ reinterpret_cast<RAWINPUT*>(rawData.get()) };

		// Return if the raw input is not from the mouse
		if (raw->header.dwType != RIM_TYPEMOUSE)
		{
			return 0;
		}

		// Raw mouse delta
		EventSystem::SendEventImmediate(InputEvent{ false, InputCodes::Mouse_X, Input::MOUSE_KEYBOARD_PORT, static_cast<float>(raw->data.mouse.lLastX) });
		EventSystem::SendEventImmediate(InputEvent{ false, InputCodes::Mouse_Y, Input::MOUSE_KEYBOARD_PORT, static_cast<float>(raw->data.mouse.lLastY) });

		return 0;
	}

	case WM_LBUTTONDOWN:
		// Mouse left button down
		EventSystem::SendEventImmediate(InputEvent{ false, InputCodes::Left_Mouse_Button, Input::MOUSE_KEYBOARD_PORT, 1.0f });

		return 0;

	case WM_RBUTTONDOWN:
		// Mouse right button down
		EventSystem::SendEventImmediate(InputEvent{ false, InputCodes::Right_Mouse_Button, Input::MOUSE_KEYBOARD_PORT, 1.0f });

		return 0;

	case WM_MBUTTONDOWN:
		// Mouse middle button down
		EventSystem::SendEventImmediate(InputEvent{ false, InputCodes::Middle_Mouse_Button, Input::MOUSE_KEYBOARD_PORT, 1.0f });

		return 0;

	case WM_LBUTTONUP:
		// Mouse left button up
		EventSystem::SendEventImmediate(InputEvent{ false, InputCodes::Left_Mouse_Button, Input::MOUSE_KEYBOARD_PORT, 0.0f });

		return 0;

	case WM_RBUTTONUP:
		// Mouse right button up
		EventSystem::SendEventImmediate(InputEvent{ false, InputCodes::Right_Mouse_Button, Input::MOUSE_KEYBOARD_PORT, 0.0f });

		return 0;

	case WM_MBUTTONUP:
		// Mouse middle button up
		EventSystem::SendEventImmediate(InputEvent{ false, InputCodes::Middle_Mouse_Button, Input::MOUSE_KEYBOARD_PORT, 0.0f });

		return 0;

	case WM_KEYDOWN:
		// Key down
		EventSystem::SendEventImmediate(InputEvent{ static_cast<bool>(lParam & 0x40000000), static_cast<int16_t>(wParam), Input::MOUSE_KEYBOARD_PORT, 1.0f });

		return 0;

	case WM_KEYUP:
		// Key up
		EventSystem::SendEventImmediate(InputEvent{ false, static_cast<int16_t>(wParam), Input::MOUSE_KEYBOARD_PORT, 0.0f });

		return 0;

	case WM_ENTERSIZEMOVE:
		// Window move started
		EventSystem::SendEventImmediate(WindowMoveEvent{ pWindow });

		return 0;

	case WM_EXITSIZEMOVE:
		// Window move finished
		EventSystem::SendEventImmediate(WindowEndMoveEvent{ pWindow });

		return 0;

	case WM_ACTIVATEAPP:
		if (wParam == TRUE)
		{
			// Received focus
			EventSystem::SendEventImmediate(WindowReceivedFocusEvent{ pWindow });

			return 0;
		}
		else if (wParam == FALSE)
		{
			// Lost focus
			EventSystem::SendEventImmediate(WindowLostFocusEvent{ pWindow });

			return 0;
		}

		return 0;

	case WM_SIZE:
		switch (wParam)
		{
		case SIZE_MAXIMIZED:
			// Window maximized
			EventSystem::SendEventImmediate(WindowResizedEvent{ pWindow });
			EventSystem::SendEventImmediate(WindowMaximizedEvent{ pWindow });

			return 0;

		case SIZE_MINIMIZED:
			// Window minimized
			EventSystem::SendEventImmediate(WindowResizedEvent{ pWindow });
			EventSystem::SendEventImmediate(WindowMinimizedEvent{ pWindow });

			return 0;

		case SIZE_RESTORED:
			// Window restored
			EventSystem::SendEventImmediate(WindowRestoredEvent{ pWindow });
			EventSystem::SendEventImmediate(WindowResizedEvent{ pWindow });

			return 0;
		}

		return 0;

	case WM_CLOSE:
		// Window closed
		EventSystem::SendEventImmediate(WindowClosedEvent{ pWindow });
		DestroyWindow(hWnd);

		return 0;

	case WM_DESTROY:
		// Window destroyed
		EventSystem::SendEventImmediate(WindowDestroyedEvent{ pWindow });
		PostQuitMessage(0);

		return 0;

	default:
		return DefWindowProc(hWnd, msg, wParam, lParam);
	}
}

static LRESULT CALLBACK InitWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_NCCREATE:
	{
		// Get the create parameters
		const CREATESTRUCTW* const pCreate{ reinterpret_cast<CREATESTRUCTW*>(lParam) };

		// Get the window instance pointer from the create parameters
		auto* pWindow{ reinterpret_cast<Window*>(pCreate->lpCreateParams) };

		// Set the window instance pointer
		SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pWindow));

		// Set the window procedure pointer
		SetWindowLongPtr(hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(WindowProc));

		// Call the window procedure
		return WindowProc(hwnd, uMsg, wParam, lParam);
	}
	}
	return 0;
}

bool Window::Init(const glm::vec2& resolution, HWND parentWindowHandle, const std::wstring& className, const std::wstring& title, const EWindowStyle style)
{
	auto hInstance{ GetModuleHandle(nullptr) };
	auto* classNameAsCStr{ className.c_str() };

	// Register window class
	WNDCLASSEX windowClass{};
	windowClass.cbSize = sizeof(WNDCLASSEX);
	windowClass.style = CS_HREDRAW | CS_VREDRAW;
	windowClass.lpfnWndProc = &InitWindowProc;
	windowClass.cbClsExtra = 0;
	windowClass.cbWndExtra = 0;
	windowClass.hInstance = hInstance;
	windowClass.hIcon = ::LoadIcon(hInstance, IDI_APPLICATION);
	windowClass.hCursor = ::LoadCursor(NULL, IDC_ARROW);
	windowClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	windowClass.lpszMenuName = NULL;
	windowClass.lpszClassName = classNameAsCStr;
	windowClass.hIconSm = ::LoadIcon(hInstance, IDI_APPLICATION);

	if (!(::RegisterClassExW(&windowClass) > 0))
	{
		return false;
	}

	// Create window
	Handle = CreateWindowExW(0, classNameAsCStr, title.c_str(), GetWindowStyleFlag(style),
		CW_USEDEFAULT, CW_USEDEFAULT, static_cast<int>(resolution.x), static_cast<int>(resolution.y), parentWindowHandle, nullptr, hInstance, this);

	if (Handle == nullptr)
	{
		return false;
	}

	// Set the resolution of the window
	if (!SetResolution(resolution))
	{
		return false;
	}

	// Set the init style
	InitStyle = style;

	// Register raw input devices
	RAWINPUTDEVICE rid{};
	rid.usUsagePage = 0x01;
	rid.usUsage = 0x02;
	rid.dwFlags = 0;
	rid.hwndTarget = nullptr;
	if (!RegisterRawInputDevices(&rid, 1, sizeof(rid)))
	{
		return false;
	}

	return true;
}

bool Window::SetResolution(const glm::vec2& resolution)
{
	// Retreive info about the monitor the window is on
	HMONITOR hMon{ ::MonitorFromWindow(static_cast<HWND>(Handle), MONITOR_DEFAULTTONEAREST) };
	MONITORINFO monitorInfo{ sizeof(monitorInfo) };
	if (!::GetMonitorInfo(hMon, &monitorInfo))
	{
		return false;
	}

	// Calculate width and height of the monitor
	auto monitorWidth{ monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left };
	auto monitorHeight{ monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top };

	// Position the window in the center of the monitor
	return ::SetWindowPos(static_cast<HWND>(Handle),
		HWND_TOP,
		(monitorWidth / 2) - (static_cast<long>(resolution.x) / 2), (monitorHeight / 2) - (static_cast<long>(resolution.y) / 2),
		static_cast<int>(resolution.x), static_cast<int>(resolution.y),
		SWP_FRAMECHANGED | SWP_NOACTIVATE) != 0;
}

bool Window::Close()
{
	return ::DestroyWindow(Handle) != 0;
}

void Window::Show()
{
	::ShowWindow(Handle, SW_NORMAL);
}

bool Window::GetWindowAreaRect(RECT& rect)
{
	if (!::GetWindowRect(Handle, &rect))
	{
		return false;
	}

	return true;
}

bool Window::GetClientAreaRect(RECT& rect)
{
	if (!::GetClientRect(Handle, &rect))
	{
		return false;
	}

	return true;
}

bool Window::SetFullscreen(bool fullscreen)
{
	if (fullscreen)
	{
		if (!Fullscreen)
		{
			if (!EnterFullscreen())
			{
				return false;
			}
		}
	}
	else
	{
		if (Fullscreen)
		{
			if (!EnterWindowed())
			{
				return false;
			}
		}
	}

	return true;
}

bool Window::CaptureCursor()
{
	RECT clientRect;
	if (!Window::GetClientAreaRect(clientRect))
	{
		return false;
	}

	POINT ul;
	ul.x = clientRect.left;
	ul.y = clientRect.top;

	POINT lr;
	lr.x = clientRect.right;
	lr.y = clientRect.bottom;

	MapWindowPoints(Handle, nullptr, &ul, 1);
	MapWindowPoints(Handle, nullptr, &lr, 1);

	clientRect.left = ul.x;
	clientRect.top = ul.y;

	clientRect.right = lr.x;
	clientRect.bottom = lr.y;

	return ClipCursor(&clientRect) != 0;
}

uint32_t Window::GetWindowStyleFlag(const EWindowStyle style) const
{
	switch (style)
	{
	case EWindowStyle::NO_RESIZE_WINDOWED: return NO_RESIZE_WINDOW_STYLE;
	case EWindowStyle::ONLY_MAXIMIZE_WINDOWED: return ONLY_MAXIMIZE_WINDOW_STYLE;
	case EWindowStyle::RESIZEABLE_WINDOWED: return WINDOWED_WINDOW_STYLE;
	case EWindowStyle::BORDERLESS: return BORDERLESS_WINDOW_STYLE;
	default:
		return WINDOWED_WINDOW_STYLE;
	}
}

bool Window::EnterFullscreen()
{
	// Store the current window area rect
	if (!GetWindowAreaRect(WindowRectAtEnterFullscreen))
	{
		return false;
	}

	// Retreive info about the monitor the window is on
	HMONITOR hMon{ ::MonitorFromWindow(Handle, MONITOR_DEFAULTTONEAREST) };
	MONITORINFO monitorInfo = { sizeof(monitorInfo) };
	if (!::GetMonitorInfo(hMon, &monitorInfo))
	{
		return false;
	}

	// Calculate width and height of the monitor
	auto fWidth = monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left;
	auto fHeight = monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top;

	// Update position and size of the window
	if (::SetWindowPos(Handle,
		HWND_TOP, monitorInfo.rcMonitor.left, monitorInfo.rcMonitor.top, fWidth, fHeight,
		SWP_FRAMECHANGED | SWP_NOACTIVATE) == 0)
	{
		return false;
	}

	// Update window style
	if (::SetWindowLong(Handle, GWL_STYLE, 0) == 0)
	{
		return false;
	}

	// Show the window maximized
	::ShowWindow(Handle, SW_MAXIMIZE);

	Fullscreen = true;

	return true;
}

bool Window::EnterWindowed()
{
	// Update position and size of the window
	if (::SetWindowPos(Handle,
		HWND_TOP, WindowRectAtEnterFullscreen.left, WindowRectAtEnterFullscreen.top,
		WindowRectAtEnterFullscreen.right - WindowRectAtEnterFullscreen.left, WindowRectAtEnterFullscreen.bottom - WindowRectAtEnterFullscreen.top,
		SWP_FRAMECHANGED | SWP_NOACTIVATE) == 0)
	{
		return false;
	}

	// Update window style
	if (::SetWindowLong(Handle, GWL_STYLE, GetWindowStyleFlag(InitStyle)) == 0)
	{
		return false;
	}

	// Show the window
	::ShowWindow(Handle, SW_SHOW);

	Fullscreen = false;

	return true;
}
