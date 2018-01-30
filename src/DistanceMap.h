#pragma once

#include "Common.h"
#include <map>

class CCBot;


class DistanceMap 
{
    int m_width;
    int m_height;
    sc2::Point2DI m_startTile;

    std::vector<std::vector<int>> m_dist;
    std::vector<sc2::Point2DI> m_sortedTiles;
    
public:
    
    DistanceMap();
    void computeDistanceMap(CCBot & m_bot, const sc2::Point2DI & startTile);

    int getDistance(int tileX, int tileY) const;
    int getDistance(const sc2::Point2D & pos) const;
	int getDistance(const sc2::Point2DI & pos) const;
    // given a position, get the position we should move to to minimize distance
    const std::vector<sc2::Point2DI> & getSortedTiles() const;
    const sc2::Point2DI & getStartTile() const;

    void draw(CCBot & bot) const;
};