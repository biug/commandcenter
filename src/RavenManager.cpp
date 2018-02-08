#include "RavenManager.h"
#include "Util.h"
#include "CCBot.h"

RavenInfo::RavenInfo() :
m_hpLastSecond(45)
{
	
}

RavenInfo::RavenInfo(float hp) :
	m_hpLastSecond(hp)
{

}

RavenManager::RavenManager(CCBot & bot)
	: MicroManager(bot)
{

}

void RavenManager::executeMicro(const std::vector<const sc2::Unit *> & targets)
{
	assignTargets(targets);
}

void RavenManager::assignTargets(const std::vector<const sc2::Unit *> & targets)
{
	std::cout << "Raven" << std::endl;
	const std::vector<const sc2::Unit *> & Ravens = getUnits();

	// figure out targets
	std::vector<const sc2::Unit *> RavenTargets;
	for (auto target : targets)
	{
		if (!target) { continue; }
		if (target->unit_type == sc2::UNIT_TYPEID::ZERG_EGG) { continue; }
		if (target->unit_type == sc2::UNIT_TYPEID::ZERG_LARVA) { continue; }

		RavenTargets.push_back(target);
	}

	// for each meleeUnit
	bool refreshInfo = m_bot.Map().frame() % 16;
	for (auto Raven : Ravens)
	{
		BOT_ASSERT(Raven, "ranged unit is null");
		if (RavenInfos.find(Raven) == RavenInfos.end())
		{
			RavenInfos[Raven] = RavenInfo(Raven->health);
		}
		float currentHP = Raven->health;
		bool beingAttack = currentHP < RavenInfos[Raven].m_hpLastSecond;
		if (refreshInfo) RavenInfos[Raven].m_hpLastSecond = currentHP;
		
		// if the order is to attack or defend
		if (order.getType() == SquadOrderTypes::Attack || order.getType() == SquadOrderTypes::Defend)
		{
			if (!RavenTargets.empty())
			{
				// find the best target for this meleeUnit
				const sc2::Unit * target = getTarget(Raven, RavenTargets);
				if (!target) continue;
				auto abilities = m_bot.Query()->GetAbilitiesForUnit(Raven);
				bool isTurretVaild = false;
				bool hasSelfCombatUnit = false;
				bool hasCloakingUnit = false;
				for (auto & ab : abilities.abilities)
				{
					if (ab.ability_id.ToType() == sc2::ABILITY_ID::EFFECT_AUTOTURRET)
					{
						isTurretVaild = true;

					}
				}
				for (auto & u : m_bot.Observation()->GetUnits())
				{
					if ((Util::GetPlayer(u) == Players::Self)&& Util::IsCombatUnit(u,m_bot))
					{
						hasSelfCombatUnit = true;
					}
					if ((Util::GetPlayer(u) == Players::Enemy) && (u->unit_type.ToType() == sc2::UNIT_TYPEID::TERRAN_BANSHEE ||
						u->unit_type.ToType() == sc2::UNIT_TYPEID::TERRAN_GHOST || u->unit_type.ToType() == sc2::UNIT_TYPEID::PROTOSS_DARKTEMPLAR))
					{
						hasCloakingUnit = true;
					}
				}
				if (isTurretVaild && Raven->energy >= 50)
				{
					sc2::Point2D tp = GetTurretPosition(target);
					Micro::SmartAbility(Raven, sc2::ABILITY_ID::EFFECT_AUTOTURRET, tp, m_bot);

				}
				else if (beingAttack && Raven->energy < 50 && !(hasSelfCombatUnit && hasCloakingUnit)) {
					sc2::Point2D rp = RetreatPosition(Raven);
					Micro::SmartMove(Raven, rp, m_bot);
				}
				else {
					Micro::SmartAttackMove(Raven, target->pos, m_bot);
				}
			}
			// if there are no targets
			else
			{
				// if we're not near the order position
				if (Util::Dist(Raven->pos, order.getPosition()) > 4)
				{
					// move to it
					Micro::SmartMove(Raven, order.getPosition(), m_bot);
				}
			}
		}

		if (m_bot.Config().DrawUnitTargetInfo)
		{
			// TODO: draw the line to the unit's target
		}
	}
}


sc2::Point2D RavenManager::GetTurretPosition(const sc2::Unit * unit)
{
	auto pos = unit->pos;
	sc2::Point2D base = m_bot.GetStartLocation();

	double angle = atan(pos.y - base.y / (pos.x - base.x));
	double step_size = 4;
	double step_x = step_size * sin(angle);
	double step_y = step_size * cos(angle);
	return sc2::Point2D(pos.x + step_x, pos.y + step_y);

}


sc2::Point2D RavenManager::RetreatPosition(const sc2::Unit * unit)
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
const sc2::Unit * RavenManager::getTarget(const sc2::Unit * rangedUnit, const std::vector<const sc2::Unit *> & targets)
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
int RavenManager::getAttackPriority(const sc2::Unit * attacker, const sc2::Unit * unit)
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

