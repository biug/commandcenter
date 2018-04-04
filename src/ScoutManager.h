#pragma once

#include "Common.h"

class CCBot;

class ScoutManager
{
    CCBot &   m_bot;

	const sc2::Unit * m_scoutUnit;
    std::string     m_scoutStatus;
    int             m_numScouts;
    bool            m_scoutUnderAttack;
    float           m_previousScoutHP;
	bool            m_arriveEnemyStart;
    bool            enemyWorkerInRadiusOf(const sc2::Point2D & pos) const;
    sc2::Point2D    getFleePosition() const;
    const sc2::Unit * closestEnemyWorkerTo(const sc2::Point2D & pos) const;
    void            moveScouts();
    void            drawScoutInformation();


public:

    ScoutManager(CCBot & bot);
	const sc2::Unit * getScout();
    void onStart();
    void onFrame();
    void setWorkerScout(const sc2::Unit * unit);
};