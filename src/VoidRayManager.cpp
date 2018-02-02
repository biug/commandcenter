#include "VoidRayManager.h"
#include "Util.h"
#include "CCBot.h"

VoidRayInfo::VoidRayInfo() :
	m_hpLastSecond(45)
{

}

VoidRayInfo::VoidRayInfo(float hp) :
	m_hpLastSecond(hp)
{

}



VoidRayManager::VoidRayManager(CCBot & bot)
	: MicroManager(bot)
{

}

void VoidRayManager::executeMicro(const std::vector<const sc2::Unit *> & targets)
{
	assignTargets(targets);
}
void VoidRayManager::assignTargets(const std::vector<const sc2::Unit *> & targets) {
	const std::vector<const sc2::Unit *> & VoidRays = getUnits();

	// figure out targets
	std::vector<const sc2::Unit *> VoidRayTargets;
	for (auto target : targets)
	{
		if (!target) { continue; }
		if (target->unit_type == sc2::UNIT_TYPEID::ZERG_EGG) { continue; }
		if (target->unit_type == sc2::UNIT_TYPEID::ZERG_LARVA) { continue; }

		VoidRayTargets.push_back(target);
	}

	// for each meleeUnit
	bool refreshInfo = m_bot.Map().frame() % 16;
	for (auto VoidRay : VoidRays)
	{
		BOT_ASSERT(VoidRay, "ranged unit is null");
		if (VoidRayInfos.find(VoidRay) == VoidRayInfos.end())
		{
			VoidRayInfos[VoidRay] = VoidRayInfo(VoidRay->health + VoidRay->shield);
		}
		float currentHP = VoidRay->health + VoidRay->shield;
		bool beingAttack = currentHP < VoidRayInfos[VoidRay].m_hpLastSecond;
		if (refreshInfo) VoidRayInfos[VoidRay].m_hpLastSecond = currentHP;

		// if the order is to attack or defend
		if (order.getType() == SquadOrderTypes::Attack || order.getType() == SquadOrderTypes::Defend)
		{
			if (!VoidRayTargets.empty())
			{
				// find the best target for this meleeUnit
				const sc2::Unit * target = getTarget(VoidRay, VoidRayTargets);
				if (!target) continue;

				auto abilities = m_bot.Query()->GetAbilitiesForUnit(VoidRay);
				bool voidrayprismaticalignment = false;

				for (auto & ab : abilities.abilities)
				{
					if (ab.ability_id.ToType() == sc2::ABILITY_ID::EFFECT_VOIDRAYPRISMATICALIGNMENT)
					{
						voidrayprismaticalignment = true;

					}
				}
				if (voidrayprismaticalignment && Util::IsHeavyArmor(target))
				{
					Micro::SmartAbility(VoidRay, sc2::ABILITY_ID::EFFECT_VOIDRAYPRISMATICALIGNMENT, m_bot);
				}
				else {
					Micro::SmartAttackMove(VoidRay, target->pos, m_bot);
				}

			}
			// if there are no targets
			else
			{
				// if we're not near the order position
				if (Util::Dist(VoidRay->pos, order.getPosition()) > 4)
				{
					// move to it
					Micro::SmartMove(VoidRay, order.getPosition(), m_bot);
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
const sc2::Unit * VoidRayManager::getTarget(const sc2::Unit * rangedUnit, const std::vector<const sc2::Unit *> & targets)
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
int VoidRayManager::getAttackPriority(const sc2::Unit * attacker, const sc2::Unit * unit)
{
	BOT_ASSERT(unit, "null unit in getAttackPriority");
	if (Util::IsHeavyArmor(unit))
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

