#pragma once

namespace Renderer
{
	struct Vertex1Pos1UV
	{
		Vertex1Pos1UV() : Pos({ 0.0f, 0.0f, 0.0f }), UV({ 0.0f, 0.0f }) {}
		Vertex1Pos1UV(const glm::vec3& pos, const glm::vec2& uv) : Pos(pos), UV(uv) {}

		glm::vec3 Pos;
		glm::vec2 UV;
	};
}