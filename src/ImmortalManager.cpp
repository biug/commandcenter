#include "ImmortalManager.h"
#include "Util.h"
#include "CCBot.h"

ImmortalInfo::ImmortalInfo() :
	m_hpLastSecond(45)
{

}

ImmortalInfo::ImmortalInfo(float hp) :
	m_hpLastSecond(hp)
{

}

ImmortalManager::ImmortalManager(CCBot & bot)
	: MicroManager(bot)
{

}

void ImmortalManager::executeMicro(const std::vector<const sc2::Unit *> & targets)
{
	assignTargets(targets);
}

void ImmortalManager::assignTargets(const std::vector<const sc2::Unit *> & targets)
{
	const std::vector<const sc2::Unit *> & Immortals = getUnits();

	// figure out targets
	std::vector<const sc2::Unit *> ImmortalTargets;
	for (auto target : targets)
	{
		if (!target) { continue; }
		if (target->unit_type == sc2::UNIT_TYPEID::ZERG_EGG) { continue; }
		if (target->unit_type == sc2::UNIT_TYPEID::ZERG_LARVA) { continue; }

		ImmortalTargets.push_back(target);
	}

	// for each meleeUnit
	bool refreshInfo = m_bot.Map().frame() % 16;
	for (auto Immortal : Immortals)
	{
		BOT_ASSERT(Immortal, "ranged unit is null");
		if (ImmortalInfos.find(Immortal) == ImmortalInfos.end())
		{
			ImmortalInfos[Immortal] = ImmortalInfo(Immortal->health + Immortal->shield);
		}
		float currentHP = Immortal->health + Immortal->shield;
		bool beingAttack = currentHP < ImmortalInfos[Immortal].m_hpLastSecond;
		if (refreshInfo) ImmortalInfos[Immortal].m_hpLastSecond = currentHP;

		// if the order is to attack or defend
		if (order.getType() == SquadOrderTypes::Attack || order.getType() == SquadOrderTypes::Defend)
		{
			if (!ImmortalTargets.empty())
			{
				// find the best target for this meleeUnit
				const sc2::Unit * target = getTarget(Immortal, ImmortalTargets);
				if (!target) continue;
				auto abilities = m_bot.Query()->GetAbilitiesForUnit(Immortal);
				bool immortalbarrier = false;
				for (auto & ab : abilities.abilities)
				{
					if (ab.ability_id.ToType() == sc2::ABILITY_ID::EFFECT_IMMORTALBARRIER)
					{
						immortalbarrier = true;

					}
				}
				for (auto buff : Immortal->buffs) {
					if (buff == sc2::BUFF_ID::IMMORTALOVERLOAD) {
						immortalbarrier = false;
					}
				}

				bool being_attacked_fiercely = false;
				int nerbytargetnum = 0;
				for (auto nearbytarget : ImmortalTargets)
				{
					if (Util::Dist(Immortal->pos, nearbytarget->pos) <= 10) {
						nerbytargetnum++;
					}
				}
				if (beingAttack && ((ImmortalInfos[Immortal].m_hpLastSecond - currentHP >= 30) || nerbytargetnum >= 4)) {
					being_attacked_fiercely = true;
				}
				if (being_attacked_fiercely  &&  immortalbarrier)
				{
					Micro::SmartAbility(Immortal, sc2::ABILITY_ID::EFFECT_IMMORTALBARRIER, m_bot);
				}
				else { Micro::SmartAttackMove(Immortal, target->pos, m_bot); }

			}
			// if there are no targets
			else
			{
				// if we're not near the order position
				if (Util::Dist(Immortal->pos, order.getPosition()) > 4)
				{
					// move to it
					Micro::SmartMove(Immortal, order.getPosition(), m_bot);
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
const sc2::Unit * ImmortalManager::getTarget(const sc2::Unit * rangedUnit, const std::vector<const sc2::Unit *> & targets)
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
int ImmortalManager::getAttackPriority(const sc2::Unit * attacker, const sc2::Unit * unit)
{
	BOT_ASSERT(unit, "null unit in getAttackPriority");
	if (Util::IsHeavyArmor(unit))
	{
		return 12;
	}
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

