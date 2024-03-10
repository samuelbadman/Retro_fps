#pragma once

namespace Maths
{
	struct Transform;
}

namespace GameEvents
{
	void Init();
	void EnemyShot(const Maths::Transform& enemyTransform);
	void PlayerInteracted();
	void PlayerFired();
	void SetPlayerWithinLevelGoalRadius(bool withinRadius);
	void PlayerDefeated();
	void PlayerDamaged();
}