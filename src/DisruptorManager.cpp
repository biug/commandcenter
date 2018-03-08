#include "DisruptorManager.h"
#include "Util.h"
#include "CCBot.h"

DisruptorInfo::DisruptorInfo() :
	m_hpLastSecond(45)
{

}

DisruptorInfo::DisruptorInfo(float hp) :
	m_hpLastSecond(hp)
{

}

DisruptorManager::DisruptorManager(CCBot & bot)
	: MicroManager(bot)
{

}

void DisruptorManager::executeMicro(const std::vector<const sc2::Unit *> & targets)
{
	assignTargets(targets);
}


void DisruptorManager::assignTargets(const std::vector<const sc2::Unit *> & targets) {
	const std::vector<const sc2::Unit *> & Disruptors = getUnits();

	// figure out targets
	std::vector<const sc2::Unit *> DisruptorTargets;
	for (auto target : targets)
	{
		if (!target) { continue; }
		if (target->unit_type == sc2::UNIT_TYPEID::ZERG_EGG) { continue; }
		if (target->unit_type == sc2::UNIT_TYPEID::ZERG_LARVA) { continue; }

		DisruptorTargets.push_back(target);
	}

	// for each meleeUnit
	bool refreshInfo = m_bot.Map().frame() % 16;
	for (auto Disruptor : Disruptors)
	{
		BOT_ASSERT(Disruptor, "ranged unit is null");
		if (DisruptorInfos.find(Disruptor) == DisruptorInfos.end())
		{
			DisruptorInfos[Disruptor] = DisruptorInfo(Disruptor->health + Disruptor->shield);
		}
		float currentHP = Disruptor->health + Disruptor->shield;
		bool beingAttack = currentHP < DisruptorInfos[Disruptor].m_hpLastSecond;
		if (refreshInfo) DisruptorInfos[Disruptor].m_hpLastSecond = currentHP;

		// if the order is to attack or defend
		if (order.getType() == SquadOrderTypes::Attack || order.getType() == SquadOrderTypes::Defend)
		{
			if (!DisruptorTargets.empty())
			{
				// find the best target for this meleeUnit
				const sc2::Unit * target = getTarget(Disruptor, DisruptorTargets);
				if (!target) continue;
				auto abilities = m_bot.Query()->GetAbilitiesForUnit(Disruptor);
				bool purificationnova = false;
				for (auto & ab : abilities.abilities)
				{
					if (ab.ability_id.ToType() == sc2::ABILITY_ID::EFFECT_PURIFICATIONNOVA)
					{
						purificationnova = true;
					}

				}
				if (purificationnova && DisruptorTargets.size() >=4) {
					Micro::SmartAbility(Disruptor, sc2::ABILITY_ID::EFFECT_PURIFICATIONNOVA, SetPurificationnovaPosition(Disruptor, DisruptorTargets), m_bot);
				}


			}
			// if there are no targets
			else
			{
				// if we're not near the order position
				if (Util::Dist(Disruptor->pos, order.getPosition()) > 4)
				{
					// move to it
					Micro::SmartMove(Disruptor, order.getPosition(), m_bot);
				}
			}
		}
	}
}


sc2::Point2D DisruptorManager::SetPurificationnovaPosition(const sc2::Unit * rangedUnit, const std::vector<const sc2::Unit *> & targets) {
	const sc2::Unit *ClosestTarget = nullptr;
	const sc2::Unit *FarthestTarget = nullptr;
	float ClosestDistance = std::numeric_limits<float>::max();
	float FarthestDistance = 0;
	for (auto targetUnit : targets)
	{
		if (!targetUnit->is_flying)
		{
			float distance = Util::Dist(rangedUnit->pos, targetUnit->pos);
			if (distance < ClosestDistance)
			{
				ClosestDistance = distance;
				ClosestTarget = targetUnit;
			}
			else if (distance > FarthestDistance)
			{
				FarthestDistance = distance;
				FarthestTarget = targetUnit;
			}
		}

	}
	return sc2::Point2D((ClosestTarget->pos.x + FarthestTarget->pos.x) / 2, (ClosestTarget->pos.y + FarthestTarget->pos.y) / 2);
}



// get a target for the ranged unit to attack
// TODO: this is the melee targeting code, replace it with something better for ranged units
const sc2::Unit * DisruptorManager::getTarget(const sc2::Unit * rangedUnit, const std::vector<const sc2::Unit *> & targets)
{
	BOT_ASSERT(rangedUnit, "null melee unit in getTarget");

	int highPriority = 0;
	double closestDist = std::numeric_limits<double>::max();
	const sc2::Unit * closestTarget = nullptr;

	// for each target possiblity
	for (auto targetUnit : targets)
	{
		BOT_ASSERT(targetUnit, "null target unit in getTarget");

		int priority = getAttackPriority(rangedUnit, targetUnit);
		float distance = Util::Dist(rangedUnit->pos, targetUnit->pos);

		// if it's a higher priority, or it's closer, set it
		if (!closestTarget || (priority > highPriority) || (priority == highPriority && distance < closestDist))
		{
			closestDist = distance;
			highPriority = priority;
			closestTarget = targetUnit;
		}
	}

	return closestTarget;
}

// get the attack priority of a type in relation to a zergling
int DisruptorManager::getAttackPriority(const sc2::Unit * attacker, const sc2::Unit * unit)
{
	BOT_ASSERT(unit, "null unit in getAttackPriority");
	if (Util::IsPsionicUnit(unit))
	{
		return 11;
	}
	if (Util::IsMeleeUnit(unit))
	{
		return 10;
	}
	if (Util::IsCombatUnit(unit, m_bot))
	{
		return 9;
	}

	if (Util::IsWorker(unit))
	{
		return 8;
	}

	return 1;
}

