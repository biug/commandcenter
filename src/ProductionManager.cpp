#include "ProductionManager.h"
#include "Util.h"
#include "CCBot.h"
#include "Micro.h"

ProductionManager::ProductionManager(CCBot & bot)
    : m_bot             (bot)
    , m_buildingManager (bot)
    , m_queue           (bot)
	, warpGate (0)
	, used(0)
{

}

void ProductionManager::setBuildOrder(const BuildOrder & buildOrder)
{
    m_queue.clearAll();

    for (size_t i(0); i<buildOrder.size(); ++i)
    {
        m_queue.queueAsLowestPriority(buildOrder[i], true);
    }
}


void ProductionManager::onStart()
{
    m_buildingManager.onStart();
    setBuildOrder(m_bot.Strategy().getOpeningBookBuildOrder());
}

void ProductionManager::onFrame()
{
    // check the _queue for stuff we can build
    manageBuildOrderQueue();

    // TODO: if nothing is currently building, get a new goal from the strategy manager
    // TODO: detect if there's a build order deadlock once per second
    // TODO: triggers for game things like cloaked units etc

    m_buildingManager.onFrame();
    drawProductionInformation();
}

// on unit destroy
void ProductionManager::onUnitDestroy(const sc2::Unit * unit)
{
    // TODO: might have to re-do build order if a vital unit died
}

void ProductionManager::manageBuildOrderQueue()
{
    // if there is nothing in the queue, oh well
    if (m_queue.isEmpty())
    {
        return;
    }

    // the current item to be used
    BuildOrderItem & currentItem = m_queue.getHighestPriorityItem();

    // while there is still something left in the queue
    while (!m_queue.isEmpty())
    {
		if (currentItem.type.isUnit())
		{
			
			// this is the unit which can produce the currentItem
			
			
				const sc2::Unit * producer = getProducer(currentItem.type.getUnitType());
			
			// check to see if we can make it right now
			bool canMake = canMakeNow(producer, currentItem.type.getUnitType());

			// TODO: if it's a building and we can't make it yet, predict the worker movement to the location

			// if we can make the current item
			if (producer && canMake)
			{
				// create it and remove it from the _queue
				create(producer, currentItem);
				m_queue.removeCurrentHighestPriorityItem();

				// don't actually loop around in here
				break;
			}
			else
			{
				
				// so break out
				break;
			}
		}
		else if (currentItem.type.isUpgrade())
		{
			// this is the unit which can produce the currentItem
			const sc2::Unit * producer = getProducer(currentItem.type.getUpgradeType());

			// check to see if we can make it right now
			bool canMake = canMakeNow(producer, currentItem.type.getUpgradeType());
			// if we can make the current item
			if (producer && canMake)
			{
				// create it and remove it from the _queue
				create(producer, currentItem);
				m_queue.removeCurrentHighestPriorityItem();

				// don't actually loop around in here
				break;
			}
			else
			{
			
				// so break out
				break;
			}
		}
    }
}

const sc2::Unit * ProductionManager::getProducer(const BuildType & type, sc2::Point2D closestTo)
{
    // get all the types of units that can build this type
    auto & producerTypes = m_bot.Data(type).whatBuilds;
	warpGate = m_bot.UnitInfo().getUnitTypeCount(Players::Self, sc2::UNIT_TYPEID::PROTOSS_WARPGATE, true);
	if (type.getUnitTypeID() == sc2::UNIT_TYPEID::PROTOSS_GATEWAY&&m_bot.UnitInfo().getUnitTypeCount(Players::Self, sc2::UNIT_TYPEID::PROTOSS_PYLON, false) == 3) {
		closestTo = sc2::Point2D(10, 15) + sc2::Point2D(m_bot.Bases().getPlayerStartingBaseLocation(Players::Enemy)->getPosition().x / 2 + m_bot.Bases().getPlayerStartingBaseLocation(Players::Self)->getPosition().x / 2, m_bot.Bases().getPlayerStartingBaseLocation(Players::Enemy)->getPosition().y / 2 + m_bot.Bases().getPlayerStartingBaseLocation(Players::Self)->getPosition().y / 2);
		
	}
    // make a set of all candidate producers
    std::vector<const sc2::Unit *> candidateProducers;

    for (auto unit : m_bot.UnitInfo().getUnits(Players::Self))
    {
        // reasons a unit can not train the desired type
        if (std::find(producerTypes.begin(), producerTypes.end(), unit->unit_type) == producerTypes.end()) { continue; }
        if (unit->build_progress < 1.0f) { continue; }
        if (m_bot.Data(unit->unit_type).isBuilding && unit->orders.size() > 0) { continue; }
        if (unit->is_flying) { continue; }
		if (unit->unit_type == sc2::UNIT_TYPEID::PROTOSS_GATEWAY&&m_bot.warpgateComplete()) { continue; }
        // TODO: if unit is not powered continue
        // TODO: if the type is an addon, some special cases
        // TODO: if the type requires an addon and the producer doesn't have one

        // if we haven't cut it, add it to the set of candidates
        candidateProducers.push_back(unit);
    }
	
    return getClosestUnitToPosition(candidateProducers, closestTo);
}

const sc2::Unit * ProductionManager::getClosestUnitToPosition(const std::vector<const sc2::Unit *> & units, sc2::Point2D closestTo)
{
    if (units.size() == 0)
    {
        return 0;
    }
	if (warpGate>0&&units[0]->unit_type == sc2::UNIT_TYPEID::PROTOSS_WARPGATE) {
		int t = used%warpGate;
		used++;
		return units[t];

	}
    // if we don't care where the unit is return the first one we have
    if (closestTo.x == 0 && closestTo.y == 0)
    {
		
        return units[0];
    }

    const sc2::Unit * closestUnit = nullptr;
    double minDist = std::numeric_limits<double>::max();

    for (auto & unit : units)
    {
        double distance = Util::Dist(unit->pos, closestTo);
        if (!closestUnit || distance < minDist)
        {
            closestUnit = unit;
            minDist = distance;
        }
    }

    return closestUnit;
}

const sc2::Unit * ProductionManager::pylonClosestToEnemy()
{
	const sc2::Unit * closest = nullptr;
	float closestDist = std::numeric_limits<float>::max();
	
	for (auto unit : m_bot.UnitInfo().getUnits(Players::Self))
	{
		BOT_ASSERT(unit, "null unit");
		if (unit->unit_type == sc2::UNIT_TYPEID::PROTOSS_PYLON) {
			// the distance to the order position
			int dist = m_bot.Map().getGroundDistance(unit->pos, m_bot.Bases().getPlayerStartingBaseLocation(Players::Enemy)->getPosition());

			if (dist != -1 && (!closest || dist < closestDist))
			{
				closest = unit;
				closestDist = (float)dist;
			}
		}
	}

	return closest;
}
// this function will check to see if all preconditions are met and then create a unit
void ProductionManager::create(const sc2::Unit * producer, BuildOrderItem & item)
{
    if (!producer)
    {
        return;
    }

    // if we're dealing with a building
    // TODO: deal with morphed buildings & addons
    if (item.type.isBuilding(m_bot))
    {
        // send the building task to the building manager
        m_buildingManager.addBuildingTask(item.type.getUnitType(), m_bot.GetStartLocation());
    }
    // if we're dealing with a non-building unit
    else if (item.type.isUnit())
    {
		if (producer->unit_type == sc2::UNIT_TYPEID::PROTOSS_WARPGATE) {
			
					Micro::SmartWarp(producer, item.type.getUnitType(), pylonClosestToEnemy()->pos + sc2::Point2D(rand()%5+1,rand()%5+1) - sc2::Point2D(rand() % 5, rand() % 5), m_bot);
			
		}
		else {
			Micro::SmartTrain(producer, item.type.getUnitType(), m_bot);
		}
    }
    else if (item.type.isUpgrade())
    {
        Micro::SmartAbility(producer, m_bot.Data(item.type.getUpgradeType()).buildAbility, m_bot);
    }
}

bool ProductionManager::canMakeNow(const sc2::Unit * producer, const BuildType & type)
{
    if (!producer || !meetsReservedResources(type))
    {
        return false;
    }

    sc2::AvailableAbilities available_abilities = m_bot.Query()->GetAbilitiesForUnit(producer);

    // quick check if the unit can't do anything it certainly can't build the thing we want
    if (available_abilities.abilities.empty())
    {
        return false;
    }
    else
    {
        // check to see if one of the unit's available abilities matches the build ability type
        sc2::AbilityID buildTypeAbility = m_bot.Data(type).buildAbility;
        for (const sc2::AvailableAbility & available_ability : available_abilities.abilities)
        {
            if (available_ability.ability_id == buildTypeAbility)
            {
                return true;
            }
        }
		buildTypeAbility = m_bot.Data(type).warpAbility;
		for (const sc2::AvailableAbility & available_ability : available_abilities.abilities)
		{
			if (available_ability.ability_id == buildTypeAbility)
			{
				return true;
			}
		}
    }

    return false;
}

bool ProductionManager::detectBuildOrderDeadlock()
{
    // TODO: detect build order deadlocks here
    return false;
}

int ProductionManager::getFreeMinerals()
{
    return m_bot.Observation()->GetMinerals() - m_buildingManager.getReservedMinerals();
}

int ProductionManager::getFreeGas()
{
    return m_bot.Observation()->GetVespene() - m_buildingManager.getReservedGas();
}

// return whether or not we meet resources, including building reserves
bool ProductionManager::meetsReservedResources(const BuildType & type)
{
    // return whether or not we meet the resources
    return (m_bot.Data(type).mineralCost <= getFreeMinerals()) && (m_bot.Data(type).gasCost <= getFreeGas());
}

void ProductionManager::drawProductionInformation()
{
    if (!m_bot.Config().DrawProductionInfo)
    {
        return;
    }

    std::stringstream ss;
    ss << "Production Information\n\n";

    for (auto & unit : m_bot.UnitInfo().getUnits(Players::Self))
    {
        if (unit->build_progress < 1.0f)
        {
            //ss << sc2::UnitTypeToName(unit.unit_type) << " " << unit.build_progress << "\n";
        }
    }

    ss << m_queue.getQueueInformation();

    m_bot.Map().drawTextScreen(sc2::Point2D(0.01f, 0.01f), ss.str(), sc2::Colors::Yellow);
}
