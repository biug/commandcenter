#include "DarktemplarManager.h"
#include "Util.h"
#include "CCBot.h"

DarktemplarInfo::DarktemplarInfo() :
	m_hpLastSecond(45)
{

}

DarktemplarInfo::DarktemplarInfo(float hp) :
	m_hpLastSecond(hp)
{

}

DarktemplarManager::DarktemplarManager(CCBot & bot)
	: MicroManager(bot)
{

}

void DarktemplarManager::executeMicro(const std::vector<const sc2::Unit *> & targets)
{
	assignTargets(targets);
}

void DarktemplarManager::assignTargets(const std::vector<const sc2::Unit *> & targets)
{
	const std::vector<const sc2::Unit *> & Darktemplars = getUnits();

	// figure out targets
	std::vector<const sc2::Unit *> DarktemplarTargets;
	for (auto target : targets)
	{
		if (!target) { continue; }
		if (target->unit_type == sc2::UNIT_TYPEID::ZERG_EGG) { continue; }
		if (target->unit_type == sc2::UNIT_TYPEID::ZERG_LARVA) { continue; }

		DarktemplarTargets.push_back(target);
	}

	bool refreshInfo = m_bot.Map().frame() % 16;
	for (auto Darktemplar : Darktemplars)
	{
		BOT_ASSERT(Darktemplar, "ranged unit is null");
		if (DarktemplarInfos.find(Darktemplar) == DarktemplarInfos.end())
		{
			DarktemplarInfos[Darktemplar] = DarktemplarInfo(Darktemplar->health + Darktemplar->shield);
		}
		float currentHP = Darktemplar->health + Darktemplar->shield;
		bool beingAttack = currentHP < DarktemplarInfos[Darktemplar].m_hpLastSecond;
		if (refreshInfo) DarktemplarInfos[Darktemplar].m_hpLastSecond = currentHP;

		// if the order is to attack or defend
		if (order.getType() == SquadOrderTypes::Attack || order.getType() == SquadOrderTypes::Defend)
		{
			if (!DarktemplarTargets.empty())
			{
				// find the best target for this meleeUnit
				const sc2::Unit * target = getTarget(Darktemplar, DarktemplarTargets);
				if (!target) continue;
				if (Darktemplar->unit_type == sc2::UNIT_TYPEID::PROTOSS_DARKTEMPLAR) {
					auto abilities = m_bot.Query()->GetAbilitiesForUnit(Darktemplar);
					bool rallyunits = false;
					for (auto & ab : abilities.abilities)
					{
						if (ab.ability_id.ToType() == sc2::ABILITY_ID::RALLY_UNITS)
						{
							rallyunits = true;
						}
					}
					if (rallyunits && currentHP <= 50)
					{
						Micro::SmartAbility(Darktemplar, sc2::ABILITY_ID::RALLY_UNITS, getRallyUnit(Darktemplar,Darktemplars), m_bot);
					}
				}

			}
			// if there are no targets
			else
			{
				// if we're not near the order position
				if (Util::Dist(Darktemplar->pos, order.getPosition()) > 4)
				{
					// move to it
					Micro::SmartMove(Darktemplar, order.getPosition(), m_bot);
				}
			}
		}

		if (m_bot.Config().DrawUnitTargetInfo)
		{
			// TODO: draw the line to the unit's target
		}
	}

}


const sc2::Unit * DarktemplarManager::getRallyUnit(const sc2::Unit * rangedUnit, const std::vector<const sc2::Unit *> & targets) {
	bool canRallyUnit = false;
	const sc2::Unit *UnitForRally = nullptr;
	for (auto selfunit : targets)
	{
		if(selfunit != rangedUnit && (selfunit->unit_type == sc2::UNIT_TYPEID::PROTOSS_HIGHTEMPLAR || selfunit->unit_type == sc2::UNIT_TYPEID::PROTOSS_DARKTEMPLAR)) {
			canRallyUnit = true;
		}
		if (canRallyUnit && ((selfunit->energy + selfunit->health)< (UnitForRally->energy + UnitForRally->health) || UnitForRally == nullptr)) {
			UnitForRally = selfunit;
		}
		canRallyUnit = false;
	}
	return UnitForRally;
}


// get a target for the ranged unit to attack
// TODO: this is the melee targeting code, replace it with something better for ranged units
const sc2::Unit * DarktemplarManager::getTarget(const sc2::Unit * rangedUnit, const std::vector<const sc2::Unit *> & targets)
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
int DarktemplarManager::getAttackPriority(const sc2::Unit * attacker, const sc2::Unit * unit)
{
	BOT_ASSERT(unit, "null unit in getAttackPriority");
	if (Util::IsWorker(unit))
	{
		return 13;
	}
	if (Util::IsSupplyProvider(unit))
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


	return 1;
}

