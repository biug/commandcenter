#include "QueenManager.h"
#include "Util.h"
#include "CCBot.h"

QueenInfo::QueenInfo() :
m_hpLastSecond(45)
{
	
}

QueenInfo::QueenInfo(float hp) :
	m_hpLastSecond(hp)
{

}

QueenManager::QueenManager(CCBot & bot)
	: MicroManager(bot)
{

}

void QueenManager::executeMicro(const std::vector<const sc2::Unit *> & targets)
{
	assignTargets(targets);
}

void QueenManager::assignTargets(const std::vector<const sc2::Unit *> & targets)
{
	const std::vector<const sc2::Unit *> & Queens = getUnits();

	// figure out targets
	std::vector<const sc2::Unit *> QueenTargets;
	for (auto target : targets)
	{
		if (!target) { continue; }
		if (target->unit_type == sc2::UNIT_TYPEID::ZERG_EGG) { continue; }
		if (target->unit_type == sc2::UNIT_TYPEID::ZERG_LARVA) { continue; }

		QueenTargets.push_back(target);
	}

	// for each meleeUnit
	bool refreshInfo = m_bot.Map().frame() % 16;
	for (auto Queen : Queens)
	{
		BOT_ASSERT(Queen, "ranged unit is null");
		if (QueenInfos.find(Queen) == QueenInfos.end())
		{
			QueenInfos[Queen] = QueenInfo(Queen->health);
		}
		float currentHP = Queen->health;
		bool beingAttack = currentHP < QueenInfos[Queen].m_hpLastSecond;
		if (refreshInfo) QueenInfos[Queen].m_hpLastSecond = currentHP;
		
		// if the order is to attack or defend
		if (order.getType() == SquadOrderTypes::Attack || order.getType() == SquadOrderTypes::Defend)
		{
			if (!QueenTargets.empty())
			{
				// find the best target for this meleeUnit
				const sc2::Unit * target = getTarget(Queen, QueenTargets);
				if (!target) continue;
				
				if (m_bot.State().m_stimpack)
				{
					auto abilities = m_bot.Query()->GetAbilitiesForUnit(Queen);
					bool stimpack = false;
					
					for (auto & ab : abilities.abilities)
					{
						if (ab.ability_id.ToType() == sc2::ABILITY_ID::EFFECT_STIM)
						{
							stimpack = true;
							
						}
					}
					for (auto buff : Queen->buffs) {
						if (buff == sc2::BUFF_ID::STIMPACK) {
							stimpack = false;
						}
					}
					if (stimpack && (beingAttack || Queen->weapon_cooldown >0))
					{
						Micro::SmartAbility(Queen, sc2::ABILITY_ID::EFFECT_STIM,m_bot);
					}
					continue;
				}
				// kite attack it
				if (Util::IsMeleeUnit(target) && Queen->weapon_cooldown > 0) {
					auto p1 = target->pos, p2 = Queen->pos;
					auto tp = p2 * 2 - p1;
					Micro::SmartMove(Queen, tp, m_bot);
				}
				else {
					Micro::SmartAttackMove(Queen, target->pos, m_bot);
				}
			}
			// if there are no targets
			else
			{
				// if we're not near the order position
				if (Util::Dist(Queen->pos, order.getPosition()) > 4)
				{
					// move to it
					Micro::SmartMove(Queen, order.getPosition(), m_bot);
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
const sc2::Unit * QueenManager::getTarget(const sc2::Unit * rangedUnit, const std::vector<const sc2::Unit *> & targets)
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
int QueenManager::getAttackPriority(const sc2::Unit * attacker, const sc2::Unit * unit)
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

