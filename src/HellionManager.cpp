#include "HellionManager.h"
#include "Util.h"
#include "CCBot.h"

HellionInfo::HellionInfo() :
	m_hpLastSecond(150)
{

}

HellionInfo::HellionInfo(float hp) :
	m_hpLastSecond(hp)
{

}

HellionManager::HellionManager(CCBot & bot)
	: MicroManager(bot)
{

}

void HellionManager::executeMicro(const std::vector<const sc2::Unit *> & targets)
{
	assignTargets(targets);
}

void HellionManager::assignTargets(const std::vector<const sc2::Unit *> & targets)
{
	const std::vector<const sc2::Unit *> & Hellions = getUnits();

	// figure out targets
	std::vector<const sc2::Unit *> HellionTargets;
	for (auto target : targets)
	{
		if (!target) { continue; }
		if (target->unit_type == sc2::UNIT_TYPEID::ZERG_EGG) { continue; }
		if (target->unit_type == sc2::UNIT_TYPEID::ZERG_LARVA) { continue; }

		HellionTargets.push_back(target);
	}

	// for each meleeUnit
	bool refreshInfo = m_bot.Map().frame() % 16;
	for (auto hellion : Hellions)
	{
		BOT_ASSERT(hellion, "ranged unit is null");
		if (HellionInfos.find(hellion) == HellionInfos.end())
		{
			HellionInfos[hellion] = HellionInfo(hellion->health);
		}
		float currentHP = hellion->health;
		bool beingAttack = currentHP < HellionInfos[hellion].m_hpLastSecond;
		if (refreshInfo) HellionInfos[hellion].m_hpLastSecond = currentHP;

		// if the order is to attack or defend
		if (order.getType() == SquadOrderTypes::Attack || order.getType() == SquadOrderTypes::Defend)
		{
			if (!HellionTargets.empty())
			{
				// find the best target for this meleeUnit
				const sc2::Unit * target = getTarget(hellion, HellionTargets);
				if (!target) continue;

				float distance = Util::Dist(hellion->pos, target->pos);
				if (hellion->unit_type.ToType() == sc2::UNIT_TYPEID::TERRAN_HELLION && target->unit_type.ToType() == sc2::UNIT_TYPEID::ZERG_ZERGLING && distance < 8.0) {
					Micro::SmartAbility(hellion, sc2::ABILITY_ID::MORPH_HELLBAT, m_bot);
				}
				// kite attack it
				if (Util::IsMeleeUnit(target) && hellion->weapon_cooldown > 0) {
					auto p1 = target->pos, p2 = hellion->pos;
					auto tp = p2 * 2 - p1;
					Micro::SmartMove(hellion, tp, m_bot);
				}
				else {
					Micro::SmartAttackMove(hellion, target->pos, m_bot);
				}
			}
			// if there are no targets
			else
			{
				// if we're not near the order position
				if (Util::Dist(hellion->pos, order.getPosition()) > 4)
				{
					// move to it
					Micro::SmartMove(hellion, order.getPosition(), m_bot);
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
const sc2::Unit * HellionManager::getTarget(const sc2::Unit * rangedUnit, const std::vector<const sc2::Unit *> & targets)
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
int HellionManager::getAttackPriority(const sc2::Unit * attacker, const sc2::Unit * unit)
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

