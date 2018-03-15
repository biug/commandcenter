#include "GhostManager.h"
#include "Util.h"
#include "CCBot.h"

GhostInfo::GhostInfo() :
	m_hpLastSecond(150)
{

}

GhostInfo::GhostInfo(float hp) :
	m_hpLastSecond(hp)
{

}

GhostManager::GhostManager(CCBot & bot)
	: MicroManager(bot)
{

}

void GhostManager::executeMicro(const std::vector<const sc2::Unit *> & targets)
{
	assignTargets(targets);
}

void GhostManager::assignTargets(const std::vector<const sc2::Unit *> & targets)
{
	const std::vector<const sc2::Unit *> & Ghosts = getUnits();

	// figure out targets
	std::vector<const sc2::Unit *> GhostTargets;
	for (auto target : targets)
	{
		if (!target) { continue; }
		if (target->unit_type == sc2::UNIT_TYPEID::ZERG_EGG) { continue; }
		if (target->unit_type == sc2::UNIT_TYPEID::ZERG_LARVA) { continue; }

		GhostTargets.push_back(target);
	}

	// for each meleeUnit
	bool refreshInfo = m_bot.Map().frame() % 16;
	for (auto Ghost : Ghosts)
	{
		BOT_ASSERT(Ghost, "ranged unit is null");
		if (GhostInfos.find(Ghost) == GhostInfos.end())
		{
			GhostInfos[Ghost] = GhostInfo(Ghost->health);
		}
		float currentHP = Ghost->health;
		bool beingAttack = currentHP < GhostInfos[Ghost].m_hpLastSecond;
		if (refreshInfo) GhostInfos[Ghost].m_hpLastSecond = currentHP;

		// if the order is to attack or defend
		if (order.getType() == SquadOrderTypes::Attack)
		{
			if (!GhostTargets.empty())
			{
				// find the best target for this meleeUnit
				const sc2::Unit * target = getTarget(Ghost, GhostTargets);
				if (!target) continue;

				float distance = Util::Dist(Ghost->pos, target->pos);
				// kite attack it

			}

			// if there are no targets
			else
			{
				// if we're not near the order position
				if (Util::Dist(Ghost->pos, order.getPosition()) > 4)
				{
					// move to it
					Micro::SmartMove(Ghost, order.getPosition(), m_bot);
				}
			}
		}


		if (m_bot.Config().DrawUnitTargetInfo)
		{
			// TODO: draw the line to the unit's target
		}
	}
}

// get a target for the ranged unit to attack
// TODO: this is the melee targeting code, replace it with something better for ranged units
const sc2::Unit * GhostManager::getTarget(const sc2::Unit * rangedUnit, const std::vector<const sc2::Unit *> & targets)
{
	BOT_ASSERT(rangedUnit, "null melee unit in getTarget");

	int highPriorityFar = 0;
	int highPriorityNear = 0;
	double closestDist = std::numeric_limits<double>::max();
	const sc2::Unit * closestTargetOutsideRange = nullptr;
	const sc2::Unit * weakestTargetInsideRange = nullptr;
	double lowestHealth = std::numeric_limits<double>::max();

	// for each target possiblity
	for (auto targetUnit : targets)
	{
		BOT_ASSERT(targetUnit, "null target unit in getTarget");

		int priority = getAttackPriority(rangedUnit, targetUnit);
		float distance = Util::Dist(rangedUnit->pos, targetUnit->pos);
		float range = Util::GetAttackRange(rangedUnit->unit_type, m_bot);
		if (distance > range)
		{
			if (distance <= Util::GetSightRange(rangedUnit->unit_type, m_bot))
			{
				priority += 20;
			}
			if (!closestTargetOutsideRange || (priority > highPriorityFar) || (priority == highPriorityFar && distance < closestDist))
			{
				closestDist = distance;
				highPriorityFar = priority;
				closestTargetOutsideRange = targetUnit;
			}
		}
		else
		{
			if (!weakestTargetInsideRange || (priority > highPriorityNear) || (priority == highPriorityNear && (targetUnit->health + targetUnit->shield <lowestHealth)))
			{
				closestDist = distance;
				highPriorityNear = priority;
				weakestTargetInsideRange = targetUnit;
			}

		}

	}

	return weakestTargetInsideRange && highPriorityNear>1 ? weakestTargetInsideRange : closestTargetOutsideRange;

}

// get the attack priority of a type in relation to a zergling
int GhostManager::getAttackPriority(const sc2::Unit * attacker, const sc2::Unit * unit)
{
	BOT_ASSERT(unit, "null unit in getAttackPriority");
	if (Util::IsHeavyArmor(unit))
	{
		return 11;
	}
	if (Util::IsCombatUnit(unit, m_bot))
	{
		return 10;
	}

	if (Util::IsWorker(unit))
	{
		return 9;
	}

	return 1;
}

