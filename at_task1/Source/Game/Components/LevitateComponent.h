#pragma once

struct LevitateComponent
{
	float LevitateRate{ 0.0002f };
	float MaxHeightDelta{ 0.75f };
	float OriginalHeight{ 0.0f };
	bool MovingUp{ false };
	glm::vec3 RotationDelta{ 0.0f, 0.0f, 0.0f };
};