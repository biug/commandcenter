#include "Common.h"
#include "BuildingPlacer.h"
#include "CCBot.h"
#include "Building.h"
#include "Util.h"

BuildingPlacer::BuildingPlacer(CCBot & bot)
    : m_bot(bot)
{

}

void BuildingPlacer::onStart()
{
    m_reserveMap = std::vector< std::vector<bool> >(m_bot.Map().width(), std::vector<bool>(m_bot.Map().height(), false));
}

bool BuildingPlacer::isInResourceBox(int x, int y) const
{
    return false;
    return m_bot.Bases().getPlayerStartingBaseLocation(Players::Self)->isInResourceBox(x, y);
}

// makes final checks to see if a building can be built at a certain location
bool BuildingPlacer::canBuildHere(int bx, int by, const Building & b) const
{
    if (isInResourceBox(by, by))
    {
        return false;
    }

    // check the reserve map
    for (int x = bx; x < bx + Util::GetUnitTypeWidth(b.type, m_bot); x++)
    {
        for (int y = by; y < by + Util::GetUnitTypeHeight(b.type, m_bot); y++)
        {
            if (!m_bot.Map().isValid(x, y) || m_reserveMap[x][y])
            {
                return false;
            }
        }
    }

    // if it overlaps a base location return false
    if (tileOverlapsBaseLocation(bx, by, b.type))
    {
        return false;
    }

    return true;
}

//returns true if we can build this type of unit here with the specified amount of space.
bool BuildingPlacer::canBuildHereWithSpace(int bx, int by, const Building & b, int buildDist) const
{
    sc2::UnitTypeID type = b.type;

    //if we can't build here, we of course can't build here with space
    if (!canBuildHere(bx, by, b))
    {
        return false;
    }

    // height and width of the building
    int width  = Util::GetUnitTypeWidth(b.type, m_bot);
    int height = Util::GetUnitTypeHeight(b.type, m_bot);

    // TODO: make sure we leave space for add-ons. These types of units can have addons:

    // define the rectangle of the building spot
    int startx = bx - buildDist;
    int starty = by - buildDist;
    int endx   = bx + width + buildDist;
    int endy   = by + height + buildDist;

    // TODO: recalculate start and end positions for addons

    // if this rectangle doesn't fit on the map we can't build here
    if (startx < 0 || starty < 0 || endx > m_bot.Map().width() || endx < bx + width || endy > m_bot.Map().height())
    {
        return false;
    }

    // if we can't build here, or space is reserved, or it's in the resource box, we can't build here
    for (int x = startx; x < endx; x++)
    {
        for (int y = starty; y < endy; y++)
        {
            if (!Util::IsRefineryType(b.type))
            {
                if (!buildable(b, x, y) || m_reserveMap[x][y])
                {
                    return false;
                }
            }
        }
    }

    return true;
}

sc2::Point2DI BuildingPlacer::getBuildLocationNear(const Building & b, int buildDist) const
{
    Timer t;
    t.start();

    // get the precomputed vector of tile positions which are sorted closes to this location
    auto & closestToBuilding = m_bot.Map().getClosestTilesTo(b.desiredPosition);

    double ms1 = t.getElapsedTimeInMilliSec();

    // iterate through the list until we've found a suitable location
    for (size_t i(0); i < closestToBuilding.size(); ++i)
    {
        auto & pos = closestToBuilding[i];

        if (canBuildHereWithSpace((int)pos.x, (int)pos.y, b, buildDist))
        {
            double ms = t.getElapsedTimeInMilliSec();
            //printf("Building Placer Took %d iterations, lasting %lf ms @ %lf iterations/ms, %lf setup ms\n", (int)i, ms, (i / ms), ms1);

            return pos;
        }
    }

    double ms = t.getElapsedTimeInMilliSec();
    //printf("Building Placer Took %lf ms\n", ms);

    return sc2::Point2DI(0, 0);
}



sc2::Point2DI BuildingPlacer::GetNextCoordinateToWallWithBuilding(const Building & b, const sc2::Point2D start_point) const
{
	sc2::Point2D closest_point(0, 0);
	double closest_distance = std::numeric_limits<double>::max();

	// Get the closest ramp to our starting base. 
	const sc2::Point2D base_location = m_bot.Bases().getPlayerStartingBaseLocation(Players::Self)->getPosition();
	const int startx = start_point.x;
	const int starty = start_point.y;


	// No need to iterate through the edges of the map, as the edge can never be part of our wall. 
	// The smallest building is width 2, so shrink the iteration dimensions by that amount. 
	for (int y = starty-20; y < starty+20; ++y)
	{
		if (y <= 0 || y > m_bot.Map().height())
			continue;
		for (int x = startx - 20; x < startx + 20; ++x)
		{
			if (x <= 0 || x > m_bot.Map().width())
				continue;
			// If we can walk on it, but not build on it, it is most likely a ramp.
			// TODO: That is not actually correct, come up with a beter way to detect ramps. 
			if (m_bot.Map().IsAnyTileAdjacentToTileType(sc2::Point2DI(x, y), MapTileType::Ramp, sc2::UNIT_TYPEID::TERRAN_SUPPLYDEPOT)
				&& CanBuildHere(x, y, b))
			{
				// The first depot in a wall has to be next to, well, a wall. 
				// This allows the depot wall to be built correctly on AbyssalReefLE.
				//if (bot_.Config().MapName == "AbyssalReefLE" &&
				//    information_manager_.UnitInfo().GetNumDepots(sc2::Unit::Alliance::Self) < 2
				//    && !(IsTileCornerOfTileType(sc2::Point2DI(x, y), MapTileType::CantWalk))
				//    continue;

				if (/*IsTileCornerReserved(sc2::Point2DI(x, y))
					|| */m_bot.Map().terrainHeight(x, y) < 10.5)
					continue;

				//// Don't wall of at Proxima Station's pocket expansion.
				//if (bot_.Config().MapName == "ProximaStationLE" 
				//    && information_manager_.UnitInfo().GetNumDepots(sc2::Unit::Alliance::Self) < 3
				// && ((y < 49 || y > 119) || TerrainHeight(x, y) < 10.5))
				//    continue;

				const sc2::Point2D point(x, y);
				const double distance = Util::DistSq(point, base_location);
				if (distance < closest_distance)
				{
					closest_point = point;
					closest_point.x;
					closest_distance = distance;
				}
			}
		}
	}
	return sc2::Point2DI(closest_point.x, closest_point.y);
}


bool BuildingPlacer::tileOverlapsBaseLocation(int x, int y, sc2::UnitTypeID type) const
{
    // if it's a resource depot we don't care if it overlaps
    if (Util::IsTownHallType(type))
    {
        return false;
    }

    // dimensions of the proposed location
    int tx1 = x;
    int ty1 = y;
    int tx2 = tx1 + Util::GetUnitTypeWidth(type, m_bot);
    int ty2 = ty1 + Util::GetUnitTypeHeight(type, m_bot);

    // for each base location
    for (const BaseLocation * base : m_bot.Bases().getBaseLocations())
    {
        // dimensions of the base location
        int bx1 = (int)base->getDepotPosition().x;
        int by1 = (int)base->getDepotPosition().y;
        int bx2 = bx1 + Util::GetUnitTypeWidth(Util::GetTownHall(m_bot.GetPlayerRace(Players::Self)), m_bot);
        int by2 = by1 + Util::GetUnitTypeHeight(Util::GetTownHall(m_bot.GetPlayerRace(Players::Self)), m_bot);

        // conditions for non-overlap are easy
        bool noOverlap = (tx2 < bx1) || (tx1 > bx2) || (ty2 < by1) || (ty1 > by2);

        // if the reverse is true, return true
        if (!noOverlap)
        {
            return true;
        }
    }

    // otherwise there is no overlap
    return false;
}

bool BuildingPlacer::buildable(const Building & b, int x, int y) const
{
    // TODO: does this take units on the map into account?
    if (!m_bot.Map().isValid(x, y) || !m_bot.Map().canBuildTypeAtPosition(x, y, b.type))
    {
        return false;
    }

    // todo: check that it won't block an addon

    return true;
}

void BuildingPlacer::reserveTiles(int bx, int by, int width, int height)
{
    size_t rwidth = m_reserveMap.size();
    size_t rheight = m_reserveMap[0].size();
    for (size_t x = bx; x < bx + width && x < rwidth; x++)
    {
        for (size_t y = by; y < by + height && y < rheight; y++)
        {
            m_reserveMap[x][y] = true;
        }
    }
}

void BuildingPlacer::drawReservedTiles()
{
    if (!m_bot.Config().DrawReservedBuildingTiles)
    {
        return;
    }

    int rwidth = (int)m_reserveMap.size();
    int rheight = (int)m_reserveMap[0].size();

    for (int x = 0; x < rwidth; ++x)
    {
        for (int y = 0; y < rheight; ++y)
        {
            if (m_reserveMap[x][y] || isInResourceBox(x, y))
            {
                int x1 = x*32 + 8;
                int y1 = y*32 + 8;
                int x2 = (x+1)*32 - 8;
                int y2 = (y+1)*32 - 8;

                m_bot.Map().drawBox((float)x1, (float)y1, (float)x2, (float)y2, sc2::Colors::Yellow);
            }
        }
    }
}

void BuildingPlacer::freeTiles(int bx, int by, int width, int height)
{
    int rwidth = (int)m_reserveMap.size();
    int rheight = (int)m_reserveMap[0].size();

    for (int x = bx; x < bx + width && x < rwidth; x++)
    {
        for (int y = by; y < by + height && y < rheight; y++)
        {
            m_reserveMap[x][y] = false;
        }
    }
}

sc2::Point2DI BuildingPlacer::getRefineryPosition()
{
    sc2::Point2D closestGeyser(0, 0);
    double minGeyserDistanceFromHome = std::numeric_limits<double>::max();
    sc2::Point2D homePosition = m_bot.GetStartLocation();

	std::unordered_set<const sc2::Unit*> refineries;
	for (auto & unit : m_bot.UnitInfo().getUnits(Players::Self))
	{
		if (unit && Util::IsRefinery(unit))
		{
			refineries.insert(unit);
		}
	}

    for (auto & base : m_bot.Bases().getOccupiedBaseLocations(Players::Self))
    {
		if (!base) continue;
		for (const auto & geyser : base->getGeysers())
		{
			bool check = true;
			for (auto & refinery : refineries)
			{
				if (refinery->pos.x == geyser->pos.x && refinery->pos.y == geyser->pos.y)
				{
					check = false;
					break;
				}
			}
			if (!check) continue;

			double homeDistance = Util::PlanerDist(geyser->pos, homePosition);

			if (homeDistance < minGeyserDistanceFromHome)
			{
				minGeyserDistanceFromHome = homeDistance;
				closestGeyser = geyser->pos;
			}
		}
    }

    return Util::GetTilePosition(closestGeyser);
}

bool BuildingPlacer::isReserved(int x, int y) const
{
    int rwidth = (int)m_reserveMap.size();
    int rheight = (int)m_reserveMap[0].size();
    if (x < 0 || y < 0 || x >= rwidth || y >= rheight)
    {
        return false;
    }

    return m_reserveMap[x][y];
}

// makes final checks to see if a building can be built at a certain location
bool BuildingPlacer::CanBuildHere(const int bx, const int by, const Building & b) const
{

	// Will the starcraft engine allow the building to be built at the location?
	if (!m_bot.Map().isValid(bx, by) || !m_bot.Map().canBuildTypeAtPosition(bx, by,b.type))
		return false;

	// We are not allowed to build on any tile that we have reserved.
	if (canBuildHereWithSpace(bx,by,b,0))
		return false;

	// If none of the above conditions failed, we must be allowed to build at the location.
	return true;
}
