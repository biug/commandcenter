#include "BattlecruiserManager.h"
#include "Util.h"
#include "CCBot.h"

BattlecruiserInfo::BattlecruiserInfo() :
m_hpLastSecond(45)
{
	
}

BattlecruiserInfo::BattlecruiserInfo(float hp) :
	m_hpLastSecond(hp)
{

}

BattlecruiserManager::BattlecruiserManager(CCBot & bot)
	: MicroManager(bot)
{

}

void BattlecruiserManager::executeMicro(const std::vector<const sc2::Unit *> & targets)
{
	assignTargets(targets);
}

void BattlecruiserManager::assignTargets(const std::vector<const sc2::Unit *> & targets)
{
	const std::vector<const sc2::Unit *> & Battlecruisers = getUnits();

	// figure out targets
	std::vector<const sc2::Unit *> BattlecruiserTargets;
	for (auto target : targets)
	{
		if (!target) { continue; }
		if (target->unit_type == sc2::UNIT_TYPEID::ZERG_EGG) { continue; }
		if (target->unit_type == sc2::UNIT_TYPEID::ZERG_LARVA) { continue; }

		BattlecruiserTargets.push_back(target);
	}

	// for each meleeUnit
	bool refreshInfo = m_bot.Map().frame() % 16;
	for (auto Battlecruiser : Battlecruisers)
	{
		BOT_ASSERT(Battlecruiser, "ranged unit is null");
		if (BattlecruiserInfos.find(Battlecruiser) == BattlecruiserInfos.end())
		{
			BattlecruiserInfos[Battlecruiser] = BattlecruiserInfo(Battlecruiser->health);
		}
		float currentHP = Battlecruiser->health;
		bool beingAttack = currentHP < BattlecruiserInfos[Battlecruiser].m_hpLastSecond;
		if (refreshInfo) BattlecruiserInfos[Battlecruiser].m_hpLastSecond = currentHP;
		
		// if the order is to attack or defend
		if (order.getType() == SquadOrderTypes::Attack || order.getType() == SquadOrderTypes::Defend)
		{
			if (!BattlecruiserTargets.empty())
			{
				// find the best target for this meleeUnit
				const sc2::Unit * target = getTarget(Battlecruiser, BattlecruiserTargets);
				if (!target) continue;
				auto abilities = m_bot.Query()->GetAbilitiesForUnit(Battlecruiser);
				bool isYamatogunVaild = false;
				bool isTacticaljumpVaild = false;

				for (auto & ab : abilities.abilities)
				{
					if (ab.ability_id.ToType() == sc2::ABILITY_ID::EFFECT_YAMATOGUN)
					{
						isYamatogunVaild = true;

					}
					if (ab.ability_id.ToType() == sc2::ABILITY_ID::EFFECT_TACTICALJUMP)
					{
						isTacticaljumpVaild = true;

					}
				}

				if (isYamatogunVaild && Battlecruiser->energy >= 125)
				{
					Micro::SmartAbility(Battlecruiser, sc2::ABILITY_ID::EFFECT_YAMATOGUN, target, m_bot);
					
				}
				else if (beingAttack && currentHP<50 && isTacticaljumpVaild && (Battlecruiser->weapon_cooldown > 0) && Battlecruiser->energy >= 100) {
					sc2::Point2D base = m_bot.GetStartLocation();
					Micro::SmartAbility(Battlecruiser, sc2::ABILITY_ID::EFFECT_TACTICALJUMP, base, m_bot);
				}
				else {
					Micro::SmartAttackMove(Battlecruiser, target->pos, m_bot);
					
				}
			}
			// if there are no targets 
			else
			{
				// if we're not near the order position
				if (Util::Dist(Battlecruiser->pos, order.getPosition()) > 4)
				{
					// move to it
					Micro::SmartMove(Battlecruiser, order.getPosition(), m_bot);
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
const sc2::Unit * BattlecruiserManager::getTarget(const sc2::Unit * rangedUnit, const std::vector<const sc2::Unit *> & targets)
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
int BattlecruiserManager::getAttackPriority(const sc2::Unit * attacker, const sc2::Unit * unit)
{
	BOT_ASSERT(unit, "null unit in getAttackPriority");
	if (Util::canAttackSky(unit->unit_type)) {
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

