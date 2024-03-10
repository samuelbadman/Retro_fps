#pragma once

namespace Renderer
{
	struct Vertex1Pos1UV1Norm
	{
		Vertex1Pos1UV1Norm() : Pos({ 0.0f, 0.0f, 0.0f }), UV({ 0.0f, 0.0f }), Norm({ 0.0f, 0.0f, 0.0f }) {}
		Vertex1Pos1UV1Norm(const glm::vec3& pos, const glm::vec2& uv, const glm::vec3& norm) : Pos(pos), UV(uv), Norm(norm) {}

		glm::vec3 Pos;
		glm::vec2 UV;
		glm::vec3 Norm;
	};
}