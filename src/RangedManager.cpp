#include "RangedManager.h"
#include "Util.h"
#include "CCBot.h"

RangedManager::RangedManager(CCBot & bot)
    : MicroManager(bot)
{

}

void RangedManager::executeMicro(const std::vector<const sc2::Unit *> & targets)
{
    assignTargets(targets);
}

void RangedManager::assignTargets(const std::vector<const sc2::Unit *> & targets)
{
    const std::vector<const sc2::Unit *> & rangedUnits = getUnits();

    // figure out targets
    std::vector<const sc2::Unit *> rangedUnitTargets;
    for (auto target : targets)
    {
        if (!target) { continue; }
        if (target->unit_type == sc2::UNIT_TYPEID::ZERG_EGG) { continue; }
        if (target->unit_type == sc2::UNIT_TYPEID::ZERG_LARVA) { continue; }

        rangedUnitTargets.push_back(target);
    }

    // for each meleeUnit
    for (auto rangedUnit : rangedUnits)
    {
		BOT_ASSERT(rangedUnit, "ranged unit is null");

        // if the order is to attack or defend
        if (order.getType() == SquadOrderTypes::Attack || order.getType() == SquadOrderTypes::Defend)
        {
            if (!rangedUnitTargets.empty())
            {
                // find the best target for this meleeUnit
                const sc2::Unit * target = getTarget(rangedUnit, rangedUnitTargets);
				
                // attack it
                if (m_bot.Config().KiteWithRangedUnits)
                {
                    Micro::SmartKiteTarget(rangedUnit, target, m_bot);
                }
                else
                {
                    Micro::SmartAttackUnit(rangedUnit, target, m_bot);
                }
            }
            // if there are no targets
            else
            {
                // if we're not near the order position
                if (Util::Dist(rangedUnit->pos, order.getPosition()) > 4)
                {
                    // move to it
                    Micro::SmartMove(rangedUnit, order.getPosition(), m_bot);
                }
            }
        }

        if (m_bot.Config().DrawUnitTargetInfo)
        {
            // TODO: draw the line to the unit's target
        }
    }
}

void RangedManager::escapeFromEffect(const sc2::Unit * rangedUnit)
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
const sc2::Unit * RangedManager::getTarget(const sc2::Unit * rangedUnit, const std::vector<const sc2::Unit *> & targets)
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
int RangedManager::getAttackPriority(const sc2::Unit * attacker, const sc2::Unit * unit)
{
    BOT_ASSERT(unit, "null unit in getAttackPriority");

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

