#pragma once
#include "sc2api/sc2_api.h"
#include "Common.h"
#include "BuildOrder.h"
#include "BuildingManager.h"
#include "BuildOrderQueue.h"

class CCBot;

class ProductionManager
{
    CCBot &       m_bot;

    BuildingManager m_buildingManager;
    BuildOrderQueue m_queue;
	size_t warpGate;
	int used;
    const sc2::Unit * getClosestUnitToPosition(const std::vector<const sc2::Unit *> & units, sc2::Point2D closestTo);
    bool    meetsReservedResources(const BuildType & type);
    bool    canMakeNow(const sc2::Unit * producer, const BuildType & type);
    bool    detectBuildOrderDeadlock();
	bool	detectSupplyDeadlock();
	bool    isMorphedBuilding(const sc2::UNIT_TYPEID t);
    void    setBuildOrder(const BuildOrder & buildOrder);
    void    create(const sc2::Unit * producer, BuildOrderItem & item);
    void    manageBuildOrderQueue();
	size_t  NumberOfBuildingsQueued(sc2::UnitTypeID unit_type) const;
    int     getFreeMinerals();
    int     getFreeGas();
	void    fixBuildOrderDeadlock();
	void    trainWarpGate();
	void    keepTrainWorker();
public:

    ProductionManager(CCBot & bot);
    void    onStart();
    void    onFrame();
    void    onUnitDestroy(const sc2::Unit * unit);
    void    drawProductionInformation();
	
    const sc2::Unit * getProducer(const MacroAct & type, sc2::Point2D closestTo = sc2::Point2D(0, 0));
};
