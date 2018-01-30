#include "MedivacManager.h"
#include "Util.h"
#include "CCBot.h"

MedivacInfo::MedivacInfo() :
	m_hpLastSecond(150)
{

}

MedivacInfo::MedivacInfo(float hp) :
	m_hpLastSecond(hp)
{

}

MedivacManager::MedivacManager(CCBot & bot)
	: MicroManager(bot)
{

}
/**
void MedivacManager::executeMicro(const std::vector<const sc2::Unit *> & targets)
{
	assignTargets(targets);
}
**/

void MedivacManager::executeMicro(const std::vector<const sc2::Unit *> & targets){ //unfinished

	const std::vector<const sc2::Unit *> & Medivacs = getUnits();

	std::vector<const sc2::Unit *> nearbySelf;
	for (auto & u : m_bot.Observation()->GetUnits())
	{
		if (Util::GetPlayer(u) == Players::Self && 
			(u->unit_type.ToType() == sc2::UNIT_TYPEID::TERRAN_MARINE 
				|| u->unit_type.ToType() == sc2::UNIT_TYPEID::TERRAN_MARAUDER 
				|| u->unit_type.ToType() == sc2::UNIT_TYPEID::TERRAN_REAPER)) {
				nearbySelf.push_back(u);
			}
	}
	bool refreshInfo = m_bot.Map().frame() % 16;

	for (auto Medivac : Medivacs) {
		BOT_ASSERT(Medivac, "ranged unit is null");
		if (MedivacInfos.find(Medivac) == MedivacInfos.end())
		{
			MedivacInfos[Medivac] = MedivacInfo(Medivac->health);
		}
		
		float currentHP = Medivac->health;

		if (refreshInfo) MedivacInfos[Medivac].m_hpLastSecond = currentHP;

		if (!nearbySelf.empty()) {
			// find the best target for this meleeUnit
			const sc2::Unit * target = getTarget(Medivac, nearbySelf);

				//auto p1 = target->pos, p2 = Medivac->pos;
				//auto tp = p2 * 2 - p1;
			Micro::SmartMove(Medivac, target->pos, m_bot);
			/**
			if (target->health < target->health_max*0.2) {
				loadUnit(target, Medivac);
				if (ShouldRetreat(Medivac)) {
					sc2::Point2D fleeTo(m_bot.GetStartLocation());
					Micro::SmartMove(Medivac, fleeTo, m_bot);
				}
			}
			bool safe = true;
			for (auto & enemyUnit : targets)
			{
				if (Util::Dist(enemyUnit->pos, Medivac->pos) < 10)
				{
					safe = false;
				}
			}
			if (safe) {
				unloadUnit(target, Medivac);
			}
			**/
		}

	}


	/**
	for (auto Medivac : Medivacs) {
		auto abilities = m_bot.Query()->GetAbilitiesForUnit(Medivac);
		bool loaded = false;
		sc2::Unit * friend_unit;
		for (auto & ab : abilities.abilities)
		{
			if (loaded == false || ab.ability_id.ToType() == sc2::ABILITY_ID::LOAD_MEDIVAC)
			{
				Micro::SmartAbility(Medivac, sc2::ABILITY_ID::LOAD_MEDIVAC, friend_unit, m_bot);
				loaded = true;

			}

			if (loaded == true || ab.ability_id.ToType() == sc2::ABILITY_ID::UNLOADALLAT_MEDIVAC)
			{
				Micro::SmartAbility(Medivac, sc2::ABILITY_ID::UNLOADALLAT_MEDIVAC, friend_unit, m_bot);
				loaded = false;

			}
		}
	}
	**/
}


bool MedivacManager::loadUnit(const sc2::Unit * Unit, const sc2::Unit * Medivac)
{
	auto abilities = m_bot.Query()->GetAbilitiesForUnit(Medivac);
	for (auto & ab : abilities.abilities)
	{
		if (ab.ability_id.ToType() == sc2::ABILITY_ID::LOAD_MEDIVAC)
		{
			Micro::SmartAbility(Medivac, sc2::ABILITY_ID::LOAD_MEDIVAC, Unit, m_bot);
			return true;
		}
	}
	return false;
}

bool MedivacManager::unloadUnit(const sc2::Unit * Unit, const sc2::Unit * Medivac)
{
	auto abilities = m_bot.Query()->GetAbilitiesForUnit(Medivac);
	for (auto & ab : abilities.abilities)
	{
		if (ab.ability_id.ToType() == sc2::ABILITY_ID::UNLOADALLAT_MEDIVAC)
		{
			Micro::SmartAbility(Medivac, sc2::ABILITY_ID::UNLOADALLAT_MEDIVAC, Unit, m_bot);
			return true;
		}
	}
	return false;
}

bool MedivacManager::ShouldRetreat(const sc2::Unit * Unit)
{	
	float currentHP = Unit->health;
	bool beingAttack = currentHP < MedivacInfos[Unit].m_hpLastSecond;
	// TODO: should melee units ever retreat?
	if (beingAttack && currentHP < Unit->health_max*0.2) {
		return true;
	}
	return false;
}

// get a target for the ranged unit to attack
// TODO: this is the melee targeting code, replace it with something better for ranged units
const sc2::Unit * MedivacManager::getTarget(const sc2::Unit * rangedUnit, const std::vector<const sc2::Unit *> & targets)
{
	BOT_ASSERT(rangedUnit, "null melee unit in getTarget");

	float min_health = targets.front()->health_max;
	double closestDist = std::numeric_limits<double>::max();
	const sc2::Unit * closestTarget = nullptr;

	// for each target possiblity
	for (auto targetUnit : targets)
	{
		BOT_ASSERT(targetUnit, "null target unit in getTarget");

		float distance = Util::Dist(rangedUnit->pos, targetUnit->pos);

		// if it's a higher priority, or it's closer, set it
		if (!closestTarget || (targetUnit->health < min_health) || (targetUnit->health == min_health && distance < closestDist))
		{
			closestDist = distance;
			min_health = targetUnit->health;
			closestTarget = targetUnit;
		}
	}

	return closestTarget;
}

