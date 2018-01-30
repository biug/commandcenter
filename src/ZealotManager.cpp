#include "ZealotManager.h"
#include "Util.h"
#include "CCBot.h"

ZealotInfo::ZealotInfo() :
	m_hpLastSecond(45)
{

}

ZealotInfo::ZealotInfo(float hp) :
	m_hpLastSecond(hp)
{

}

ZealotManager::ZealotManager(CCBot & bot)
	: MicroManager(bot)
{

}

void ZealotManager::executeMicro(const std::vector<const sc2::Unit *> & targets)
{
	assignTargets(targets);
}

void ZealotManager::assignTargets(const std::vector<const sc2::Unit *> & targets)
{
	const std::vector<const sc2::Unit *> & Zealots = getUnits();

	// figure out targets
	std::vector<const sc2::Unit *> ZealotTargets;
	for (auto target : targets)
	{
		if (!target) { continue; }
		if (target->unit_type == sc2::UNIT_TYPEID::ZERG_EGG) { continue; }
		if (target->unit_type == sc2::UNIT_TYPEID::ZERG_LARVA) { continue; }

		ZealotTargets.push_back(target);
	}
	bool refreshInfo = m_bot.Map().frame() % 16;
	for (auto Zealot : Zealots)
	{
		BOT_ASSERT(Zealot, "ranged unit is null");
		if (ZealotInfos.find(Zealot) == ZealotInfos.end())
		{
			ZealotInfos[Zealot] = ZealotInfo(Zealot->health + Zealot->shield);
		}
		float currentHP = Zealot->health + Zealot->shield;
		bool beingAttack = currentHP < ZealotInfos[Zealot].m_hpLastSecond;
		if (refreshInfo) ZealotInfos[Zealot].m_hpLastSecond = currentHP;

		// if the order is to attack or defend
		if (order.getType() == SquadOrderTypes::Attack || order.getType() == SquadOrderTypes::Defend)
		{
			if (!ZealotTargets.empty())
			{
				// find the best target for this meleeUnit
				const sc2::Unit * target = getTarget(Zealot, ZealotTargets);
				if (!target) continue;

				if (Zealot->health + Zealot->shield < 50 && beingAttack)
				{
					auto abilities = m_bot.Query()->GetAbilitiesForUnit(Zealot);
					bool canCharge = false;
					for (auto & ab : abilities.abilities)
					{
						if (ab.ability_id.ToType() == sc2::ABILITY_ID::EFFECT_CHARGE)
						{
							canCharge = true;
						}
					}
					if (canCharge && (Util::Dist(Zealot->pos, target->pos)) <20 && (Util::Dist(Zealot->pos, target->pos))>4)
					{
						Micro::SmartAbility(Zealot, sc2::ABILITY_ID::EFFECT_CHARGE, target, m_bot);
					}
					continue;
				}
				// kite attack it
				if (Util::IsMeleeUnit(target) && Zealot->weapon_cooldown > 0) {
					sc2::Point2D rp = RetreatPosition(Zealot);
					Micro::SmartMove(Zealot, rp, m_bot);
				}
				else {
					Micro::SmartAttackMove(Zealot, target->pos, m_bot);
				}
			}
			// if there are no targets
			else
			{
				// if we're not near the order position
				if (Util::Dist(Zealot->pos, order.getPosition()) > 4)
				{
					// move to it
					Micro::SmartMove(Zealot, order.getPosition(), m_bot);
				}
			}
		}

		if (m_bot.Config().DrawUnitTargetInfo)
		{
			// TODO: draw the line to the unit's target
		}
	}

}


sc2::Point2D ZealotManager::RetreatPosition(const sc2::Unit * unit)
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
const sc2::Unit * ZealotManager::getTarget(const sc2::Unit * rangedUnit, const std::vector<const sc2::Unit *> & targets)
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
int ZealotManager::getAttackPriority(const sc2::Unit * attacker, const sc2::Unit * unit)
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

