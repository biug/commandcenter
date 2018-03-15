#include "VikingManager.h"
#include "Util.h"
#include "CCBot.h"

VikingInfo::VikingInfo() :
	m_hpLastSecond(45)
{

}

VikingInfo::VikingInfo(float hp) :
	m_hpLastSecond(hp)
{

}

VikingManager::VikingManager(CCBot & bot)
	: MicroManager(bot)
{

}

void VikingManager::executeMicro(const std::vector<const sc2::Unit *> & targets)
{
	assignTargets(targets);
}

void VikingManager::assignTargets(const std::vector<const sc2::Unit *> & targets)
{
	const std::vector<const sc2::Unit *> & Vikings = getUnits();

	// figure out targets
	std::vector<const sc2::Unit *> VikingTargets;
	for (auto target : targets)
	{
		if (!target) { continue; }
		if (target->unit_type == sc2::UNIT_TYPEID::ZERG_EGG) { continue; }
		if (target->unit_type == sc2::UNIT_TYPEID::ZERG_LARVA) { continue; }
		VikingTargets.push_back(target);
	}

	// for each meleeUnit
	bool refreshInfo = m_bot.Map().frame() % 16;
	for (auto Viking : Vikings)
	{
		BOT_ASSERT(Viking, "ranged unit is null");
		if (VikingInfos.find(Viking) == VikingInfos.end())
		{
			VikingInfos[Viking] = VikingInfo(Viking->health);
		}
		float currentHP = Viking->health;
		bool beingAttack = currentHP < VikingInfos[Viking].m_hpLastSecond;
		if (refreshInfo) VikingInfos[Viking].m_hpLastSecond = currentHP;

		// if the order is to attack or defend
		if (order.getType() == SquadOrderTypes::Attack || order.getType() == SquadOrderTypes::Defend)
		{
			if (!VikingTargets.empty())
			{
				// find the best target for this meleeUnit
				const sc2::Unit * target = getTarget(Viking, VikingTargets);
				if (!target) continue;
				if (Util::IsWorker(target)) {
					Micro::SmartAttackMove(Viking, target->pos, m_bot);
				}
				if (target->is_flying)
				{
					if (Viking->unit_type.ToType() == sc2::UNIT_TYPEID::TERRAN_VIKINGASSAULT) {
						Micro::SmartAbility(Viking, sc2::ABILITY_ID::MORPH_VIKINGFIGHTERMODE, m_bot);
					}
					else {
						Micro::SmartAttackMove(Viking, target->pos, m_bot);
					}
				}else if (!target->is_flying) {
					if (Viking->unit_type.ToType() == sc2::UNIT_TYPEID::TERRAN_VIKINGFIGHTER) {
						Micro::SmartAbility(Viking, sc2::ABILITY_ID::MORPH_VIKINGASSAULTMODE, m_bot);
					}
					else {
						Micro::SmartAttackMove(Viking, target->pos, m_bot);
					}
					//if (Util::IsLightArmor(target) || Util::canAttackSky(target->unit_type)) {
					//	if (Viking->unit_type.ToType() == sc2::UNIT_TYPEID::TERRAN_VIKINGFIGHTER) {
					//		Micro::SmartAbility(Viking, sc2::ABILITY_ID::MORPH_VIKINGASSAULTMODE, m_bot);
					//	}
					//	else {
					//		Micro::SmartAttackMove(Viking, target->pos, m_bot);
					//	}
					//}
					//else {
					//	if (ShouldRetreat(Viking)) {
					//		sc2::Point2D fleeTo(m_bot.GetStartLocation());
					//		Micro::SmartMove(Viking, fleeTo, m_bot);
					//	}
					//}
				}
			}
			// if there are no targets
			else
			{
				// if we're not near the order position
				if (Util::Dist(Viking->pos, order.getPosition()) > 4)
				{
					// move to it
					Micro::SmartMove(Viking, order.getPosition(), m_bot);
				}
			}
		}

		if (m_bot.Config().DrawUnitTargetInfo)
		{
			// TODO: draw the line to the unit's target
		}
	}
}

bool VikingManager::ShouldRetreat(const sc2::Unit * Unit)
{
	float currentHP = Unit->health;
	bool beingAttack = currentHP < VikingInfos[Unit].m_hpLastSecond;
	// TODO: should melee units ever retreat?
	if (beingAttack && currentHP < Unit->health_max*0.2) {
		return true;
	}
	return false;
}

// get a target for the ranged unit to attack
// TODO: this is the melee targeting code, replace it with something better for ranged units
const sc2::Unit * VikingManager::getTarget(const sc2::Unit * rangedUnit, const std::vector<const sc2::Unit *> & targets)
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
int VikingManager::getAttackPriority(const sc2::Unit * attacker, const sc2::Unit * unit)
{
	if (unit->is_flying) {
		return 20;
	}
	if (Util::canAttackSky(unit->unit_type)) {
		return 15;
	}
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
		return 12;
	}

	return 1;
}

