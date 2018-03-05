#include "ScoutManager.h"
#include "CCBot.h"
#include "Util.h"
#include "Micro.h"

ScoutManager::ScoutManager(CCBot & bot)
    : m_bot             (bot)
    , m_scoutUnit       (nullptr)
    , m_numScouts       (0)
    , m_scoutUnderAttack(false)
    , m_scoutStatus     ("None")
    , m_previousScoutHP (0.0f)
	, m_arriveEnemyStart(false)

{
}

void ScoutManager::onStart()
{

}

void ScoutManager::onFrame()
{
	
		moveScouts();
	
    drawScoutInformation();
}

void ScoutManager::setWorkerScout(const sc2::Unit * tag)
{
    // if we have a previous worker scout, release it back to the worker manager
    if (m_scoutUnit)
    {
        m_bot.Workers().finishedWithWorker(m_scoutUnit);
    }

    m_scoutUnit = tag;
    m_bot.Workers().setScoutWorker(m_scoutUnit);
}

void ScoutManager::drawScoutInformation()
{
    if (!m_bot.Config().DrawScoutInfo)
    {
        return;
    }

    std::stringstream ss;
    ss << "Scout Info: " << m_scoutStatus;

    m_bot.Map().drawTextScreen(sc2::Point2D(0.1f, 0.6f), ss.str());
}

void ScoutManager::moveScouts()
{
    if (!m_scoutUnit || m_scoutUnit->health <= 0) { return; }

    float scoutHP = m_scoutUnit->health + m_scoutUnit->shield;

    // get the enemy base location, if we have one
    const BaseLocation * enemyBaseLocation = m_bot.Bases().getPlayerStartingBaseLocation(Players::Enemy);

    int scoutDistanceThreshold = 20;
	
    // if we know where the enemy region is and where our scout is
	if (enemyBaseLocation)
	{
		//std::cout << "enemyBaseLocation is not null" << std::endl;
		int scoutDistanceToEnemy = m_bot.Map().getGroundDistance(m_scoutUnit->pos, enemyBaseLocation->getPosition());
		bool scoutInRangeOfenemy = enemyBaseLocation->containsPosition(m_scoutUnit->pos);
		if (m_arriveEnemyStart) {
			auto enemyThird = m_bot.Bases().getPlayerThirdLocation(Players::Enemy);
			if (enemyThird)
			{
				auto enemyThirdPos = enemyThird->getDepotPosition();
				Micro::SmartMove(m_scoutUnit, enemyThirdPos, m_bot);

				if (fabs(m_scoutUnit->pos.x - enemyThirdPos.x) + fabs(m_scoutUnit->pos.y - enemyThirdPos.y) < 10) {
					m_bot.Workers().setWaitWorker(m_scoutUnit);
					m_scoutUnit = nullptr;
					return;
				}
			}
		}
		
		// we only care if the scout is under attack within the enemy region
		// this ignores if their scout worker attacks it on the way to their base
		if (scoutHP < m_previousScoutHP)
		{
			m_scoutUnderAttack = true;
		}

		if (scoutHP == m_previousScoutHP && !enemyWorkerInRadiusOf(m_scoutUnit->pos))
		{
			m_scoutUnderAttack = false;
		}
		
		// if the scout is in the enemy region
		if (scoutInRangeOfenemy)
		{
			// get the closest enemy worker
			const sc2::Unit * closestEnemyWorkerUnit = closestEnemyWorkerTo(m_scoutUnit->pos);

			// if the worker scout is not under attack
			if (!m_scoutUnderAttack)
			{
				// don't harass
				//if (m_bot.Config().ScoutHarassEnemy && closestEnemyWorkerUnit &&
				//	Util::Dist(m_scoutUnit->pos, closestEnemyWorkerUnit->pos) < 10)
				//{
				//	m_scoutStatus = "Harass enemy worker";
				//	Micro::SmartAttackUnit(m_scoutUnit, closestEnemyWorkerUnit, m_bot);
				//}
				// otherwise keep moving to the enemy base location
				m_scoutStatus = "Moving to enemy base location";
				if (!m_arriveEnemyStart) {
					Micro::SmartMove(m_scoutUnit, enemyBaseLocation->getPosition(), m_bot);
				}
				if (Util::Dist(m_scoutUnit->pos, enemyBaseLocation->getPosition()) < 10) {
					m_arriveEnemyStart = true;
				}
			}
			// if the worker scout is under attack
			else
			{
				m_scoutStatus = "Under attack inside, fleeing";
				Micro::SmartMove(m_scoutUnit, getFleePosition(), m_bot);

			}
		}
		// if the scout is not in the enemy region
		else if (m_scoutUnderAttack)
		{
			m_scoutStatus = "Under attack outside, fleeing";

			Micro::SmartMove(m_scoutUnit, getFleePosition(), m_bot);
		}
		else
		{
			m_scoutStatus = "Enemy region known, going there";

			// move to the enemy region
			if(!m_arriveEnemyStart)
				Micro::SmartMove(m_scoutUnit, enemyBaseLocation->getPosition(), m_bot);
		}

	}

	// for each start location in the level
	if (!enemyBaseLocation)
	{
		m_scoutStatus = "Enemy base unknown, exploring";
		//std::cout << m_scoutStatus << std::endl;
		for (const BaseLocation * startLocation : m_bot.Bases().getStartingBaseLocations())
		{
			// if we haven't explored it yet then scout it out
			// TODO: this is where we could change the order of the base scouting, since right now it's iterator order
			if (!m_bot.Map().isExplored(startLocation->getPosition()))
			{
				Micro::SmartMove(m_scoutUnit, startLocation->getPosition(), m_bot);
				return;
			}
		}
	}
	
    m_previousScoutHP = scoutHP;
}

const sc2::Unit * ScoutManager::closestEnemyWorkerTo(const sc2::Point2D & pos) const
{
    if (!m_scoutUnit) { return nullptr; }

    UnitTag enemyWorkerTag = 0;
    float minDist = std::numeric_limits<float>::max();

    // for each enemy worker
    for (auto & unit : m_bot.UnitInfo().getUnits(Players::Enemy))
    {
        if (Util::IsWorker(unit))
        {
            float dist = Util::Dist(unit->pos, m_scoutUnit->pos);

            if (dist < minDist)
            {
                minDist = dist;
                enemyWorkerTag = unit->tag;
            }
        }
    }

    return m_bot.GetUnit(enemyWorkerTag);
}
bool ScoutManager::enemyWorkerInRadiusOf(const sc2::Point2D & pos) const
{
    for (auto & unit : m_bot.UnitInfo().getUnits(Players::Enemy))
    {
        if (Util::IsWorker(unit) && Util::Dist(unit->pos, pos) < 10)
        {
            return true;
        }
    }

    return false;
}

const sc2::Unit * ScoutManager::getScout() {
	return m_scoutUnit;
}

sc2::Point2D ScoutManager::getFleePosition() const
{
    // TODO: make this follow the perimeter of the enemy base again, but for now just use home base as flee direction
    return m_bot.GetStartLocation();
}