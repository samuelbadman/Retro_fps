#pragma once

enum class EWindowStyle : uint8_t
{
	RESIZEABLE_WINDOWED = 0,
	NO_RESIZE_WINDOWED,
	ONLY_MAXIMIZE_WINDOWED,
	BORDERLESS,
};

class Window
{
public:
	bool Init(const glm::vec2& resolution, HWND parentWindowHandle, const std::wstring& className, const std::wstring& title, const EWindowStyle style);
	bool SetResolution(const glm::vec2& resolution);
	bool Close();
	void Show();
	bool GetWindowAreaRect(RECT& rect);
	bool GetClientAreaRect(RECT& rect);
	HWND GetHandle() const { return Handle; }
	bool SetFullscreen(bool fullscreen);
	bool CaptureCursor();

private:
	uint32_t GetWindowStyleFlag(const EWindowStyle style) const;
	bool EnterFullscreen();
	bool EnterWindowed();

private:
	// Fully featured window
	static constexpr auto WINDOWED_WINDOW_STYLE = WS_OVERLAPPEDWINDOW;
	// Removes resizing feature
	static constexpr auto NO_RESIZE_WINDOW_STYLE = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX /*| WS_MAXIMIZEBOX*/;
	// Removes the sizing box restricting drag resizing but retaining the maximize button
	static constexpr auto ONLY_MAXIMIZE_WINDOW_STYLE = WS_OVERLAPPEDWINDOW ^ WS_THICKFRAME;
	// Borderless game window
	static constexpr auto BORDERLESS_WINDOW_STYLE = WS_POPUPWINDOW;

	// Fullscreen flag
	bool Fullscreen{ false };
	// The Window style the window was initialised with. Defaults to windowed
	EWindowStyle InitStyle{ EWindowStyle::RESIZEABLE_WINDOWED };
	// Window handle
	HWND Handle;
	// The window rect before the window last entered fullscreen
	RECT WindowRectAtEnterFullscreen{};
};