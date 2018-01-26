#include "MutaliskManager.h"
#include "Util.h"
#include "CCBot.h"

MutaliskInfo::MutaliskInfo() :
m_hpLastSecond(45)
{
	
}

MutaliskInfo::MutaliskInfo(float hp) :
	m_hpLastSecond(hp)
{

}

MutaliskManager::MutaliskManager(CCBot & bot)
	: MicroManager(bot)
{

}

void MutaliskManager::executeMicro(const std::vector<const sc2::Unit *> & targets)
{
	assignTargets(targets);
}

void MutaliskManager::assignTargets(const std::vector<const sc2::Unit *> & targets)
{
	const std::vector<const sc2::Unit *> & Mutalisks = getUnits();

	// figure out targets
	std::vector<const sc2::Unit *> MutaliskTargets;
	for (auto target : targets)
	{
		if (!target) { continue; }
		if (target->unit_type == sc2::UNIT_TYPEID::ZERG_EGG) { continue; }
		if (target->unit_type == sc2::UNIT_TYPEID::ZERG_LARVA) { continue; }

		MutaliskTargets.push_back(target);
	}

	// for each meleeUnit
	bool refreshInfo = m_bot.Map().frame() % 16;
	for (auto Mutalisk : Mutalisks)
	{
		BOT_ASSERT(Mutalisk, "ranged unit is null");
		if (MutaliskInfos.find(Mutalisk) == MutaliskInfos.end())
		{
			MutaliskInfos[Mutalisk] = MutaliskInfo(Mutalisk->health);
		}
		int currentHP = Mutalisk->health;
		bool beingAttack = currentHP < MutaliskInfos[Mutalisk].m_hpLastSecond;
		if (refreshInfo) MutaliskInfos[Mutalisk].m_hpLastSecond = currentHP;
		
		// if the order is to attack or defend
		if (order.getType() == SquadOrderTypes::Attack || order.getType() == SquadOrderTypes::Defend)
		{
			if (!MutaliskTargets.empty())
			{
				// find the best target for this meleeUnit
				const sc2::Unit * target = getTarget(Mutalisk, MutaliskTargets);
				if (!target) continue;
				
				if (m_bot.State().m_stimpack)
				{
					auto abilities = m_bot.Query()->GetAbilitiesForUnit(Mutalisk);
					bool stimpack = false;
					
					for (auto & ab : abilities.abilities)
					{
						if (ab.ability_id.ToType() == sc2::ABILITY_ID::EFFECT_STIM)
						{
							stimpack = true;
							
						}
					}
					for (auto buff : Mutalisk->buffs) {
						if (buff == sc2::BUFF_ID::STIMPACK) {
							stimpack = false;
						}
					}
					if (stimpack && (beingAttack || Mutalisk->weapon_cooldown >0))
					{
						Micro::SmartAbility(Mutalisk, sc2::ABILITY_ID::EFFECT_STIM,m_bot);
					}
					continue;
				}
				// kite attack it
				if (Util::IsMeleeUnit(target) && Mutalisk->weapon_cooldown > 0) {
					auto p1 = target->pos, p2 = Mutalisk->pos;
					auto tp = p2 * 2 - p1;
					Micro::SmartMove(Mutalisk, tp, m_bot);
				}
				else {
					Micro::SmartAttackMove(Mutalisk, target->pos, m_bot);
				}
			}
			// if there are no targets
			else
			{
				// if we're not near the order position
				if (Util::Dist(Mutalisk->pos, order.getPosition()) > 4)
				{
					// move to it
					Micro::SmartMove(Mutalisk, order.getPosition(), m_bot);
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
const sc2::Unit * MutaliskManager::getTarget(const sc2::Unit * rangedUnit, const std::vector<const sc2::Unit *> & targets)
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
int MutaliskManager::getAttackPriority(const sc2::Unit * attacker, const sc2::Unit * unit)
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

