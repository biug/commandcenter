#include "ThorManager.h"
#include "Util.h"
#include "CCBot.h"

ThorInfo::ThorInfo() :
m_hpLastSecond(45)
{
	
}

ThorInfo::ThorInfo(float hp) :
	m_hpLastSecond(hp)
{

}

ThorManager::ThorManager(CCBot & bot)
	: MicroManager(bot)
{

}

void ThorManager::executeMicro(const std::vector<const sc2::Unit *> & targets)
{
	assignTargets(targets);
}

void ThorManager::assignTargets(const std::vector<const sc2::Unit *> & targets)
{
	const std::vector<const sc2::Unit *> & Thors = getUnits();

	// figure out targets
	std::vector<const sc2::Unit *> ThorTargets;
	for (auto target : targets)
	{
		if (!target) { continue; }
		if (target->unit_type == sc2::UNIT_TYPEID::ZERG_EGG) { continue; }
		if (target->unit_type == sc2::UNIT_TYPEID::ZERG_LARVA) { continue; }

		ThorTargets.push_back(target);
	}

	// for each meleeUnit
	bool refreshInfo = m_bot.Map().frame() % 16;
	for (auto Thor : Thors)
	{
		BOT_ASSERT(Thor, "ranged unit is null");
		if (ThorInfos.find(Thor) == ThorInfos.end())
		{
			ThorInfos[Thor] = ThorInfo(Thor->health);
		}
		float currentHP = Thor->health;
		bool beingAttack = currentHP < ThorInfos[Thor].m_hpLastSecond;
		if (refreshInfo) ThorInfos[Thor].m_hpLastSecond = currentHP;
		
		// if the order is to attack or defend
		if (order.getType() == SquadOrderTypes::Attack || order.getType() == SquadOrderTypes::Defend)
		{
			if (!ThorTargets.empty())
			{
				// find the best target for this meleeUnit
				const sc2::Unit * target = getTarget(Thor, ThorTargets);
				if (!target) continue;
				int enemyflynum = 0;
				bool flag = false;
				for (auto & u : m_bot.Observation()->GetUnits())
				{
					if ((Util::GetPlayer(u) == Players::Enemy) && !(Util::IsHeavyArmor(u))&&u->is_flying)
					{
						enemyflynum++;
					}
					if ((Util::GetPlayer(u) == Players::Enemy) && Util::IsHeavyArmor(u) && u->is_flying)
					{
						flag=true;
					}
				}
				if (Thor->unit_type.ToType() == sc2::UNIT_TYPEID::TERRAN_THORAP && enemyflynum > 2) {
					Micro::SmartAbility(Thor, sc2::ABILITY_ID::MORPH_THOREXPLOSIVEMODE, m_bot);
				}
				else if (Thor->unit_type.ToType() == sc2::UNIT_TYPEID::TERRAN_THOR && enemyflynum==1 && flag)
				{
						Micro::SmartAbility(Thor, sc2::ABILITY_ID::MORPH_THORHIGHIMPACTMODE, m_bot);
				}
				else {
					Micro::SmartAttackUnit(Thor, target, m_bot);
				}
				
			}
			// if there are no targets
			else
			{
				// if we're not near the order position
				if (Util::Dist(Thor->pos, order.getPosition()) > 4)
				{
					// move to it
					Micro::SmartMove(Thor, order.getPosition(), m_bot);
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
const sc2::Unit * ThorManager::getTarget(const sc2::Unit * rangedUnit, const std::vector<const sc2::Unit *> & targets)
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
int ThorManager::getAttackPriority(const sc2::Unit * attacker, const sc2::Unit * unit)
{
	BOT_ASSERT(unit, "null unit in getAttackPriority");
	if (unit->is_flying) {
		return 12;
	}
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

