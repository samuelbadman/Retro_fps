#include "Pch.h"
#include "Maths.h"

// Converts a euler rotation described in degrees to a rotation matrix
static glm::mat3 ToRotationMatrix3(const glm::vec3& rotation)
{
	// Convert transform rotations from degrees to radians 
	glm::vec3 eulerRotationsRadians = {
		glm::radians(rotation.x),
		glm::radians(rotation.y),
		glm::radians(rotation.z) };

	// Build a rotation quaternion to use as rotation matrix from euler rotations expressed in radians
	return glm::mat3_cast(glm::quat(eulerRotationsRadians));
}

// Converts a euler rotation described in degrees to a rotation matrix
static glm::mat4 ToRotationMatrix4(const glm::vec3& rotation)
{
	// Convert transform rotations from degrees to radians 
	glm::vec3 eulerRotationsRadians = {
		glm::radians(rotation.x),
		glm::radians(rotation.y),
		glm::radians(rotation.z) };

	// Build a rotation quaternion to use as rotation matrix from euler rotations expressed in radians
	return glm::mat4_cast(glm::quat(eulerRotationsRadians));
}

glm::mat4 Maths::CalculateWorldMatrix(const Transform& transform)
{
	// Flip y axis of the translation
	//auto translation = transform.Position;
	//translation.y = -translation.y;

	return glm::translate(glm::identity<glm::mat4>(), transform.Position) * // Translation matrix
		ToRotationMatrix4(transform.Rotation) * // Rotation matrix
		glm::scale(glm::identity<glm::mat4>(), transform.Scale); // Scale matrix
}

glm::mat4 Maths::CalculateViewMatrix(const glm::vec3& viewPosition, const glm::vec3& viewRotation)
{
	return	glm::inverse(glm::translate(glm::identity<glm::mat4>(), viewPosition) *
		ToRotationMatrix4(viewRotation)); // Inverse of the view translation rotation matrix
}

glm::mat4 Maths::CalculatePerspectiveProjectionMatrix(const float fov, const float viewportWidth, const float viewportHeight, const float nearClipPlane, const float farClipPlane)
{
	return glm::perspectiveFovLH(glm::radians(fov), viewportWidth, viewportHeight, nearClipPlane, farClipPlane);
}

glm::mat4 Maths::CalculateOrthographicProjectionMatrix(const float aspectWidth, const float aspectHeight, const float nearClipPlane, const float farClipPlane)
{
	return glm::orthoLH(-aspectWidth, aspectWidth, -aspectHeight, aspectHeight, nearClipPlane, farClipPlane);
}

glm::vec3 Maths::CalculateAABBExtent(const glm::vec3& baseExtent, const Transform& transform)
{
	// Resize the extent of the bounding box based on the transform
	return glm::abs(baseExtent * (transform.Scale * ToRotationMatrix3(transform.Rotation)));
}

glm::vec3 Maths::RotationMatrix4ToEuler(const glm::mat4& matrix)
{
	// Cast rotation matrix to quaternion
	glm::quat quaternion = glm::quat_cast(matrix);

	// Convert quaternion to euler
	glm::vec3 euler = glm::eulerAngles(quaternion);

	// Return euler rotation expressed in degrees
	return glm::vec3(
		glm::degrees(euler.x),
		glm::degrees(euler.y),
		glm::degrees(euler.z)
	);
}

glm::vec3 Maths::RotateVector(const glm::vec3& rotation, const glm::vec3& vector)
{
	glm::vec3 eulerRotationRadians = {
		glm::radians(rotation.x),
		glm::radians(rotation.y),
		glm::radians(rotation.z) 
	};

	glm::quat rotationQuaternion = glm::quat(eulerRotationRadians);
	glm::mat3 rotationMatrix = glm::mat3_cast(rotationQuaternion);

	// Rotate the vector to match the rotation
	return rotationMatrix * vector;
}

float Maths::Lerp(float a, float b, float alpha)
{
	return a * (1.0f - alpha) + b * alpha;
}
