#pragma once

namespace Renderer
{
	enum class ESampler : uint8_t;

	struct Material
	{
		void SetSamplerID(const Renderer::ESampler id) { SamplerID = static_cast<uint32_t>(id); }

		bool AlphaBlended{ false };
		uint32_t SamplerID{ 0 };
		uint32_t TextureID{ 0 };
		glm::vec2 TextureScale{ 1.0f, 1.0f };
	};
}