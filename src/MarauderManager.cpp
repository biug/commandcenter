#include "MarauderManager.h"
#include "Util.h"
#include "CCBot.h"

MarauderInfo::MarauderInfo() :
	m_hpLastSecond(120)
{

}

MarauderInfo::MarauderInfo(float hp) :
	m_hpLastSecond(hp)
{

}

MarauderManager::MarauderManager(CCBot & bot)
	: MicroManager(bot)
{

}

void MarauderManager::executeMicro(const std::vector<const sc2::Unit *> & targets)
{
	assignTargets(targets);
}

void MarauderManager::assignTargets(const std::vector<const sc2::Unit *> & targets)
{
	const std::vector<const sc2::Unit *> & marauders = getUnits();

	// figure out targets
	std::vector<const sc2::Unit *> marauderTargets;
	for (auto target : targets)
	{
		if (!target) { continue; }
		if (target->unit_type == sc2::UNIT_TYPEID::ZERG_EGG) { continue; }
		if (target->unit_type == sc2::UNIT_TYPEID::ZERG_LARVA) { continue; }

		marauderTargets.push_back(target);
	}

	// for each meleeUnit
	bool refreshInfo = m_bot.Map().frame() % 16;
	for (auto marauder : marauders)
	{
		BOT_ASSERT(marauder, "ranged unit is null");
		if (marauderInfos.find(marauder) == marauderInfos.end())
		{
			marauderInfos[marauder] = MarauderInfo(marauder->health);
		}
		float currentHP = marauder->health;
		bool beingAttack = currentHP < marauderInfos[marauder].m_hpLastSecond;
		if (refreshInfo) marauderInfos[marauder].m_hpLastSecond = currentHP;
		
		// if the order is to attack or defend
		if (order.getType() == SquadOrderTypes::Attack || order.getType() == SquadOrderTypes::Defend)
		{
			if (!marauderTargets.empty())
			{
				// find the best target for this meleeUnit
				const sc2::Unit * target = getTarget(marauder, marauderTargets);
				if (!target) continue;

				if (m_bot.State().m_stimpack)
				{
					auto abilities = m_bot.Query()->GetAbilitiesForUnit(marauder);
					bool stimpack = false;
					
					for (auto & ab : abilities.abilities)
					{
						if (ab.ability_id.ToType() == sc2::ABILITY_ID::EFFECT_STIM)
						{
							stimpack = true;
							
						}
					}
					for (auto buff : marauder->buffs) {
						if (buff == sc2::BUFF_ID::STIMPACKMARAUDER) {
							stimpack = false;
						}
					}
					if (stimpack && (beingAttack || marauder->weapon_cooldown >0))
					{
						std::cout << "abality EFFECT_STIM " << std::endl;
						Micro::SmartAbility(marauder, sc2::ABILITY_ID::EFFECT_STIM,m_bot);
					}
					//continue;
				}
				//std::cout << "weapon_cooldown " << marauder->weapon_cooldown <<std::endl;
				// kite attack it
				if (Util::IsMeleeUnit(target) && marauder->weapon_cooldown > 0) {
					//auto p1 = target->pos, p2 = marauder->pos;
					//auto tp = p2 * 2 - p1;
					sc2::Point2D rp = RetreatPosition(marauder);
					Micro::SmartMove(marauder, rp, m_bot);
					std::cout << "kite attack" << std::endl;
				}
				else {
					Micro::SmartAttackMove(marauder, target->pos, m_bot);
				}
			}
			// if there are no targets
			else
			{
				// if we're not near the order position
				if (Util::Dist(marauder->pos, order.getPosition()) > 4)
				{
					// move to it
					Micro::SmartMove(marauder, order.getPosition(), m_bot);
				}
			}
		}

		if (m_bot.Config().DrawUnitTargetInfo)
		{
			// TODO: draw the line to the unit's target
		}
	}
}

sc2::Point2D MarauderManager::RetreatPosition(const sc2::Unit * unit)
{
	auto pos = unit->pos;
	sc2::Point2D base = m_bot.GetStartLocation();

	double angle = atan(pos.y - base.y / (pos.x - base.x));
	double step_size = 5;
	double step_x = step_size * sin(angle);
	double step_y = step_size * cos(angle);

	std::stringstream ss;
	ss << std::endl;
	ss << "old position:      " << pos.x << " " << pos.y << std::endl;
	ss << "new position:      " << (pos.x + step_x) << " " << pos.y + step_y << std::endl;
	std::cout << ss.str();
	m_bot.Debug()->DebugTextOut("x", sc2::Point2D(pos.x + step_x, pos.y + step_y), sc2::Colors::White);

	return sc2::Point2D(pos.x + step_x, pos.y + step_y);

}

// get a target for the ranged unit to attack
// TODO: this is the melee targeting code, replace it with something better for ranged units
const sc2::Unit * MarauderManager::getTarget(const sc2::Unit * rangedUnit, const std::vector<const sc2::Unit *> & targets)
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
int MarauderManager::getAttackPriority(const sc2::Unit * attacker, const sc2::Unit * unit)
{
	BOT_ASSERT(unit, "null unit in getAttackPriority");
	if (Util::IsHeavyArmor(unit))
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

