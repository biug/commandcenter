#include "SentryManager.h"
#include "Util.h"
#include "CCBot.h"

SentryInfo::SentryInfo() :
m_hpLastSecond(45)
{
	
}

SentryInfo::SentryInfo(float hp) :
	m_hpLastSecond(hp)
{

}

SentryManager::SentryManager(CCBot & bot)
	: MicroManager(bot)
{

}

void SentryManager::executeMicro(const std::vector<const sc2::Unit *> & targets)
{
	assignTargets(targets);
}

void SentryManager::assignTargets(const std::vector<const sc2::Unit *> & targets)
{
	const std::vector<const sc2::Unit *> & Sentrys = getUnits();

	// figure out targets
	std::vector<const sc2::Unit *> SentryTargets;
	for (auto target : targets)
	{
		if (!target) { continue; }
		if (target->unit_type == sc2::UNIT_TYPEID::ZERG_EGG) { continue; }
		if (target->unit_type == sc2::UNIT_TYPEID::ZERG_LARVA) { continue; }

		SentryTargets.push_back(target);
	}

	// for each meleeUnit
	bool refreshInfo = m_bot.Map().frame() % 16;
	for (auto Sentry : Sentrys)
	{
		BOT_ASSERT(Sentry, "ranged unit is null");
		if (SentryInfos.find(Sentry) == SentryInfos.end())
		{
			SentryInfos[Sentry] = SentryInfo(Sentry->health);
		}
		int currentHP = Sentry->health;
		bool beingAttack = currentHP < SentryInfos[Sentry].m_hpLastSecond;
		if (refreshInfo) SentryInfos[Sentry].m_hpLastSecond = currentHP;
		
		// if the order is to attack or defend
		if (order.getType() == SquadOrderTypes::Attack || order.getType() == SquadOrderTypes::Defend)
		{
			if (!SentryTargets.empty())
			{
				// find the best target for this meleeUnit
				const sc2::Unit * target = getTarget(Sentry, SentryTargets);
				if (!target) continue;
				
				if (m_bot.State().m_stimpack)
				{
					auto abilities = m_bot.Query()->GetAbilitiesForUnit(Sentry);
					bool stimpack = false;
					
					for (auto & ab : abilities.abilities)
					{
						if (ab.ability_id.ToType() == sc2::ABILITY_ID::EFFECT_STIM)
						{
							stimpack = true;
							
						}
					}
					for (auto buff : Sentry->buffs) {
						if (buff == sc2::BUFF_ID::STIMPACK) {
							stimpack = false;
						}
					}
					if (stimpack && (beingAttack || Sentry->weapon_cooldown >0))
					{
						Micro::SmartAbility(Sentry, sc2::ABILITY_ID::EFFECT_STIM,m_bot);
					}
					continue;
				}
				// kite attack it
				if (Util::IsMeleeUnit(target) && Sentry->weapon_cooldown > 0) {
					auto p1 = target->pos, p2 = Sentry->pos;
					auto tp = p2 * 2 - p1;
					Micro::SmartMove(Sentry, tp, m_bot);
				}
				else {
					Micro::SmartAttackMove(Sentry, target->pos, m_bot);
				}
			}
			// if there are no targets
			else
			{
				// if we're not near the order position
				if (Util::Dist(Sentry->pos, order.getPosition()) > 4)
				{
					// move to it
					Micro::SmartMove(Sentry, order.getPosition(), m_bot);
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
const sc2::Unit * SentryManager::getTarget(const sc2::Unit * rangedUnit, const std::vector<const sc2::Unit *> & targets)
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
int SentryManager::getAttackPriority(const sc2::Unit * attacker, const sc2::Unit * unit)
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

