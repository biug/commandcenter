#include "ZerglingManager.h"
#include "Util.h"
#include "CCBot.h"

ZerglingInfo::ZerglingInfo() :
	m_hpLastSecond(45)
{

}

ZerglingInfo::ZerglingInfo(float hp) :
	m_hpLastSecond(hp)
{

}

ZerglingManager::ZerglingManager(CCBot & bot)
	: MicroManager(bot)
{

}

void ZerglingManager::executeMicro(const std::vector<const sc2::Unit *> & targets)
{
	assignTargets(targets);
}

void ZerglingManager::assignTargets(const std::vector<const sc2::Unit *> & targets)
{
	const std::vector<const sc2::Unit *> & Zerglings = getUnits();

	// figure out targets
	std::vector<const sc2::Unit *> ZerglingTargets;
	for (auto target : targets)
	{
		if (!target) { continue; }
		if (target->unit_type == sc2::UNIT_TYPEID::ZERG_EGG) { continue; }
		if (target->unit_type == sc2::UNIT_TYPEID::ZERG_LARVA) { continue; }

		ZerglingTargets.push_back(target);
	}

	double morphrate = 0.2;
	double banelingnum = 0;
	double num = Zerglings.size()* morphrate;
	std::cout << "Zerglings.size() " << Zerglings.size() << std::endl;
	std::cout << "num " << num << std::endl;

	for (auto Zergling : Zerglings) {
		if (Zergling->unit_type.ToType() == sc2::UNIT_TYPEID::ZERG_BANELING) {
			banelingnum += 1;
		}
	}

	std::cout << "banelingnum " << banelingnum << std::endl;
	num -= banelingnum;
	std::cout << "num " << num << std::endl;
	if (Zerglings.size() > 10 && num >0) {
		for (auto Zergling : Zerglings) {
			auto abilities = m_bot.Query()->GetAbilitiesForUnit(Zergling);
			bool canmorph = false;
			if (Zergling->unit_type.ToType() == sc2::UNIT_TYPEID::ZERG_BANELING) {
				continue;
			}
			if (num < 0) {
				break;
			}
			for (auto & ab : abilities.abilities)
			{
				if (ab.ability_id.ToType() == sc2::ABILITY_ID::TRAIN_BANELING)
				{
					canmorph = true;
					Micro::SmartAbility(Zergling, sc2::ABILITY_ID::TRAIN_BANELING, m_bot);
					num -= 1;
				}
			}
		}
	 }
	// for each meleeUnit
	bool refreshInfo = m_bot.Map().frame() % 16;
	for (auto Zergling : Zerglings)
	{
		BOT_ASSERT(Zergling, "ranged unit is null");
		if (ZerglingInfos.find(Zergling) == ZerglingInfos.end())
		{
			ZerglingInfos[Zergling] = ZerglingInfo(Zergling->health);
		}
		float currentHP = Zergling->health;
		bool beingAttack = currentHP < ZerglingInfos[Zergling].m_hpLastSecond;
		if (refreshInfo) ZerglingInfos[Zergling].m_hpLastSecond = currentHP;

		std::vector<const sc2::Unit *> Banelings;
		// if the order is to attack or defend
		if (order.getType() == SquadOrderTypes::Attack || order.getType() == SquadOrderTypes::Defend)
		{
			if (Zergling->unit_type.ToType() == sc2::UNIT_TYPEID::ZERG_BANELING) {
				Banelings.push_back(Zergling);
				if (!ZerglingTargets.empty())
				{
					// find the best target for this meleeUnit
					const sc2::Unit * target = getTarget(Zergling, ZerglingTargets);
					if (!target) continue;
					Micro::SmartAttackMove(Zergling, target->pos, m_bot);
				}
				// if there are no targets
				else
				{
					// if we're not near the order position
					if (Util::Dist(Zergling->pos, order.getPosition()) > 4)
					{
						// move to it
						Micro::SmartMove(Zergling, order.getPosition(), m_bot);
					}
				}
			}
			// zergling micro act
			else {
				if (!ZerglingTargets.empty())
				{
					// find the best target for this meleeUnit
					const sc2::Unit * target = getTarget(Zergling, ZerglingTargets);
					if (!target) continue;
					else {
						Micro::SmartAttackMove(Zergling, target->pos, m_bot);
					}
				}
				// if there are no targets
				else
				{
					//  speed down to follow baneling
					if (Banelings.size()>0) {
						double closestDist = Util::Dist(Banelings[0]->pos, Zergling->pos);
						const sc2::Unit *  closestBaneling = Banelings[0];
						for (auto baneling : Banelings) {
							float distance = Util::Dist(Zergling->pos, baneling->pos);
							if (distance > closestDist) {
								closestDist = distance;
								closestBaneling = baneling;
							}
						}
						Micro::SmartMove(Zergling, closestBaneling->pos, m_bot);

					}
					// if we're not near the order position
					else if (Util::Dist(Zergling->pos, order.getPosition()) > 4)
					{
						// move to it
						Micro::SmartMove(Zergling, order.getPosition(), m_bot);
					}
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
const sc2::Unit * ZerglingManager::getTarget(const sc2::Unit * rangedUnit, const std::vector<const sc2::Unit *> & targets)
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
int ZerglingManager::getAttackPriority(const sc2::Unit * attacker, const sc2::Unit * unit)
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

