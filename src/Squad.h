#pragma once

#include "Common.h"
#include "MeleeManager.h"
#include "RangedManager.h"
#include "StalkerManager.h"
#include "SquadOrder.h"
#include "MarineManager.h"
#include "MarauderManager.h"
#include "TankManager.h"
#include "HellionManager.h"
#include "ReaperManager.h"
#include "MedivacManager.h"
#include "GhostManager.h"
class CCBot;

class Squad
{
    CCBot &             m_bot;

    std::string         m_name;
    std::set<const sc2::Unit *> m_units;
    std::string         m_regroupStatus;
    int                 m_lastRetreatSwitch;
    bool                m_lastRetreatSwitchVal;
    size_t              m_priority;

    SquadOrder          m_order;
    MeleeManager        m_meleeManager;
    RangedManager       m_rangedManager;
	StalkerManager		m_stalkerManager;
	MarineManager       m_marineManager;
	MarauderManager     m_marauderManager;
	TankManager         m_tankManager;
	HellionManager		m_hellionManager;
	ReaperManager		m_reaperManager;
	MedivacManager      m_medivacManager;
	GhostManager		m_ghostManager;
    std::map<const sc2::Unit *, bool> m_nearEnemy;

    const sc2::Unit * unitClosestToEnemy() const;
	
    void updateUnits();
    void addUnitsToMicroManagers();
    void setNearEnemyUnits();
    void setAllUnits();

    bool isUnitNearEnemy(const sc2::Unit * unit) const;
    bool needsToRegroup() const;
    int  squadUnitsNear(const sc2::Point2D & pos) const;

public:

    Squad(const std::string & name, const SquadOrder & order, size_t priority, CCBot & bot);
    Squad(CCBot & bot);
	
    void onFrame();
    void setSquadOrder(const SquadOrder & so);
    void addUnit(const sc2::Unit * unit);
    void removeUnit(const sc2::Unit * unit);
    void clear();

    bool containsUnit(const sc2::Unit * unit) const;
    bool isEmpty() const;
    size_t getPriority() const;
    void setPriority(const size_t & priority);
    const std::string & getName() const;

    sc2::Point2D calcCenter() const;
    sc2::Point2D calcRegroupPosition() const;

    const std::set<const sc2::Unit *> & getUnits() const;
    const SquadOrder & getSquadOrder() const;
};
