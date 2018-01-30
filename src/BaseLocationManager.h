#pragma once

#include "sc2api/sc2_api.h"
#include "BaseLocation.h"

class CCBot;

class BaseLocationManager
{
    CCBot & m_bot;

    std::vector<BaseLocation>                       m_baseLocationData;
    std::vector<const BaseLocation *>               m_baseLocationPtrs;
    std::vector<const BaseLocation *>               m_startingBaseLocations;
    std::map<int, const BaseLocation *>             m_playerStartingBaseLocations;
    std::map<int, std::set<const BaseLocation *>>   m_occupiedBaseLocations;
    std::vector<std::vector<BaseLocation *>>        m_tileBaseLocations;
	std::map<int, const BaseLocation *>				m_playerNaturalLocations;
	std::map<int, const BaseLocation *>				m_playerThirdLocations;

    BaseLocation * getBaseLocation(const sc2::Point2D & pos) const;

public:

    BaseLocationManager(CCBot & bot);
    
    void onStart();
    void onFrame();
    void drawBaseLocations();

    const std::vector<const BaseLocation *> & getBaseLocations() const;
    const std::vector<const BaseLocation *> & getStartingBaseLocations() const;
    const std::set<const BaseLocation *> & getOccupiedBaseLocations(int player) const;
    const BaseLocation * getPlayerStartingBaseLocation(int player) const;
	const BaseLocation * getPlayerNaturalLocation(int player) const;
	const BaseLocation * getPlayerThirdLocation(int player) const;
    
    sc2::Point2DI getNextExpansion(int player) const;

};
