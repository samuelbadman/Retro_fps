#pragma once

struct SphereCollisionComponent;
struct AABBCollisionComponent;

namespace CollisionDetection
{
	struct CollisionTestResult
	{
		bool Hit{ false };
		glm::vec3 HitSurfaceNormal{ 0.0f, 0.0f, 0.0f };
		glm::vec3 HitPosition{ 0.0f, 0.0f, 0.0f };
	};

	bool TestSphereAABB(const glm::vec3& spherePosition, const SphereCollisionComponent& sphere, 
		const glm::vec3& aabbPosition, const AABBCollisionComponent& aabb, CollisionTestResult& result);
	bool TestLineAABB(const glm::vec3& lineStart,
		const glm::vec3& lineEnd,
		const glm::vec3& aabbPosition,
		const glm::vec3& aabbExtent,
		CollisionDetection::CollisionTestResult& result);
}