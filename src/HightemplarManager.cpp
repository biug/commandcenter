#include "HightemplarManager.h"
#include "Util.h"
#include "CCBot.h"

HightemplarInfo::HightemplarInfo() :
	m_hpLastSecond(45)
{

}

HightemplarInfo::HightemplarInfo(float hp) :
	m_hpLastSecond(hp)
{

}

HightemplarManager::HightemplarManager(CCBot & bot)
	: MicroManager(bot)
{

}

void HightemplarManager::executeMicro(const std::vector<const sc2::Unit *> & targets)
{
	assignTargets(targets);
}

void HightemplarManager::assignTargets(const std::vector<const sc2::Unit *> & targets)
{
	const std::vector<const sc2::Unit *> & Hightemplars = getUnits();

	// figure out targets
	std::vector<const sc2::Unit *> HightemplarTargets;
	for (auto target : targets)
	{
		if (!target) { continue; }
		if (target->unit_type == sc2::UNIT_TYPEID::ZERG_EGG) { continue; }
		if (target->unit_type == sc2::UNIT_TYPEID::ZERG_LARVA) { continue; }

		HightemplarTargets.push_back(target);
	}

	bool refreshInfo = m_bot.Map().frame() % 16;
	for (auto Hightemplar : Hightemplars)
	{
		BOT_ASSERT(Hightemplar, "ranged unit is null");
		if (HightemplarInfos.find(Hightemplar) == HightemplarInfos.end())
		{
			HightemplarInfos[Hightemplar] = HightemplarInfo(Hightemplar->health + Hightemplar->shield);
		}
		float currentHP = Hightemplar->health + Hightemplar->shield;
		bool beingAttack = currentHP < HightemplarInfos[Hightemplar].m_hpLastSecond;
		if (refreshInfo) HightemplarInfos[Hightemplar].m_hpLastSecond = currentHP;

		// if the order is to attack or defend
		if (order.getType() == SquadOrderTypes::Attack || order.getType() == SquadOrderTypes::Defend)
		{
			if (!HightemplarTargets.empty())
			{
				// find the best target for this meleeUnit
				const sc2::Unit * target = getTarget(Hightemplar, HightemplarTargets);
				if (!target) continue;
				bool feedback = false;
				bool psistorm = false;
				bool rallyunits = false;
				auto abilities = m_bot.Query()->GetAbilitiesForUnit(Hightemplar);
				for (auto & ab : abilities.abilities)
				{
					if (ab.ability_id.ToType() == sc2::ABILITY_ID::EFFECT_FEEDBACK)
					{
						feedback = true;
					}
					else if (ab.ability_id.ToType() == sc2::ABILITY_ID::EFFECT_PSISTORM)
					{
						psistorm = true;
					}
					else if (ab.ability_id.ToType() == sc2::ABILITY_ID::RALLY_UNITS)
					{
						rallyunits= true;
					}
				}
				if (feedback && target->energy >= 30)
				{
					Micro::SmartAbility(Hightemplar, sc2::ABILITY_ID::EFFECT_FEEDBACK, target, m_bot);
				}
				if (psistorm && SetPsistormPosition(Hightemplar, HightemplarTargets).maxtargetnum >= 2)
				{
					Micro::SmartAbility(Hightemplar, sc2::ABILITY_ID::EFFECT_PSISTORM, SetPsistormPosition(Hightemplar, HightemplarTargets).hasmosttargetposition, m_bot);
				}
				if (rallyunits && Hightemplar->energy < 50)
				{
					Micro::SmartAbility(Hightemplar, sc2::ABILITY_ID::RALLY_UNITS, getRallyUnit(Hightemplar, Hightemplars), m_bot);
				}

				
			}
			// if there are no targets
			else
			{
				// if we're not near the order position
				if (Util::Dist(Hightemplar->pos, order.getPosition()) > 4)
				{
					// move to it
					Micro::SmartMove(Hightemplar, order.getPosition(), m_bot);
				}
			}
		}

		if (m_bot.Config().DrawUnitTargetInfo)
		{
			// TODO: draw the line to the unit's target
		}
	}

}


MostTargetPosition HightemplarManager::SetPsistormPosition(const sc2::Unit * rangedUnit, const std::vector<const sc2::Unit *> & targets) {
	const sc2::Unit *HasMostTargetPosition = nullptr;
	int maxtargetnum = 0;
	for (auto target : targets)
	{
		int nearbytargetnum = 0;
		for (auto nearbytarget : targets)
		{
			if (Util::Dist(target->pos, nearbytarget->pos) <= 3)
			{
				nearbytargetnum++;
			}
		}
		if (nearbytargetnum > maxtargetnum)
		{
			maxtargetnum = nearbytargetnum;
			HasMostTargetPosition = target;
		}
	}
	MostTargetPosition a;
	a.hasmosttargetposition.x = HasMostTargetPosition->pos.x;
	a.hasmosttargetposition.y = HasMostTargetPosition->pos.y;
	a.maxtargetnum = maxtargetnum;
	return a;
}


const sc2::Unit * HightemplarManager::getRallyUnit(const sc2::Unit * rangedUnit, const std::vector<const sc2::Unit *> & targets) {
	bool canRallyUnit = false;
	const sc2::Unit *UnitForRally = nullptr;
	for (auto selfunit : targets)
	{
		if(selfunit !=rangedUnit && (selfunit->unit_type == sc2::UNIT_TYPEID::PROTOSS_HIGHTEMPLAR || selfunit->unit_type == sc2::UNIT_TYPEID::PROTOSS_DARKTEMPLAR)) {
			canRallyUnit = true;
		}
		if (canRallyUnit && ((selfunit->energy +selfunit->health)< (UnitForRally->energy+UnitForRally->health) || UnitForRally == nullptr)) {
			UnitForRally = selfunit;
		}
		canRallyUnit = false;
	}
	return UnitForRally;
}

// get a target for the ranged unit to attack
// TODO: this is the melee targeting code, replace it with something better for ranged units
const sc2::Unit * HightemplarManager::getTarget(const sc2::Unit * rangedUnit, const std::vector<const sc2::Unit *> & targets)
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

// get the attack priority of a type in relation to a Hightemplar
int HightemplarManager::getAttackPriority(const sc2::Unit * attacker, const sc2::Unit * unit)
{
	BOT_ASSERT(unit, "null unit in getAttackPriority");
	if (unit->energy >= 40)
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

