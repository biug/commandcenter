#include "PhoenixManager.h"
#include "Util.h"
#include "CCBot.h"

PhoenixInfo::PhoenixInfo() :
	m_hpLastSecond(45)
{

}

PhoenixInfo::PhoenixInfo(float hp) :
	m_hpLastSecond(hp)
{

}

PhoenixManager::PhoenixManager(CCBot & bot)
	: MicroManager(bot)
{

}

void PhoenixManager::executeMicro(const std::vector<const sc2::Unit *> & targets)
{
	assignTargets(targets);
}

void PhoenixManager::assignTargets(const std::vector<const sc2::Unit *> & targets)
{
	const std::vector<const sc2::Unit *> & Phoenixs = getUnits();

	// figure out targets
	std::vector<const sc2::Unit *> PhoenixTargets;
	for (auto target : targets)
	{
		if (!target) { continue; }
		if (target->unit_type == sc2::UNIT_TYPEID::ZERG_EGG) { continue; }
		if (target->unit_type == sc2::UNIT_TYPEID::ZERG_LARVA) { continue; }

		PhoenixTargets.push_back(target);
	}

	// for each meleeUnit
	bool refreshInfo = m_bot.Map().frame() % 16;
	for (auto Phoenix : Phoenixs)
	{
		BOT_ASSERT(Phoenix, "ranged unit is null");
		if (PhoenixInfos.find(Phoenix) == PhoenixInfos.end())
		{
			PhoenixInfos[Phoenix] = PhoenixInfo(Phoenix->health + Phoenix->shield);
		}
		float currentHP = Phoenix->health + Phoenix->shield;
		bool beingAttack = currentHP < PhoenixInfos[Phoenix].m_hpLastSecond;
		if (refreshInfo) PhoenixInfos[Phoenix].m_hpLastSecond = currentHP;

		// if the order is to attack or defend
		if (order.getType() == SquadOrderTypes::Attack || order.getType() == SquadOrderTypes::Defend)
		{
			if (!PhoenixTargets.empty())
			{
				// find the best target for this meleeUnit
				const sc2::Unit * target = getTarget(Phoenix, PhoenixTargets);
				if (!target) continue;

				auto abilities = m_bot.Query()->GetAbilitiesForUnit(Phoenix);
				bool gravitonbeam = false;
				bool selfunitcanattacksky = false;
				int selfunitcanattackskynum = 0;

				for (auto selfunit : Phoenixs)
				{
					if (selfunit->is_flying || Util::canAttackSky(selfunit->unit_type)) {
						selfunitcanattackskynum++;
					}
				}
				if (selfunitcanattackskynum >= 3) {
					selfunitcanattacksky = true;
				}

				for (auto & ab : abilities.abilities)
				{
					if (ab.ability_id.ToType() == sc2::ABILITY_ID::EFFECT_GRAVITONBEAM)
					{
						gravitonbeam = true;

					}
				}
				for (auto buff : Phoenix->buffs) {
					if (buff == sc2::BUFF_ID::GRAVITONBEAM) {
						gravitonbeam = false;
					}
				}
				if (target->is_flying || target->unit_type.ToType() == sc2::UNIT_TYPEID::PROTOSS_COLOSSUS)
				{
					Micro::SmartAttackMove(Phoenix, target->pos, m_bot);
				}
				else if (gravitonbeam && !target->is_flying && selfunitcanattacksky)
				{
					Micro::SmartAbility(Phoenix, sc2::ABILITY_ID::EFFECT_GRAVITONBEAM, target, m_bot);
				}
				else if (beingAttack && (Phoenix->weapon_cooldown > 0 || Phoenix->energy<50 || !selfunitcanattacksky) && !target->is_flying) {
					sc2::Point2D rp = RetreatPosition(Phoenix);
					Micro::SmartMove(Phoenix, rp, m_bot);
				}
				else {
					Micro::SmartAttackMove(Phoenix, target->pos, m_bot);
				}
			}
			// if there are no targets
			else
			{
				// if we're not near the order position
				if (Util::Dist(Phoenix->pos, order.getPosition()) > 4)
				{
					// move to it
					Micro::SmartMove(Phoenix, order.getPosition(), m_bot);
				}
			}
		}

		if (m_bot.Config().DrawUnitTargetInfo)
		{
			// TODO: draw the line to the unit's target
		}
	}
}

sc2::Point2D PhoenixManager::RetreatPosition(const sc2::Unit * unit)
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
const sc2::Unit * PhoenixManager::getTarget(const sc2::Unit * rangedUnit, const std::vector<const sc2::Unit *> & targets)
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
int PhoenixManager::getAttackPriority(const sc2::Unit * attacker, const sc2::Unit * unit)
{
	BOT_ASSERT(unit, "null unit in getAttackPriority");
	if (unit->is_flying)
	{
		return 14;
	}
	if (Util::canAttackSky(unit->unit_type))
	{
		return 13;
	}
	if (Util::IsLightArmor(unit))
	{
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

