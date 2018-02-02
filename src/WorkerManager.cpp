#include "WorkerManager.h"
#include "Micro.h"
#include "CCBot.h"
#include "Util.h"
#include "Building.h"

WorkerManager::WorkerManager(CCBot & bot)
    : m_bot         (bot)
    , m_workerData  (bot)
{
    m_previousClosestWorker = nullptr;
}

void WorkerManager::onStart()
{

}

void WorkerManager::onFrame()
{
    m_workerData.updateAllWorkerData();
    handleGasWorkers();
    handleIdleWorkers();

    drawResourceDebugInfo();
    drawWorkerInformation();

    m_workerData.drawDepotDebugInfo();

    handleRepairWorkers();
}

void WorkerManager::setRepairWorker(const sc2::Unit * worker, const sc2::Unit * unitToRepair)
{
    m_workerData.setWorkerJob(worker, WorkerJobs::Repair, unitToRepair);
}

void WorkerManager::stopRepairing(const sc2::Unit * worker)
{
    m_workerData.setWorkerJob(worker, WorkerJobs::Idle);
}

void WorkerManager::handleGasWorkers()
{
    // for each unit we have
    for (auto unit : m_bot.UnitInfo().getUnits(Players::Self))
    {
        // if that unit is a refinery
        if (Util::IsRefinery(unit) && Util::IsCompleted(unit))
        {
            // get the number of workers currently assigned to it
            int numAssigned = m_workerData.getNumAssignedWorkers(unit);

            // if it's less than we want it to be, fill 'er up
            for (int i=0; i<(m_bot.Config().WorkersPerRefinery-numAssigned); ++i)
            {
                auto gasWorker = getGasWorker(unit);
                if (gasWorker)
                {
                    m_workerData.setWorkerJob(gasWorker, WorkerJobs::Gas, unit);
                }
            }
        }
    }
}

void WorkerManager::handleIdleWorkers()
{
    // for each of our workers
    for (auto worker : m_workerData.getWorkers())
    {
        if (!worker) { continue; }

        // if it's a scout, don't handle it here
        if (m_workerData.getWorkerJob(worker) == WorkerJobs::Scout)
        {
            continue;
        }
		if (m_workerData.getWorkerJob(worker) == WorkerJobs::Wait)
		{
			continue;
		}

        // if it is idle
        if (Util::IsIdle(worker) || m_workerData.getWorkerJob(worker) == WorkerJobs::Idle)
        {
            setMineralWorker(worker);
        }
    }
}

void WorkerManager::handleRepairWorkers()
{
    // TODO
}

const sc2::Unit * WorkerManager::getClosestBuildableWorkerTo(const sc2::Point2D & pos) const
{
	const sc2::Unit * closestWorker = nullptr;
	float closestDist = std::numeric_limits<float>::max();

	bool inThird = false;
	if (m_bot.Bases().getPlayerThirdLocation(Players::Enemy) &&
		Util::PlanerDist(pos, m_bot.Bases().getPlayerThirdLocation(Players::Enemy)->getDepotPosition()) < 10)
	{
		inThird = true;
	}
	// for each of our workers
	for (auto worker : m_workerData.getWorkers())
	{
		if (!worker) { continue; }

		// if it is a mineral worker
		if (m_workerData.getWorkerJob(worker) == WorkerJobs::Minerals || m_workerData.getWorkerJob(worker) == WorkerJobs::Wait || m_workerData.getWorkerJob(worker) == WorkerJobs::Idle)
		{
			float dist = Util::PlanerDist(worker->pos, pos);

			if (!closestWorker || dist < closestDist)
			{
				closestWorker = worker;
				closestDist = dist;
			}
		}
	}

	return closestWorker;
}


// set a worker to mine minerals
void WorkerManager::setMineralWorker(const sc2::Unit * unit)
{
    // check if there is a mineral available to send the worker to
    auto depot = getClosestDepot(unit);
	if (!depot) {
		return;
	}
    // if there is a valid mineral
    if (Util::Dist(depot->pos,unit->pos) < 4)
    {
        // update m_workerData with the new job
        m_workerData.setWorkerJob(unit, WorkerJobs::Minerals, depot);
	}
	else {
		Micro::SmartMove(unit, depot->pos, m_bot);
	}

}

const sc2::Unit * WorkerManager::getClosestDepot(const sc2::Unit * worker) const
{
    const sc2::Unit * closestDepot = nullptr;
    double closestDistance = std::numeric_limits<double>::max();

    for (auto & unit : m_bot.UnitInfo().getUnits(Players::Self))
    {
        if (!unit) { continue; }

        if (Util::IsTownHall(unit) && Util::IsCompleted(unit) && unit->assigned_harvesters < 16)
        {
            double distance = Util::DistSq(unit->pos, worker->pos);
            if (!closestDepot || distance < closestDistance)
            {
                closestDepot = unit;
                closestDistance = distance;
            }
        }
    }

    return closestDepot;
}


// other managers that need workers call this when they're done with a unit
void WorkerManager::finishedWithWorker(const sc2::Unit * unit)
{
	m_workerData.setWorkerJob(unit, WorkerJobs::Idle);
}

const sc2::Unit * WorkerManager::getGasWorker(const sc2::Unit * refinery) const
{
    return getClosestBuildableWorkerTo(refinery->pos);
}

void WorkerManager::setBuildingWorker(const sc2::Unit * worker, Building & b)
{
    m_workerData.setWorkerJob(worker, WorkerJobs::Build, b.buildingUnit);
}

void WorkerManager::setWaitWorker(const sc2::Unit * worker)
{
	m_workerData.setWorkerJob(worker, WorkerJobs::Wait);
}

// gets a builder for BuildingManager to use
// if setJobAsBuilder is true (default), it will be flagged as a builder unit
// set 'setJobAsBuilder' to false if we just want to see which worker will build a building
const sc2::Unit * WorkerManager::getBuilder(Building & b, bool setJobAsBuilder) const
{
    const sc2::Unit * builderWorker = getClosestBuildableWorkerTo(Util::GetPosition(b.finalPosition));

    // if the worker exists (one may not have been found in rare cases)
    if (builderWorker && setJobAsBuilder)
    {
        m_workerData.setWorkerJob(builderWorker, WorkerJobs::Build, b.builderUnit);
    }

    return builderWorker;
}

// sets a worker as a scout
void WorkerManager::setScoutWorker(const sc2::Unit * workerTag)
{
    m_workerData.setWorkerJob(workerTag, WorkerJobs::Scout);
}

void WorkerManager::setCombatWorker(const sc2::Unit * workerTag)
{
    m_workerData.setWorkerJob(workerTag, WorkerJobs::Combat);
}

void WorkerManager::drawResourceDebugInfo()
{
    if (!m_bot.Config().DrawResourceInfo)
    {
        return;
    }

    for (auto & worker : m_workerData.getWorkers())
    {
        if (!worker) { continue; }

        auto depot = m_workerData.getWorkerDepot(worker);
        if (depot)
        {
            m_bot.Map().drawLine(worker->pos, depot->pos);
        }
    }
}

void WorkerManager::drawWorkerInformation()
{
    if (!m_bot.Config().DrawWorkerInfo)
    {
        return;
    }

    std::stringstream ss;
    ss << "Workers: " << m_workerData.getWorkers().size() << "\n";

    int yspace = 0;

    for (auto & workerTag : m_workerData.getWorkers())
    {
        ss << m_workerData.getJobCode(workerTag) << " " << workerTag << "\n";
    }

    m_bot.Map().drawTextScreen(sc2::Point2D(0.75f, 0.2f), ss.str());
}

bool WorkerManager::isFree(const sc2::Unit * worker) const
{
    return m_workerData.getWorkerJob(worker) == WorkerJobs::Minerals ||
		m_workerData.getWorkerJob(worker) == WorkerJobs::Idle;
}

bool WorkerManager::isWorkerScout(const sc2::Unit * worker) const
{
    return (m_workerData.getWorkerJob(worker) == WorkerJobs::Scout);
}

bool WorkerManager::isBuilder(const sc2::Unit * worker) const
{
    return (m_workerData.getWorkerJob(worker) == WorkerJobs::Build);
}

int WorkerManager::getNumMineralWorkers() const
{
    return m_workerData.getWorkerJobCount(WorkerJobs::Minerals);
}

int WorkerManager::getNumGasWorkers() const
{
    return m_workerData.getWorkerJobCount(WorkerJobs::Gas);
}

int WorkerManager::getNumWorkers(int job) const
{
	return m_workerData.getWorkerJobCount(job);
}
