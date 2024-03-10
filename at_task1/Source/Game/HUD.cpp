#include "Pch.h"
#include "HUD.h"
#include "Renderer/Renderer.h"

bool HUD::Load()
{
	// Load plane geometry for HUD images
	if (!Renderer::LoadPlaneGeometryPrimitive(1.0f, &ImageGeometryID))
	{
		return false;
	}

	return true;
}

void HUD::UnLoad()
{
	// Destroy plane geometry
	Renderer::DestroyGeometry(ImageGeometryID);
}

Entity* HUD::CreateHUDEntity()
{
	return &HUDEntities.emplace_back(ECSRegistry.create(), &ECSRegistry);
}
