#include "Pch.h"
#include "DrawItem.h"

Renderer::DrawItem::DrawItem()
	: GeometryID(std::numeric_limits<uint32_t>::max()), WorldMatrix(glm::identity<glm::mat4>())
{
}

Renderer::DrawItem::DrawItem(const uint32_t geometryID, const ESampler samplerID, const uint32_t textureID, const glm::vec2& textureScale, const glm::mat4& worldMatrix)
	: GeometryID(geometryID), SamplerID(static_cast<uint32_t>(samplerID)), TextureID(textureID), TextureScale(textureScale), WorldMatrix(worldMatrix)
{
}
