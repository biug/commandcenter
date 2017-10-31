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
	int warpGate;
	int used;
    const sc2::Unit * getClosestUnitToPosition(const std::vector<const sc2::Unit *> & units, sc2::Point2D closestTo);
    bool    meetsReservedResources(const BuildType & type);
    bool    canMakeNow(const sc2::Unit * producer, const BuildType & type);
    bool    detectBuildOrderDeadlock();
    void    setBuildOrder(const BuildOrder & buildOrder);
    void    create(const sc2::Unit * producer, BuildOrderItem & item);
    void    manageBuildOrderQueue();
    int     getFreeMinerals();
    int     getFreeGas();

public:

    ProductionManager(CCBot & bot);
	const sc2::Unit * pylonClosestToEnemy();
    void    onStart();
    void    onFrame();
    void    onUnitDestroy(const sc2::Unit * unit);
    void    drawProductionInformation();
	
    const sc2::Unit * getProducer(const BuildType & type, sc2::Point2D closestTo = sc2::Point2D(0, 0));
};
