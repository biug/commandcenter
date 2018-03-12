#include "ReaperManager.h"
#include "Util.h"
#include "CCBot.h"

ReaperInfo::ReaperInfo() :
	m_hpLastSecond(45)
{

}

ReaperInfo::ReaperInfo(float hp) :
	m_hpLastSecond(hp)
{

}

ReaperManager::ReaperManager(CCBot & bot)
	: MicroManager(bot)
{

}

void ReaperManager::executeMicro(const std::vector<const sc2::Unit *> & targets)
{
	assignTargets(targets);
}

void ReaperManager::assignTargets(const std::vector<const sc2::Unit *> & targets)
{
	const std::vector<const sc2::Unit *> & Reapers = getUnits();

	// figure out targets
	std::vector<const sc2::Unit *> ReaperTargets;
	for (auto target : targets)
	{
		if (!target) { continue; }
		if (target->unit_type == sc2::UNIT_TYPEID::ZERG_EGG) { continue; }
		if (target->unit_type == sc2::UNIT_TYPEID::ZERG_LARVA) { continue; }

		ReaperTargets.push_back(target);
	}

	// for each meleeUnit
	bool refreshInfo = m_bot.Map().frame() % 16;
	for (auto Reaper : Reapers)
	{
		BOT_ASSERT(Reaper, "ranged unit is null");
		if (ReaperInfos.find(Reaper) == ReaperInfos.end())
		{
			ReaperInfos[Reaper] = ReaperInfo(Reaper->health);
		}
		float currentHP = Reaper->health;
		bool beingAttack = currentHP < ReaperInfos[Reaper].m_hpLastSecond;
		if (refreshInfo) ReaperInfos[Reaper].m_hpLastSecond = currentHP;

		// if the order is to attack or defend
		if (order.getType() == SquadOrderTypes::Attack || order.getType() == SquadOrderTypes::Defend)
		{
			if (!ReaperTargets.empty())
			{
				// find the best target for this meleeUnit
				const sc2::Unit * target = getTarget(Reaper, ReaperTargets);
				if (!target) continue;
				if (Util::IsWorker(target)) {
					Micro::SmartAttackMove(Reaper, target->pos, m_bot);
				}
				// kite attack it
				else {
					SmartKiteTarget(Reaper, target);
				}
			}
			// if there are no targets
			else
			{
				// if we're not near the order position
				if (Util::Dist(Reaper->pos, order.getPosition()) > 4)
				{
					// move to it
					Micro::SmartMove(Reaper, order.getPosition(), m_bot);
				}
			}
		}

		if (m_bot.Config().DrawUnitTargetInfo)
		{
			// TODO: draw the line to the unit's target
		}
	}
}


#pragma region Advanced micro functionality
float ReaperManager::TimeToFaceEnemy(const sc2::Unit* unit, const sc2::Unit* target) const
{
	const float x_segment = abs(unit->pos.x - target->pos.x);
	const float y_segment = abs(unit->pos.y - target->pos.y);
	const float angle = atan(y_segment / x_segment) * 57.29f; // 57.29 = 180/pi

	return angle / 999; // 999 is the turning spead of reapers. 
}

// Warning: This funcition has no discrestion in what it kites. Be careful to not attack overlords with reapers!
void ReaperManager::SmartKiteTarget(const sc2::Unit* unit, const sc2::Unit* target) const
{
	assert(unit);
	assert(target);

	const float range = Util::GetAttackRange(unit->unit_type, m_bot);

	bool should_flee(true);

	// When passing a unit into PathingDistance, how the unit moves is taken into account.
	// EXAMPLE:: Reapers can cliffjump, Void Rays can fly over everything.
	const float dist(m_bot.Query()->PathingDistance(unit, target->pos));
	const float speed(m_bot.Observation()->GetUnitTypeData()[unit->unit_type].movement_speed);

	const float time_to_enter = (dist - range) / speed;

	// If we start moving back to attack, will our weapon be off cooldown?
	if ((time_to_enter >= unit->weapon_cooldown))
	{
		should_flee = false;
	}

	// Don't kite workers and buildings. 
	if (Util::IsBuilding(target->unit_type) || Util::IsWorker(target))
	{
		should_flee = false;
	}

	sc2::Point2D flee_position;

	// find the new coordinates.
	const float delta_x = unit->pos.x - target->pos.x;
	const float delta_y = unit->pos.y - target->pos.y;

	const float dist2 = Util::Dist(unit->pos, target->pos);

	const float new_x = delta_x * range / dist2 + target->pos.x;
	const float new_y = delta_y * range / dist2 + target->pos.y;

	const float fire_time = TimeToFaceEnemy(unit, target) + Util::GetAttackRate(unit->unit_type, m_bot) + 0.05f;

	// If we are in danger of dieing, run back to home base!
	// If we are danger of dieing while attacking
	if (unit->health <= Util::PredictFutureDPSAtPoint(unit->pos, fire_time, m_bot)
		// If we are danger of dieing while moving to attack a point.
		|| unit->health <= Util::DPSAtPoint(sc2::Point2D{ new_x, new_y }, m_bot))
	{
		// No matter what the other logic above says to do, RUN!
		should_flee = true;

		sc2::Point2D base = m_bot.GetStartLocation();
		Micro::SmartMove(unit, base, m_bot);

		//need to imporve by pathfinding
		//flee_position = sc2::Point2D{ static_cast<float>(m_bot.Config().ProxyLocationX),
		//	static_cast<float>(m_bot.Config().ProxyLocationY) };
		//m_bot.DebugHelper().DrawLine(unit->pos, sc2::Point2D{ new_x, new_y }, sc2::Colors::Red);
		//Pathfinding p;
		//p.SmartRunAway(unit, 20, bot_);
		return;
	}
	// Otherwise, kite if we are not close to death.
	else
	{
		flee_position = unit->pos - target->pos + unit->pos;
		//m_bot.DebugHelper().DrawLine(unit->pos, sc2::Point2D{ new_x, new_y }, sc2::Colors::Green);
	}

	// If we are on cooldown, run away.
	if (should_flee)
	{
		//bot_.DebugHelper().DrawLine(unit->pos, flee_position);
		flee_position = unit->pos - target->pos + unit->pos;
		Micro::SmartMove(unit, flee_position, m_bot);
	}
	// Otherwise go attack!
	else
	{
		// bot.DebugHelper().DrawLine(ranged_unit->pos, target->pos, sc2::Colors::Red);
		Micro::SmartAttackUnit(unit, target, m_bot);
	}
}

sc2::Point2D ReaperManager::RetreatPosition(const sc2::Unit * unit)
{
	auto pos = unit->pos;
	sc2::Point2D base = m_bot.GetStartLocation();

	double angle = atan(pos.y - base.y / (pos.x - base.x));
	double step_size = 10;
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
const sc2::Unit * ReaperManager::getTarget(const sc2::Unit * rangedUnit, const std::vector<const sc2::Unit *> & targets)
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
int ReaperManager::getAttackPriority(const sc2::Unit * attacker, const sc2::Unit * unit)
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
		return 12;
	}

	return 1;
}

