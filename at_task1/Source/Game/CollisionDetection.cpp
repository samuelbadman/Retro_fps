#include "Pch.h"
#include "CollisionDetection.h"
#include "Game/Components/SphereCollisionComponent.h"
#include "Game/Components/AABBCollisionComponent.h"

bool CollisionDetection::TestSphereAABB(const glm::vec3& spherePosition, const SphereCollisionComponent& sphere,
    const glm::vec3& aabbPosition, const AABBCollisionComponent& aabb, CollisionTestResult& result)
{
    // Get aabb closest point to sphere center by clamping
    glm::vec3 aabbClosestPoint = glm::vec3(
        glm::max(aabbPosition.x - aabb.Extent.x, glm::min(spherePosition.x, aabbPosition.x + aabb.Extent.x)),
        glm::min(aabbPosition.y + aabb.Extent.y, glm::max(spherePosition.y, aabbPosition.y - aabb.Extent.y)), // Y flipped for vulkan viewport
        glm::max(aabbPosition.z - aabb.Extent.z, glm::min(spherePosition.z, aabbPosition.z + aabb.Extent.z))
    );

    // Determine if closest point is inside the sphere and there was a collision
    auto distanceSquared =
        (aabbClosestPoint.x - spherePosition.x) * (aabbClosestPoint.x - spherePosition.x) +
        (aabbClosestPoint.y - spherePosition.y) * (aabbClosestPoint.y - spherePosition.y) +
        (aabbClosestPoint.z - spherePosition.z) * (aabbClosestPoint.z - spherePosition.z);

    result.Hit = distanceSquared < (sphere.Radius * sphere.Radius);

    if (result.Hit)
    {
        // Store the hit position
        result.HitPosition = aabbClosestPoint;

        // Calculate the hit surface normal
        result.HitSurfaceNormal = glm::normalize((spherePosition - aabbClosestPoint));
    }

    return result.Hit;
}

bool CollisionDetection::TestLineAABB(const glm::vec3& lineStart,
    const glm::vec3& lineEnd,
    const glm::vec3& aabbPosition,
    const glm::vec3& aabbExtent,
    CollisionDetection::CollisionTestResult& result)
{
    // Calculate line direction
    auto lineDirection = glm::normalize(lineEnd - lineStart);

    // Test X axis
    if (lineDirection.x == 0.0f) lineDirection.x = 0.000001f;
    auto txMin = ((aabbPosition.x - aabbExtent.x) - lineStart.x) / lineDirection.x;
    auto txMax = ((aabbPosition.x + aabbExtent.x) - lineStart.x) / lineDirection.x;
    if (txMax < txMin)
    {
        auto tmp = txMax;
        txMax = txMin;
        txMin = tmp;
    }

    // Test Y axis
    if (lineDirection.y == 0.0f) lineDirection.y = 0.000001f;
    auto tyMin = ((aabbPosition.y - aabbExtent.y) - lineStart.y) / lineDirection.y;
    auto tyMax = ((aabbPosition.y + aabbExtent.y) - lineStart.y) / lineDirection.y;
    if (tyMax < tyMin)
    {
        auto tmp = tyMax;
        tyMax = tyMin;
        tyMin = tmp;
    }

    // Test Z axis
    if (lineDirection.z == 0.0f) lineDirection.z = 0.000001f;
    auto tzMin = ((aabbPosition.z - aabbExtent.z) - lineStart.z) / lineDirection.z;
    auto tzMax = ((aabbPosition.z + aabbExtent.z) - lineStart.z) / lineDirection.z;
    if (tzMax < tzMin)
    {
        auto tmp = tzMax;
        tzMax = tzMin;
        tzMin = tmp;
    }

    // Get largest min
    auto tMin = (txMin > tyMin) ? txMin : tyMin;

    // Get smallest max
    auto tMax = (txMax < tyMax) ? txMax : tyMax;

    // Check XY axis
    if ((txMin > tyMax) || (tyMin > txMax))
    {
        return false;
    }

    // Check Z axis
    if ((tMin > tzMax) || (tzMin > tMax))
    {
        return false;
    }

    if (tzMin > tMin)
    {
        tMin = tzMin;
    }

    if (tzMax < tMax)
    {
        tMax = tzMax;
    }

    // Check if the line originates inside an aabb volume
    if (tMin <= 0.0f)
    {
        // tMax is the first point of intersection along the line
        result.Hit = true;
        result.HitPosition = lineStart + (lineDirection * tMax);
    }
    else
    {
        // tMin is the first point of intersection along the line
        result.Hit = true;
        result.HitPosition = lineStart + (lineDirection * tMin);

    }

    return true;
}
