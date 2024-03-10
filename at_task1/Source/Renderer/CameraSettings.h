#pragma once

namespace Renderer
{
	enum class EProjectionMode : uint8_t
	{
		PERSPECTIVE = 0,
		ORTHOGRAPHIC,
	};

	struct CameraSettings
	{
		EProjectionMode ProjectionMode{ EProjectionMode::PERSPECTIVE };

		// Orthographic settings
		float AspectWidth{ 16.0f };
		float AspectHeight{ 9.0f };
		float OrthographicNearClipPlane{ 0.0f };
		float OrthographicFarClipPlane{ 100.0f };
		float OrthographicWidth{ 1.f };

		// Perspective settings
		// Field of view in degrees
		float PerspectiveFOV{ 45.0f };
		float PerspectiveNearClipPlane{ 0.1f };
		float PerspectiveFarClipPlane{ 100.0f };
	};
}