#pragma once

#include "Renderer/Material.h"

struct StaticMeshComponent
{
	bool Visible{ true };
	uint32_t GeometryID{ std::numeric_limits<uint32_t>::max() };
	Renderer::Material Material{};
};