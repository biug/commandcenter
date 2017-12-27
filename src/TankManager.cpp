#include "TankManager.h"
#include "Util.h"
#include "CCBot.h"

TankInfo::TankInfo() :
m_hpLastSecond(45)
{
	
}

TankInfo::TankInfo(float hp) :
	m_hpLastSecond(hp)
{

}

TankManager::TankManager(CCBot & bot)
	: MicroManager(bot)
{

}

void TankManager::executeMicro(const std::vector<const sc2::Unit *> & targets)
{
	assignTargets(targets);
}

void TankManager::assignTargets(const std::vector<const sc2::Unit *> & targets)
{
	const std::vector<const sc2::Unit *> & tanks = getUnits();

	// figure out targets
	std::vector<const sc2::Unit *> tankTargets;
	for (auto target : targets)
	{
		if (!target) { continue; }
		if (target->unit_type == sc2::UNIT_TYPEID::ZERG_EGG) { continue; }
		if (target->unit_type == sc2::UNIT_TYPEID::ZERG_LARVA) { continue; }

		tankTargets.push_back(target);
	}

	// for each meleeUnit
	bool refreshInfo = m_bot.Map().frame() % 16;
	for (auto tank : tanks)
	{
		BOT_ASSERT(tank, "ranged unit is null");
		if (tankInfos.find(tank) == tankInfos.end())
		{
			tankInfos[tank] = TankInfo(tank->health);
		}
		int currentHP = tank->health;
		bool beingAttack = currentHP < tankInfos[tank].m_hpLastSecond;
		if (refreshInfo) tankInfos[tank].m_hpLastSecond = currentHP;
		
		// if the order is to attack or defend
		if (order.getType() == SquadOrderTypes::Attack )
		{
			if (!tankTargets.empty())
			{
				// find the best target for this meleeUnit
				const sc2::Unit * target = getTarget(tank, tankTargets);
				if (!target) continue;

				float distance = Util::Dist(tank->pos, target->pos);
				if (distance < 8.0) {
					Micro::SmartAbility(tank, sc2::ABILITY_ID::MORPH_SIEGEMODE, m_bot);
				}
				// kite attack it
				Micro::SmartAttackMove(tank, target->pos, m_bot);
			}
			
			// if there are no targets
			else
			{
				// if we're not near the order position
				if (Util::Dist(tank->pos, order.getPosition()) > 4)
				{
					// move to it
					Micro::SmartMove(tank, order.getPosition(), m_bot);
				}
			}
		}
		else if (order.getType() == SquadOrderTypes::Defend)
		{
			Micro::SmartAbility(tank, sc2::ABILITY_ID::MORPH_SIEGEMODE,m_bot);
		}

		if (m_bot.Config().DrawUnitTargetInfo)
		{
			// TODO: draw the line to the unit's target
		}
	}
}

// get a target for the ranged unit to attack
// TODO: this is the melee targeting code, replace it with something better for ranged units
const sc2::Unit * TankManager::getTarget(const sc2::Unit * rangedUnit, const std::vector<const sc2::Unit *> & targets)
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
int TankManager::getAttackPriority(const sc2::Unit * attacker, const sc2::Unit * unit)
{
	BOT_ASSERT(unit, "null unit in getAttackPriority");
	if (Util::IsPsionicUnit(unit))
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

