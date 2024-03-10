#pragma once

struct EnemyAIComponent
{
	bool Active{ true };
	bool FirstAttack{ true };
	float AttackRadius{ 1.0f };
	std::chrono::high_resolution_clock::time_point LastAttackTime{};
	float AttackRateSeconds{ 1.25f };
	float AttackDamage{ 33.3f };
};