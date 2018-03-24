#include "MarineManager.h"
#include "Util.h"
#include "CCBot.h"
#include <cmath>

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
		float currentHP = marine->health;
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
				if (beingAttack) {
					escapeFromEffect(marine);
				}
				
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
					if (stimpack && (beingAttack || marine->weapon_cooldown >0))
					{
						Micro::SmartAbility(marine, sc2::ABILITY_ID::EFFECT_STIM,m_bot);
					}
					//continue;
				}
				// kite attack it
				if (Util::IsMeleeUnit(target) && marine->weapon_cooldown > 0) {
					//auto p1 = target->pos, p2 = marine->pos;
					//auto tp = p2 * 2 - p1;
					sc2::Point2D rp  = RetreatPosition(marine);
					Micro::SmartMove(marine, rp, m_bot);
				}
				else {
					Micro::SmartAttackMove(marine, target->pos, m_bot);
				}
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

sc2::Point2D MarineManager::RetreatPosition(const sc2::Unit * unit)
{	
	auto pos = unit->pos;
	sc2::Point2D base = m_bot.GetStartLocation();

	double angle = atan(pos.y - base.y / (pos.x- base.x));
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

void MarineManager::escapeFromEffect(const sc2::Unit * rangedUnit)
{
	const std::vector<sc2::Effect> effects = m_bot.Observation()->GetEffects();
	bool escapefromeffect = false;
	for (const auto & effect : effects)
	{
		if (Util::isBadEffect(effect.effect_id))
		{
			const float radius = m_bot.Observation()->GetEffectData()[effect.effect_id].radius;
			for (const auto & pos : effect.positions)
			{
				if (Util::Dist(rangedUnit->pos, pos) < radius + 2.0f)
				{
					sc2::Point2D escapePos;
					if (effect.positions.size() == 1)
					{
						escapePos = pos + Util::normalizeVector(rangedUnit->pos - pos, radius + 2.0f);
					}
					else
					{
						const sc2::Point2D attackDirection = effect.positions.back() - effect.positions.front();
						//"Randomly" go right and left
						if (rangedUnit->tag % 2)
						{
							escapePos = rangedUnit->pos + Util::normalizeVector(sc2::Point2D(-attackDirection.x, attackDirection.y), radius + 2.0f);
						}
						else
						{
							escapePos = rangedUnit->pos - Util::normalizeVector(sc2::Point2D(-attackDirection.x, attackDirection.y), radius + 2.0f);
						}
					}
					Micro::SmartMove(rangedUnit, escapePos, m_bot);
					escapefromeffect = true;
					break;
				}
				if (escapefromeffect)
				{
					break;
				}
			}
		}
	}
}

// get a target for the ranged unit to attack
// TODO: this is the melee targeting code, replace it with something better for ranged units
const sc2::Unit * MarineManager::getTarget(const sc2::Unit * rangedUnit, const std::vector<const sc2::Unit *> & targets)
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
int MarineManager::getAttackPriority(const sc2::Unit * attacker, const sc2::Unit * unit)
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

