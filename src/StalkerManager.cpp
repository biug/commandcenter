#include "StalkerManager.h"
#include "Util.h"
#include "CCBot.h"
#include <cmath>
#define PI 3.1415926

StalkerInfo::StalkerInfo() :
	m_hpLastSecond(160)
{

}

StalkerInfo::StalkerInfo(float hp) :
	m_hpLastSecond(hp)
{

}

StalkerManager::StalkerManager(CCBot & bot)
	: MicroManager(bot)
{

}

void StalkerManager::executeMicro(const std::vector<const sc2::Unit *> & targets)
{
	assignTargets(targets);
}

void StalkerManager::assignTargets(const std::vector<const sc2::Unit *> & targets)
{
	const std::vector<const sc2::Unit *> & stalkers = getUnits();

	// figure out targets
	std::vector<const sc2::Unit *> stalkerTargets;
	for (auto target : targets)
	{
		if (!target) { continue; }
		if (target->unit_type == sc2::UNIT_TYPEID::ZERG_EGG) { continue; }
		if (target->unit_type == sc2::UNIT_TYPEID::ZERG_LARVA) { continue; }

		stalkerTargets.push_back(target);
	}

	// for each meleeUnit
	bool refreshInfo = m_bot.Map().frame() % 16;
	for (auto stalker : stalkers)
	{
		std::stringstream ss;
		ss << stalker->facing;
		m_bot.Map().drawText(stalker->pos,ss.str());
		BOT_ASSERT(stalker, "ranged unit is null");
		if (stalkerInfos.find(stalker) == stalkerInfos.end())
		{
			stalkerInfos[stalker] = StalkerInfo(stalker->health + stalker->shield);
		}
		float currentHP = stalker->health + stalker->shield;
		bool beingAttack = currentHP < stalkerInfos[stalker].m_hpLastSecond;
		if (refreshInfo) stalkerInfos[stalker].m_hpLastSecond = currentHP;

		// if the order is to attack or defend
		if (order.getType() == SquadOrderTypes::Attack || order.getType() == SquadOrderTypes::Defend)
		{
			if (!stalkerTargets.empty())
			{
				// find the best target for this meleeUnit
				const sc2::Unit * target = getTarget(stalker, stalkerTargets);
				if (!target) continue;

				if (m_bot.State().m_rschBlink && stalker->health + stalker->shield < 50 && beingAttack)
				{
					auto abilities = m_bot.Query()->GetAbilitiesForUnit(stalker);
					bool canBlink = false;
					for (auto & ab : abilities.abilities)
					{
						if (ab.ability_id.ToType() == sc2::ABILITY_ID::EFFECT_BLINK)
						{
							canBlink = true;
						}
					}
					if (canBlink)
					{
						
						auto angle = atan((target->pos.y - stalker->pos.y) / (target->pos.x - stalker->pos.x));
						int step_size = 8;
						float step_x = step_size * sin(angle);
						float step_y = step_size * cos(angle);
						Micro::SmartAbility(stalker, sc2::ABILITY_ID::EFFECT_BLINK, stalker->pos + sc2::Point2D(step_x, step_y), m_bot);
						Micro::SmartAttackMove(stalker, target->pos, m_bot);
					}
					continue;
				}
				// kite attack it
				Micro::SmartKiteTarget(stalker, target, m_bot);
			}
			// if there are no targets
			else
			{
				// if we're not near the order position
				if (Util::Dist(stalker->pos, order.getPosition()) > 4)
				{
					// move to it
					Micro::SmartMove(stalker, order.getPosition(), m_bot);
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
const sc2::Unit * StalkerManager::getTarget(const sc2::Unit * rangedUnit, const std::vector<const sc2::Unit *> & targets)
{
	BOT_ASSERT(rangedUnit, "null melee unit in getTarget");

	int highPriority = 0;
	double closestDist = std::numeric_limits<double>::max();
	float lowestHealth = std::numeric_limits<float>::max();
	const sc2::Unit * closestTarget = nullptr;

	// for each target possiblity
	for (auto targetUnit : targets)
	{
		BOT_ASSERT(targetUnit, "null target unit in getTarget");

		int priority = getAttackPriority(rangedUnit, targetUnit);
		float distance = Util::Dist(rangedUnit->pos, targetUnit->pos);
	
		

		// If there are ranged units on high ground we can't see, we can't attack them back.
		if (!(m_bot.Observation()->GetVisibility(targetUnit->pos) == sc2::Visibility::Visible) && Util::IsCombatUnit(targetUnit, m_bot))
			continue;
		// if it's a higher priority, or it's closer, set it
		if (!closestTarget || (priority > highPriority) || (priority == highPriority && distance < closestDist) || (priority == highPriority && targetUnit->health<lowestHealth))
		{
			lowestHealth = targetUnit->health;
			closestDist = distance;
			highPriority = priority;
			closestTarget = targetUnit;
		}
		
	}
	

	return closestTarget;
}

// get the attack priority of a type in relation to a zergling
int StalkerManager::getAttackPriority(const sc2::Unit * attacker, const sc2::Unit * unit)
{
	BOT_ASSERT(unit, "null unit in getAttackPriority");
	if (Util::IsPsionicUnit(unit)) {
		return 11;
	}
	if (Util::IsHeavyArmor(unit)) {
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

