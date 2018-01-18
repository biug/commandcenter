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
		int currentHP = Ghost->health;
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

