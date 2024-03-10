#pragma once

namespace Renderer
{
	class DirectionalLight
	{
	public:
		const glm::vec3& GetColor() const { return Color; }
		const glm::vec3& GetDirection() const { return WorldSpaceDirection; }
		const float GetIntensity() const { return Intensity; }

	private:
		glm::vec3 Color{ 1.0f,1.0f,1.0f };
		glm::vec3 WorldSpaceDirection{ 1.0f, 0.4f, 0.2f };
		float Intensity{ 1.0f };
	};
}