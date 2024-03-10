#pragma once

#include "Transform.h"

namespace Maths
{
	glm::mat4 CalculateWorldMatrix(const Transform& transform);
	glm::mat4 CalculateViewMatrix(const glm::vec3& viewPosition, const glm::vec3& viewRotation);
	// FOV expressed in degrees
	glm::mat4 CalculatePerspectiveProjectionMatrix(const float fov, const float viewportWidth, const float viewportHeight, const float nearClipPlane, const float farClipPlane);
	glm::mat4 CalculateOrthographicProjectionMatrix(const float aspectWidth, const float aspectHeight, const float nearClipPlane, const float farClipPlane);
	// Returns the AABB extent with the transform applied
	glm::vec3 CalculateAABBExtent(const glm::vec3& baseExtent, const Transform& transform);
	// Returns an euler rotation expressed in degrees
	glm::vec3 RotationMatrix4ToEuler(const glm::mat4& matrix);
	glm::vec3 RotateVector(const glm::vec3& rotation, const glm::vec3& vector);
	float Lerp(float a, float b, float alpha);
}