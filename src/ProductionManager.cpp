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
	fixBuildOrderDeadlock();
    manageBuildOrderQueue();
	
    // TODO: if nothing is currently building, get a new goal from the strategy manager
    // TODO: detect if there's a build order deadlock once per second
    // TODO: triggers for game things like cloaked units etc
	
    m_buildingManager.onFrame();
	keepTrainWorker();

	// chrono boost target
	if (m_bot.State().m_chronoTarget != sc2::UNIT_TYPEID::INVALID)
	{
		const sc2::Unit * nexus = nullptr;
		const sc2::Unit * target = nullptr;
		for (auto b : m_bot.UnitInfo().getUnits(Players::Self))
		{
			if (b->unit_type.ToType() == sc2::UNIT_TYPEID::PROTOSS_NEXUS)
			{
				bool canBoost = false;
				for (const auto & ability : m_bot.Query()->GetAbilitiesForUnit(b).abilities)
				{
					if (ability.ability_id.ToType() == sc2::ABILITY_ID::EFFECT_CHRONOBOOST)
					{
						canBoost = true;
					}
				}
				if (canBoost)
				{
					nexus = b;
				}
			}
			else if (b->unit_type == m_bot.State().m_chronoTarget && b->build_progress == 1.0f)
			{
				if (std::find(b->buffs.begin(), b->buffs.end(), sc2::BUFF_ID::TIMEWARPPRODUCTION) == b->buffs.end())
				{
					target = b;
				}
			}
		}
		if (nexus && target)
		{
			Micro::SmartAbility(nexus, sc2::ABILITY_ID::EFFECT_CHRONOBOOST, target, m_bot);
		}
	}
	
	
	trainWarpGate();
    drawProductionInformation();
}
void ProductionManager::trainWarpGate() {
	// train warp gate
	if (m_bot.State().m_rschWarpGate)
	{
		for (auto b : m_bot.UnitInfo().getUnits(Players::Self))
		{
			if (b->unit_type == sc2::UNIT_TYPEID::PROTOSS_GATEWAY)
			{
				Micro::SmartAbility(b, sc2::ABILITY_ID::MORPH_WARPGATE, m_bot);
			}
		}
	}
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
	
	if (m_bot.Strategy().ShouldExpandNow()
		// Don't queue more bases than you have minerals for.
		)
	{
		switch (m_bot.GetPlayerRace(Players::Self)) {
		case sc2::Race::Protoss: m_queue.queueAsHighestPriority(MacroAct(sc2::UNIT_TYPEID::PROTOSS_NEXUS), true);
		case sc2::Race::Terran: m_queue.queueAsHighestPriority(MacroAct(sc2::UNIT_TYPEID::TERRAN_COMMANDCENTER), true);
		case sc2::Race::Zerg: m_queue.queueAsHighestPriority(MacroAct(sc2::UNIT_TYPEID::ZERG_HATCHERY), true);
		}
	
	}
	
	if (detectSupplyDeadlock() && m_bot.Observation()->GetFoodUsed() > 25)
	{
		// we need build supply depot
		switch (m_bot.GetPlayerRace(Players::Self)) 
		{
			case sc2::Race::Protoss:
				m_queue.queueAsHighestPriority(MacroAct(sc2::UNIT_TYPEID::PROTOSS_PYLON), true);
				break;
			case sc2::Race::Terran:
				m_queue.queueAsHighestPriority(MacroAct(sc2::UNIT_TYPEID::TERRAN_SUPPLYDEPOT), true);
				break;
			case sc2::Race::Zerg:
				m_queue.queueAsHighestPriority(MacroAct(sc2::UNIT_TYPEID::ZERG_OVERLORD), true);
				break;
		}
		
	}
	BuildOrderItem currentItem = m_queue.getHighestPriorityItem();
	if (m_bot.GetPlayerRace(Players::Self) == sc2::Race::Protoss && m_bot.UnitInfo().getUnitTypeCount(Players::Self, sc2::UNIT_TYPEID::PROTOSS_PYLON) == 0 && currentItem.type.isBuilding(m_bot) && currentItem.type.getUnitType() != sc2::UNIT_TYPEID::PROTOSS_PYLON){
		return;
	}
	
	if (currentItem.type.isUnit())
	{
		// this is the unit which can produce the currentItem
		const sc2::Unit * producer = getProducer(currentItem.type.getUnitType());

		
		// if we're waiting for a warpgate, we should shutdown our training
		if (producer && producer->unit_type.ToType() == sc2::UNIT_TYPEID::PROTOSS_GATEWAY && m_bot.State().m_waitWarpGate)
		{
			return;
		}
			
		// check to see if we can make it right now
		bool canMake = canMakeNow(producer, currentItem.type.getUnitType());
		
		
		// if we can make the current item
		if (producer && canMake)
		{
			// create it and remove it from the _queue
			create(producer, currentItem);
			m_queue.removeCurrentHighestPriorityItem();
			
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
		}
	}
	else if (currentItem.type.isCommand())
	{
		auto command = currentItem.type.getCommandType();
		switch (command.getType())
		{
		case MacroCommandType::Scout:m_bot.State().m_scout = true; break;
		case MacroCommandType::KeepTrainWorker:
			m_bot.State().m_keepTrainWorker = true;
			m_bot.State().m_numKeepTrainWorker = command.getAmount();
			break;
		case MacroCommandType::StartAttack:
			m_bot.State().m_startAttack = true;
			m_bot.State().m_rallyAtPylon = false;
		case MacroCommandType::WaitWarpGate: m_bot.State().m_waitWarpGate = true; break;
		case MacroCommandType::WaitBlink: m_bot.State().m_waitBlink = true; break;
		case MacroCommandType::RallyAtPylon: m_bot.State().m_rallyAtPylon = true; break;
		case MacroCommandType::StartBlink: m_bot.State().m_startBlink = true; break;
		case MacroCommandType::ChronoBoost: m_bot.State().m_chronoTarget = command.getTarget(); break;
		case MacroCommandType::StartGas:m_bot.Config().WorkersPerRefinery = 3; break;
		case MacroCommandType::GoOnPatrol:m_bot.State().m_patrol = true; break;
		}
		m_queue.removeCurrentHighestPriorityItem();
	}
}

void ProductionManager::fixBuildOrderDeadlock()
{
	if (m_queue.isEmpty()) { return; }
	BuildOrderItem & currentItem = m_queue.getHighestPriorityItem();
	if (currentItem.type.isCommand()) {
		return;
	}
	// check to see if we have the prerequisites for the topmost item
	if (currentItem.type.isUnit()) {
		bool hasRequired = m_bot.Data(currentItem.type.getUnitType()).requiredUnits.empty();
		for (auto & required : m_bot.Data(currentItem.type.getUnitType()).requiredUnits)
		{
			if (m_bot.UnitInfo().getUnitTypeCount(Players::Self, required, false) > 0 || m_buildingManager.isBeingBuilt(required))
			{
				hasRequired = true;
				break;
			}
		}

		if (!hasRequired)
		{
			//std::cout << currentItem.type.getName() << " needs " << m_bot.Data(currentItem.type.getUnitType()).requiredUnits[0].getName() << "\n";
			m_queue.queueAsHighestPriority(MacroAct(m_bot.Data(currentItem.type.getUnitType()).requiredUnits[0].ToType()), true);
			fixBuildOrderDeadlock();
			return;
		}

		// build the producer of the unit if we don't have one
		bool hasProducer = m_bot.Data(currentItem.type.getUnitType()).whatBuilds.empty();
		for (auto & producer : m_bot.Data(currentItem.type.getUnitType()).whatBuilds)
		{
			if (m_bot.UnitInfo().getUnitTypeCount(Players::Self, producer, false) > 0 || m_buildingManager.isBeingBuilt(producer))
			{
				hasProducer = true;
				break;
			}
		}

		if (!hasProducer)
		{
			// for zerg : no need to add larva to queue 
			if (m_bot.Data(currentItem.type.getUnitType()).whatBuilds[0].ToType() == sc2::UNIT_TYPEID::ZERG_LARVA) {
				if (m_bot.UnitInfo().getUnitTypeCount(Players::Self, sc2::UNIT_TYPEID::ZERG_LARVA, false) == 0) {
					return;
				}
			}
			else {
				m_queue.queueAsHighestPriority(MacroAct(m_bot.Data(currentItem.type.getUnitType()).whatBuilds[0].ToType()), true);
				fixBuildOrderDeadlock();
			}

		}

		// build a refinery if we don't have one and the thing costs gas
		auto refinery = Util::GetRefinery(m_bot.GetPlayerRace(Players::Self));
		if (m_bot.Data(currentItem.type.getUnitType()).gasCost > 0 && m_bot.UnitInfo().getUnitTypeCount(Players::Self, refinery, false) == 0)
		{
			m_queue.queueAsHighestPriority(MacroAct(refinery.ToType()), true);
		}

		// build supply if we need some
		auto supplyProvider = Util::GetSupplyProvider(m_bot.GetPlayerRace(Players::Self));
		if (m_bot.Data(currentItem.type.getUnitType()).supplyCost > (m_bot.GetMaxSupply() - m_bot.GetCurrentSupply()) && !m_buildingManager.isBeingBuilt(supplyProvider))
		{
			m_queue.queueAsHighestPriority(MacroAct(supplyProvider.ToType()), true);
		}
	}
}

const sc2::Unit * ProductionManager::getProducer(const MacroAct & type, sc2::Point2D closestTo)
{
    // get all the types of units that can build this type
	BuildType buildType = type.isUnit() ? BuildType(type.getUnitType()) : BuildType(type.getUpgradeType());
    auto & producerTypes = m_bot.Data(buildType).whatBuilds;
	warpGate = m_bot.UnitInfo().getUnitTypeCount(Players::Self, sc2::UNIT_TYPEID::PROTOSS_WARPGATE, true);
	auto macroLocation = type.getMacroLocation(m_bot);
	if (macroLocation != sc2::Point2DI(0, 0))
	{
		std::cout << "macrolocation found for " << buildType.getName() << std::endl;
		closestTo = Util::GetPosition(macroLocation);
	}
    // make a set of all candidate producers
    std::vector<const sc2::Unit *> candidateProducers;

    for (auto unit : m_bot.UnitInfo().getUnits(Players::Self))
    {
        // reasons a unit can not train the desired type
        if (std::find(producerTypes.begin(), producerTypes.end(), unit->unit_type) == producerTypes.end()) { continue; }
        if (unit->build_progress < 1.0f) { continue; }
        if (m_bot.Data(unit->unit_type).isBuilding && unit->orders.size() > 0 && !Util::hasReactor(unit,m_bot)) { continue; }
        if (unit->is_flying) { continue; }
		if (Util::hasReactor(unit, m_bot) && unit->orders.size() > 1) { continue; }
		
		
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
		
		if (isMorphedBuilding(item.type.getUnitType()) ){
			Micro::SmartMorph(producer, item.type.getUnitType(), m_bot);
		}
		else if (m_bot.Data(item.type.getUnitType()).isAddon){
			Micro::SmartAddon(producer, item.type.getUnitType(), m_bot);
		}
		else {
			// send the building task to the building manager
			auto macroLocation = item.type.getMacroLocation(m_bot);
			m_buildingManager.addBuildingTask(item.type.getUnitType(),
				macroLocation != sc2::Point2DI(0, 0) ? macroLocation : Util::GetTilePosition(m_bot.GetStartLocation()));
		}
    }
    // if we're dealing with a non-building unit
    else if (item.type.isUnit())
    {
		if (producer->unit_type == sc2::UNIT_TYPEID::PROTOSS_WARPGATE)
		{
			auto closestPylon = Util::getClosestPylon(m_bot);
			if (closestPylon)
			{
				Micro::SmartWarp(producer, item.type.getUnitType(), closestPylon->pos + sc2::Point2D((float)rand() / RAND_MAX * 5.0f, (float)rand() / RAND_MAX * 5.0f), m_bot);
			}
			
		}
		else
		{
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
void ProductionManager::keepTrainWorker() {
	if (m_bot.State().m_keepTrainWorker) {

		for (auto b : m_bot.UnitInfo().getUnits(Players::Self)) {
			switch (m_bot.GetPlayerRace(Players::Self))
			{
			case sc2::Race::Protoss:
				if (m_bot.Data(b->unit_type).isTownHall) {
					if (b->orders.size() < 1 || (b->orders[0].progress > 0.7f && b->orders.size() <2)) {
						Micro::SmartTrain(b, sc2::UNIT_TYPEID::PROTOSS_PROBE, m_bot);
					}
				}
			case sc2::Race::Terran:
				if (m_bot.Data(b->unit_type).isTownHall) {
					if (b->orders.size() < 1 || (b->orders[0].progress > 0.7f && b->orders.size() <2)) {
						Micro::SmartTrain(b, sc2::UNIT_TYPEID::TERRAN_SCV, m_bot);
					}
				}
				break;
			case sc2::Race::Zerg:
				if (m_bot.Data(b->unit_type).isTownHall) {
					if (!detectSupplyDeadlock()) {
						Micro::SmartTrain(b, sc2::UNIT_TYPEID::ZERG_DRONE, m_bot);
					}
				}
			default:
				break;
			}
		}
		if (m_bot.UnitInfo().getUnitTypeCount(Players::Self, sc2::UNIT_TYPEID::PROTOSS_PROBE) >= m_bot.State().m_numKeepTrainWorker - 2) {
			m_bot.State().m_keepTrainWorker = false;
		}
		if (m_bot.UnitInfo().getUnitTypeCount(Players::Self, sc2::UNIT_TYPEID::ZERG_DRONE) + m_bot.UnitInfo().getUnitTypeCount(Players::Self, sc2::UNIT_TYPEID::ZERG_DRONEBURROWED) >= m_bot.State().m_numKeepTrainWorker - 2) {
			m_bot.State().m_keepTrainWorker = false;
		}
		if (m_bot.UnitInfo().getUnitTypeCount(Players::Self, sc2::UNIT_TYPEID::TERRAN_SCV) >= m_bot.State().m_numKeepTrainWorker - 2) {
			m_bot.State().m_keepTrainWorker = false;
		}
	}

}
bool ProductionManager::isMorphedBuilding(const sc2::UNIT_TYPEID t) {
	switch (t)
	{
	
	case sc2::UNIT_TYPEID::TERRAN_ORBITALCOMMAND:
		return true;
	
	case sc2::UNIT_TYPEID::ZERG_LAIR:
		return true;
	
	case sc2::UNIT_TYPEID::ZERG_HIVE:
		return true;

	case sc2::UNIT_TYPEID::ZERG_GREATERSPIRE:
		return true;
	
	case sc2::UNIT_TYPEID::TERRAN_PLANETARYFORTRESS:
		return true;

	default:
		return false;
		}
}
bool ProductionManager::detectSupplyDeadlock()
{
	
	auto race = m_bot.GetPlayerRace(Players::Self);
	auto supply = m_bot.Observation()->GetFoodCap();
	if (m_queue.isEmpty() || supply >= 200) return false;

	// If supply is being built now, there's no block. Return right away.
	// Terran and protoss calculation:
	if (m_buildingManager.isBeingBuilt(Util::GetSupplyProvider(race)))
	{
		return false;
	}
	auto supplyAvailable = supply - m_bot.Observation()->GetFoodUsed();
	/*
	switch (race)
	{
	case sc2::Race::Terran:
		supplyAvailable = -m_bot.Observation()->GetFoodUsed();
		for (auto & unit : m_bot.UnitInfo().getUnits(Players::Self))
		{
			if (m_bot.Data(unit->unit_type).isSupplyDepot)
			{
				if (unit->build_progress < 1.0f)
				{
					return false;
				}
				supplyAvailable += 8;
			}
			else if (m_bot.Data(unit->unit_type).isTownHall && unit->build_progress == 1.0f)
			{
				supplyAvailable += 15;
			}
		}
		break;
	case sc2::Race::Zerg:
		break;
	case sc2::Race::Protoss:
		supplyAvailable = -m_bot.Observation()->GetFoodUsed();
		for (auto & unit : m_bot.UnitInfo().getUnits(Players::Self))
		{
			if (unit->unit_type == sc2::UNIT_TYPEID::PROTOSS_PYLON)
			{
				if (unit->build_progress < 1.0f)
				{
					return false;
				}
				supplyAvailable += 8;
			}
			else if (unit->unit_type == sc2::UNIT_TYPEID::PROTOSS_NEXUS && unit->build_progress == 1.0f)
			{
				supplyAvailable += 10;
			}
		}
		break;
	default:
		break;
	} 
	*/
	int supplyCost = m_queue.getHighestPriorityItem().type.supplyRequired(m_bot);
	// Available supply can be negative, which breaks the test below. Fix it.
	supplyAvailable = std::max(0, supplyAvailable);
	if (m_queue.getHighestPriorityItem().type.isSupply())
	{
		return false;
	}
	// if we don't have enough supply, we're supply blocked
	if (supplyAvailable < 5)
	{
		return true;
	}

	return false;

}

int ProductionManager::getFreeMinerals()
{
    return m_bot.Observation()->GetMinerals() - m_buildingManager.getReservedMinerals();
	//return m_bot.Observation()->GetMinerals();
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

size_t ProductionManager::NumberOfBuildingsQueued(sc2::UnitTypeID unit_type) const
{
	return m_buildingManager.NumberOfBuildingTypeInProduction(unit_type);
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
            ss << sc2::UnitTypeToName(unit->unit_type) << " " << unit->build_progress << "\n";
        }
    }

    ss << m_queue.getQueueInformation();

    m_bot.Map().drawTextScreen(sc2::Point2D(0.01f, 0.01f), ss.str(), sc2::Colors::Yellow);
}
