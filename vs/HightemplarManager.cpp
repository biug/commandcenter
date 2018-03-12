#include "HightemplarManager.h"
#include "Util.h"
#include "CCBot.h"
#include <cmath>

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

	// for each meleeUnit
	bool refreshInfo = m_bot.Map().frame() % 16;
	for (auto Hightemplar : Hightemplars)
	{
		BOT_ASSERT(Hightemplar, "ranged unit is null");
		if (HightemplarInfos.find(Hightemplar) == HightemplarInfos.end())
		{
			HightemplarInfos[Hightemplar] = HightemplarInfo(Hightemplar->health);
		}
		float currentHP = Hightemplar->health;
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

				if (m_bot.State().m_stimpack)
				{
					auto abilities = m_bot.Query()->GetAbilitiesForUnit(Hightemplar);
					bool stimpack = false;

					for (auto & ab : abilities.abilities)
					{
						if (ab.ability_id.ToType() == sc2::ABILITY_ID::EFFECT_STIM)
						{
							stimpack = true;

						}
					}
					for (auto buff : Hightemplar->buffs) {
						if (buff == sc2::BUFF_ID::STIMPACK) {
							stimpack = false;
						}
					}
					if (stimpack && (beingAttack || Hightemplar->weapon_cooldown >0))
					{
						Micro::SmartAbility(Hightemplar, sc2::ABILITY_ID::EFFECT_STIM, m_bot);
					}
					//continue;
				}
				// kite attack it
				if (Util::IsMeleeUnit(target) && Hightemplar->weapon_cooldown > 0) {
					//auto p1 = target->pos, p2 = Hightemplar->pos;
					//auto tp = p2 * 2 - p1;
					sc2::Point2D rp = RetreatPosition(Hightemplar);
					Micro::SmartMove(Hightemplar, rp, m_bot);
				}
				else {
					Micro::SmartAttackMove(Hightemplar, target->pos, m_bot);
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

sc2::Point2D HightemplarManager::RetreatPosition(const sc2::Unit * unit)
{
	auto pos = unit->pos;
	sc2::Point2D base = m_bot.GetStartLocation();

	double angle = atan(pos.y - base.y / (pos.x - base.x));
	double step_size = 5;
	double step_x = step_size * sin(angle);
	double step_y = step_size * cos(angle);

	//std::stringstream ss;
	//ss << std::endl;
	//ss << "old position:      " << pos.x << " " << pos.y << std::endl;
	//ss << "new position:      " << (pos.x + step_x) << " " << pos.y + step_y << std::endl;
	//std::cout << ss.str();
	m_bot.Debug()->DebugTextOut("x", sc2::Point2D(pos.x + step_x, pos.y + step_y), sc2::Colors::White);

	return sc2::Point2D(pos.x + step_x, pos.y + step_y);

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



// get the attack priority of a type in relation to a zergling
int HightemplarManager::getAttackPriority(const sc2::Unit * attacker, const sc2::Unit * unit)
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

