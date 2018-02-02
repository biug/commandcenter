#include "SentryManager.h"
#include "Util.h"
#include "CCBot.h"

SentryInfo::SentryInfo() :
	m_hpLastSecond(45)
{

}

SentryInfo::SentryInfo(float hp) :
	m_hpLastSecond(hp)
{

}

SentryManager::SentryManager(CCBot & bot)
	: MicroManager(bot)
{

}

void SentryManager::executeMicro(const std::vector<const sc2::Unit *> & targets)
{
	assignTargets(targets);
}

void SentryManager::assignTargets(const std::vector<const sc2::Unit *> & targets)
{
	const std::vector<const sc2::Unit *> & Sentrys = getUnits();

	// figure out targets
	std::vector<const sc2::Unit *> SentryTargets;
	for (auto target : targets)
	{
		if (!target) { continue; }
		if (target->unit_type == sc2::UNIT_TYPEID::ZERG_EGG) { continue; }
		if (target->unit_type == sc2::UNIT_TYPEID::ZERG_LARVA) { continue; }

		SentryTargets.push_back(target);
	}

	// for each meleeUnit
	bool refreshInfo = m_bot.Map().frame() % 16;
	for (auto Sentry : Sentrys)
	{
		BOT_ASSERT(Sentry, "ranged unit is null");
		if (SentryInfos.find(Sentry) == SentryInfos.end())
		{
			SentryInfos[Sentry] = SentryInfo(Sentry->health + Sentry->shield);
		}
		float currentHP = Sentry->health + Sentry->shield;
		bool beingAttack = currentHP < SentryInfos[Sentry].m_hpLastSecond;
		if (refreshInfo) SentryInfos[Sentry].m_hpLastSecond = currentHP;

		// if the order is to attack or defend
		if (order.getType() == SquadOrderTypes::Attack || order.getType() == SquadOrderTypes::Defend)
		{
			if (!SentryTargets.empty())
			{
				// find the best target for this meleeUnit
				const sc2::Unit * target = getTarget(Sentry, SentryTargets);
				if (!target) continue;
				auto abilities = m_bot.Query()->GetAbilitiesForUnit(Sentry);
				bool guardianshield = false;
				bool forcefield = false;
				for (auto & ab : abilities.abilities)
				{
					if (ab.ability_id.ToType() == sc2::ABILITY_ID::EFFECT_GUARDIANSHIELD)
					{
						guardianshield = true;
					}
					else if (ab.ability_id.ToType() == sc2::ABILITY_ID::EFFECT_FORCEFIELD)
					{
						forcefield = true;
					}

				}
				for (auto buff : Sentry->buffs) {
					if (buff == sc2::BUFF_ID::GUARDIANSHIELD) {
						guardianshield = false;
					}
				}
				int nearbyselfnum = 1;
				for (auto selfunit : Sentrys)
				{
					if (Util::Dist(Sentry->pos, selfunit->pos) <= 10 && selfunit->alliance == 1)
					{
						nearbyselfnum++;
					}
				}
				if (guardianshield && beingAttack && (nearbyselfnum >= 3 || currentHP <= 40))
				{
					Micro::SmartAbility(Sentry, sc2::ABILITY_ID::EFFECT_GUARDIANSHIELD, m_bot);
				}
				if (forcefield && beingAttack && (Sentrys.size() <= SentryTargets.size()))
				{
					sc2::Point2D fp = SetForcefieldPosition(Sentry, SentryTargets);
					Micro::SmartAbility(Sentry, sc2::ABILITY_ID::EFFECT_FORCEFIELD, fp, m_bot);
				}
				Micro::SmartAttackMove(Sentry, target->pos, m_bot);

			}
			// if there are no targets
			else
			{
				// if we're not near the order position
				if (Util::Dist(Sentry->pos, order.getPosition()) > 4)
				{
					// move to it
					Micro::SmartMove(Sentry, order.getPosition(), m_bot);
				}
			}
		}

		if (m_bot.Config().DrawUnitTargetInfo)
		{
			// TODO: draw the line to the unit's target
		}
	}
}

sc2::Point2D SentryManager::SetForcefieldPosition(const sc2::Unit * rangedUnit, const std::vector<const sc2::Unit *> & targets)
{
	const sc2::Unit *ClosestTarget = nullptr;
	const sc2::Unit *FarthestTarget = nullptr;
	float ClosestDistance = std::numeric_limits<float>::max();
	float FarthestDistance = 0;
	for (auto targetUnit : targets)
	{
		if (!targetUnit->is_flying)
		{
			float distance = Util::Dist(rangedUnit->pos, targetUnit->pos);
			if (distance < ClosestDistance)
			{
				ClosestDistance = distance;
				ClosestTarget = targetUnit;
			}
			else if (distance > FarthestDistance)
			{
				FarthestDistance = distance;
				FarthestTarget = targetUnit;
			}
		}

	}
	return sc2::Point2D((ClosestTarget->pos.x + FarthestTarget->pos.x) / 2, (ClosestTarget->pos.y + FarthestTarget->pos.y) / 2);
}

// get a target for the ranged unit to attack
// TODO: this is the melee targeting code, replace it with something better for ranged units
const sc2::Unit * SentryManager::getTarget(const sc2::Unit * rangedUnit, const std::vector<const sc2::Unit *> & targets)
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
int SentryManager::getAttackPriority(const sc2::Unit * attacker, const sc2::Unit * unit)
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

