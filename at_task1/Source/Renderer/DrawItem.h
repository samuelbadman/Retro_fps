#pragma once

namespace Renderer
{
	enum class ESampler : uint8_t;

	class DrawItem
	{
	public:
		DrawItem();
		DrawItem(const uint32_t geometryID, const ESampler samplerID, const uint32_t textureID, const glm::vec2& textureScale, const glm::mat4& worldMatrix);

		const uint32_t GetGeometryID() const { return GeometryID; }
		void SetGeometryID(const uint32_t id) { GeometryID = id; }

		const uint32_t GetSamplerID() const { return SamplerID; }
		void SetSamplerID(const uint32_t id) { SamplerID = id; }

		const uint32_t GetTextureID() const { return TextureID; }
		void SetTextureID(const uint32_t id) { TextureID = id; }

		const glm::vec2& GetTextureScale() const { return TextureScale; }
		void SetTextureScale(const glm::vec2& textureScale) { TextureScale = textureScale; }

		const glm::mat4& GetWorldMatrix() const { return WorldMatrix; }
		void SetWorldMatrix(const glm::mat4& worldMatrix) { WorldMatrix = worldMatrix; }

	private:
		uint32_t GeometryID{ std::numeric_limits<uint32_t>::max() };
		uint32_t SamplerID{ 0 };
		uint32_t TextureID{ 0 };
		glm::vec2 TextureScale{ 0.0f, 0.0f };
		glm::mat4 WorldMatrix{ glm::identity<glm::mat4>() };
	};
}