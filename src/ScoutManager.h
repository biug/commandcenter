#pragma once

#include "Common.h"
#include "BaseLocation.h"

class CCBot;

class ScoutManager
{
    CCBot &   m_bot;

	const sc2::Unit * m_scoutUnit;
	const sc2::Unit * m_searchProxyUnit;
    std::string     m_scoutStatus;
    int             m_numScouts;
    bool            m_scoutUnderAttack;
	bool			searchproxyUnitUnderAttack;
	bool			m_foundProxy;
	bool			isHiddenProxy(std::vector<const sc2::Unit *> enemyunnitinsight);
    float           m_previousScoutHP;
	float			searchproxyUnitpreviousHP;
	bool            m_arriveEnemyStart;
	bool			canbeingattacked(const sc2::Unit * unit);
    bool            enemyWorkerInRadiusOf(const sc2::Point2D & pos) const;
    sc2::Point2D    getFleePosition() const;
    const sc2::Unit * closestEnemyWorkerTo(const sc2::Point2D & pos) const;
    void            moveScouts();
	void			searchProxy();
	const BaseLocation* getClosestBase(std::vector<const BaseLocation* > UnknownBase);
    void            drawScoutInformation();
	std::vector<const sc2::Unit *> getEnemyUnitInfo(const sc2::Unit * unit);
	std::vector<const BaseLocation * > UnknownBase;

public:

    ScoutManager(CCBot & bot);
	const sc2::Unit * getScout();
    void onStart();
    void onFrame();
    void setWorkerScout(const sc2::Unit * unit);
	void setProxyScout(const sc2::Unit * unit);
};