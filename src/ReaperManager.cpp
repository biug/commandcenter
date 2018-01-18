#include "ReaperManager.h"
#include "Util.h"
#include "CCBot.h"

ReaperInfo::ReaperInfo() :
	m_hpLastSecond(45)
{

}

ReaperInfo::ReaperInfo(float hp) :
	m_hpLastSecond(hp)
{

}

ReaperManager::ReaperManager(CCBot & bot)
	: MicroManager(bot)
{

}

void ReaperManager::executeMicro(const std::vector<const sc2::Unit *> & targets)
{
	assignTargets(targets);
}

void ReaperManager::assignTargets(const std::vector<const sc2::Unit *> & targets)
{
	const std::vector<const sc2::Unit *> & Reapers = getUnits();

	// figure out targets
	std::vector<const sc2::Unit *> ReaperTargets;
	for (auto target : targets)
	{
		if (!target) { continue; }
		if (target->unit_type == sc2::UNIT_TYPEID::ZERG_EGG) { continue; }
		if (target->unit_type == sc2::UNIT_TYPEID::ZERG_LARVA) { continue; }

		ReaperTargets.push_back(target);
	}

	// for each meleeUnit
	bool refreshInfo = m_bot.Map().frame() % 16;
	for (auto Reaper : Reapers)
	{
		BOT_ASSERT(Reaper, "ranged unit is null");
		if (ReaperInfos.find(Reaper) == ReaperInfos.end())
		{
			ReaperInfos[Reaper] = ReaperInfo(Reaper->health);
		}
		int currentHP = Reaper->health;
		bool beingAttack = currentHP < ReaperInfos[Reaper].m_hpLastSecond;
		if (refreshInfo) ReaperInfos[Reaper].m_hpLastSecond = currentHP;

		// if the order is to attack or defend
		if (order.getType() == SquadOrderTypes::Attack || order.getType() == SquadOrderTypes::Defend)
		{
			if (!ReaperTargets.empty())
			{
				// find the best target for this meleeUnit
				const sc2::Unit * target = getTarget(Reaper, ReaperTargets);
				if (!target) continue;
				if (Util::IsWorker(target)) {
					Micro::SmartAttackMove(Reaper, target->pos, m_bot);
				}
				// kite attack it
				else if (Util::IsMeleeUnit(target) && Reaper->weapon_cooldown > 0 || beingAttack) {
					auto p1 = target->pos, p2 = Reaper->pos;
					auto tp = p2 * 3 - p1;
					Micro::SmartMove(Reaper, tp, m_bot);
				}
				else {
					Micro::SmartAttackMove(Reaper, target->pos, m_bot);
				}
			}
			// if there are no targets
			else
			{
				// if we're not near the order position
				if (Util::Dist(Reaper->pos, order.getPosition()) > 4)
				{
					// move to it
					Micro::SmartMove(Reaper, order.getPosition(), m_bot);
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
const sc2::Unit * ReaperManager::getTarget(const sc2::Unit * rangedUnit, const std::vector<const sc2::Unit *> & targets)
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
int ReaperManager::getAttackPriority(const sc2::Unit * attacker, const sc2::Unit * unit)
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
		return 12;
	}

	return 1;
}

