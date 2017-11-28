#include "MarineManager.h"
#include "Util.h"
#include "CCBot.h"

MarineInfo::MarineInfo() :
	m_hpLastSecond(45)
{

}

MarineInfo::MarineInfo(float hp) :
	m_hpLastSecond(hp)
{

}

MarineManager::MarineManager(CCBot & bot)
	: MicroManager(bot)
{

}

void MarineManager::executeMicro(const std::vector<const sc2::Unit *> & targets)
{
	assignTargets(targets);
}

void MarineManager::assignTargets(const std::vector<const sc2::Unit *> & targets)
{
	const std::vector<const sc2::Unit *> & marines = getUnits();

	// figure out targets
	std::vector<const sc2::Unit *> marineTargets;
	for (auto target : targets)
	{
		if (!target) { continue; }
		if (target->unit_type == sc2::UNIT_TYPEID::ZERG_EGG) { continue; }
		if (target->unit_type == sc2::UNIT_TYPEID::ZERG_LARVA) { continue; }

		marineTargets.push_back(target);
	}

	// for each meleeUnit
	bool refreshInfo = m_bot.Map().frame() % 16;
	for (auto marine : marines)
	{
		BOT_ASSERT(marine, "ranged unit is null");
		if (marineInfos.find(marine) == marineInfos.end())
		{
			marineInfos[marine] = MarineInfo(marine->health);
		}
		int currentHP = marine->health;
		bool beingAttack = currentHP < marineInfos[marine].m_hpLastSecond;
		if (refreshInfo) marineInfos[marine].m_hpLastSecond = currentHP;

		// if the order is to attack or defend
		if (order.getType() == SquadOrderTypes::Attack || order.getType() == SquadOrderTypes::Defend)
		{
			if (!marineTargets.empty())
			{
				// find the best target for this meleeUnit
				const sc2::Unit * target = getTarget(marine, marineTargets);
				if (!target) continue;

				if (m_bot.State().m_stimpack)
				{
					auto abilities = m_bot.Query()->GetAbilitiesForUnit(marine);
					bool stimpack = false;
					
					for (auto & ab : abilities.abilities)
					{
						if (ab.ability_id.ToType() == sc2::ABILITY_ID::EFFECT_STIM)
						{
							stimpack = true;
							
						}
					}
					for (auto buff : marine->buffs) {
						if (buff == sc2::BUFF_ID::STIMPACK) {
							stimpack = false;
						}
					}
					if (stimpack && beingAttack)
					{
						Micro::SmartAbility(marine, sc2::ABILITY_ID::EFFECT_STIM,m_bot);
					}
					continue;
				}
				// kite attack it
				Micro::SmartKiteTarget(marine, target, m_bot);
			}
			// if there are no targets
			else
			{
				// if we're not near the order position
				if (Util::Dist(marine->pos, order.getPosition()) > 4)
				{
					// move to it
					Micro::SmartMove(marine, order.getPosition(), m_bot);
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
const sc2::Unit * MarineManager::getTarget(const sc2::Unit * rangedUnit, const std::vector<const sc2::Unit *> & targets)
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
int MarineManager::getAttackPriority(const sc2::Unit * attacker, const sc2::Unit * unit)
{
	BOT_ASSERT(unit, "null unit in getAttackPriority");
	if (unit->unit_type.ToType() == sc2::UNIT_TYPEID::PROTOSS_SENTRY) {
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

