#include "BansheeManager.h"
#include "Util.h"
#include "CCBot.h"

BansheeInfo::BansheeInfo() :
m_hpLastSecond(45)
{
	
}

BansheeInfo::BansheeInfo(float hp) :
	m_hpLastSecond(hp)
{

}

BansheeManager::BansheeManager(CCBot & bot)
	: MicroManager(bot)
{

}

void BansheeManager::executeMicro(const std::vector<const sc2::Unit *> & targets)
{
	assignTargets(targets);
}

void BansheeManager::assignTargets(const std::vector<const sc2::Unit *> & targets)
{
	const std::vector<const sc2::Unit *> & Banshees = getUnits();

	// figure out targets
	std::vector<const sc2::Unit *> BansheeTargets;
	for (auto target : targets)
	{
		if (!target) { continue; }
		if (target->unit_type == sc2::UNIT_TYPEID::ZERG_EGG) { continue; }
		if (target->unit_type == sc2::UNIT_TYPEID::ZERG_LARVA) { continue; }

		BansheeTargets.push_back(target);
	}

	// for each meleeUnit
	bool refreshInfo = m_bot.Map().frame() % 16;
	for (auto Banshee : Banshees)
	{
		BOT_ASSERT(Banshee, "ranged unit is null");
		if (BansheeInfos.find(Banshee) == BansheeInfos.end())
		{
			BansheeInfos[Banshee] = BansheeInfo(Banshee->health);
		}
		float currentHP = Banshee->health;
		bool beingAttack = currentHP < BansheeInfos[Banshee].m_hpLastSecond;
		if (refreshInfo) BansheeInfos[Banshee].m_hpLastSecond = currentHP;
		
		// if the order is to attack or defend
		if (order.getType() == SquadOrderTypes::Attack || order.getType() == SquadOrderTypes::Defend)
		{
			bool iscloakon = false;
			bool iscloakoff = false;
			bool isdetector = false;
			if (!BansheeTargets.empty())
			{
				// find the best target for this meleeUnit
				const sc2::Unit * target = getTarget(Banshee, BansheeTargets);
				if (!target) continue;
				
				auto abilities = m_bot.Query()->GetAbilitiesForUnit(Banshee);
				for (auto & ab : abilities.abilities)
				{
					if (ab.ability_id.ToType() == sc2::ABILITY_ID::BEHAVIOR_CLOAKON)
					{
						iscloakon = true;
					}
					if (ab.ability_id.ToType() == sc2::ABILITY_ID::BEHAVIOR_CLOAKOFF)
					{
						iscloakoff = true;
					}
				}
				for (auto & u : m_bot.Observation()->GetUnits())
				{
					if ((Util::GetPlayer(u) == Players::Enemy) && Util::IsDetectorType(u->unit_type))
					{
						isdetector = true;
					}
				}
				std::cout << iscloakon << std::endl;
				if (iscloakon && !(isdetector) && Banshee->energy > 25)
				{
					Micro::SmartAbility(Banshee, sc2::ABILITY_ID::BEHAVIOR_CLOAKON,  m_bot);
				}
				else if (beingAttack && (Banshee->weapon_cooldown > 0 )) {
					sc2::Point2D rp = RetreatPosition(Banshee);
					Micro::SmartMove(Banshee, rp, m_bot);
				}
				else {
					Micro::SmartAttackMove(Banshee, target->pos, m_bot);
				}
			}
			// if there are no targets
			else
			{
				if (iscloakoff) {
					Micro::SmartAbility(Banshee, sc2::ABILITY_ID::BEHAVIOR_CLOAKOFF, m_bot);
				}
				// if we're not near the order position
				if (Util::Dist(Banshee->pos, order.getPosition()) > 4)
				{
					// move to it
					Micro::SmartMove(Banshee, order.getPosition(), m_bot);
				}
			}
		}

		if (m_bot.Config().DrawUnitTargetInfo)
		{
			// TODO: draw the line to the unit's target
		}
	}
}

sc2::Point2D BansheeManager::RetreatPosition(const sc2::Unit * unit)
{
	auto pos = unit->pos;
	sc2::Point2D base = m_bot.GetStartLocation();

	double angle = atan(pos.y - base.y / (pos.x - base.x));
	double step_size = 5;
	double step_x = step_size * sin(angle);
	double step_y = step_size * cos(angle);
	return sc2::Point2D(pos.x + step_x, pos.y + step_y);

}

// get a target for the ranged unit to attack
// TODO: this is the melee targeting code, replace it with something better for ranged units
const sc2::Unit * BansheeManager::getTarget(const sc2::Unit * rangedUnit, const std::vector<const sc2::Unit *> & targets)
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
int BansheeManager::getAttackPriority(const sc2::Unit * attacker, const sc2::Unit * unit)
{
	BOT_ASSERT(unit, "null unit in getAttackPriority");
	if (unit->is_flying) {
		return 7;
	}
	if (Util::canAttackSky(unit->unit_type)) {
		return 13;
	}
	if (Util::IsDetectorType(unit->unit_type)) {
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

