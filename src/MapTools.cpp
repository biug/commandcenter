#include "MapTools.h"
#include "Util.h"
#include "CCBot.h"

#include <iostream>
#include <sstream>
#include <fstream>

const size_t LegalActions = 4;
const int actionX[LegalActions] ={1, -1, 0, 0};
const int actionY[LegalActions] ={0, 0, 1, -1};

typedef std::vector<std::vector<bool>> vvb;
typedef std::vector<std::vector<int>>  vvi;
typedef std::vector<std::vector<float>>  vvf;

// constructor for MapTools
MapTools::MapTools(CCBot & bot)
    : m_bot     (bot)
    , m_width   (0)
    , m_height  (0)
    , m_maxZ    (0.0f)
    , m_frame   (0)
	, unit_info_(bot)
{

}

void MapTools::onStart()
{
    m_width  = m_bot.Observation()->GetGameInfo().width;
    m_height = m_bot.Observation()->GetGameInfo().height;

    m_walkable       = vvb(m_width, std::vector<bool>(m_height, true));
    m_buildable      = vvb(m_width, std::vector<bool>(m_height, false));
    m_depotBuildable = vvb(m_width, std::vector<bool>(m_height, false));
    m_lastSeen       = vvi(m_width, std::vector<int>(m_height, 0));
    m_sectorNumber   = vvi(m_width, std::vector<int>(m_height, 0));
    m_terrainHeight  = vvf(m_width, std::vector<float>(m_height, 0.0f));

	unit_info_.onStart();
    // Set the boolean grid data from the Map
    for (size_t x(0); x < m_width; ++x)
    {
        for (size_t y(0); y < m_height; ++y)
        {
            m_buildable[x][y]   = Util::Placement(m_bot.Observation()->GetGameInfo(), sc2::Point2D(x+0.5f, y+0.5f));
            m_walkable[x][y]    = m_buildable[x][y] || Util::Pathable(m_bot.Observation()->GetGameInfo(), sc2::Point2D(x+0.5f, y+0.5f));
            m_terrainHeight[x][y]   = Util::TerainHeight(m_bot.Observation()->GetGameInfo(), sc2::Point2D(x+0.5f, y+0.5f));
        }
    }

    for (auto & unit : m_bot.Observation()->GetUnits(sc2::Unit::Alliance::Neutral))
    {
        m_maxZ = std::max(unit->pos.z, m_maxZ);
    }

    computeConnectivity();

	dps_map_ = std::vector<std::vector<float>>{};
	dps_map_.clear();
	for (int y = 0; y < height(); ++y)
	{
		dps_map_.push_back(std::vector<float>());
		for (int x = 0; x < width(); ++x)
		{
			// There is an inherit "danger" for traveling through any square. 
			// Don't use 0, otherwise we won't find the "shortest and safest path"
			dps_map_[y].push_back(1);
		}
	}

	gs_map_ = std::vector<std::vector<float>>{};
	gs_map_.clear();
	for (int y = 0; y < height(); ++y)
	{
		gs_map_.push_back(std::vector<float>());
		for (int x = 0; x < width(); ++x)
		{
			// Don't use 0, otherwise we won't find the "shortest and safest path"
			gs_map_[y].push_back(1);
		}
	}

	ss_map_ = std::vector<std::vector<float>>{};
	ss_map_.clear();
	for (int y = 0; y < height(); ++y)
	{
		ss_map_.push_back(std::vector<float>());
		for (int x = 0; x < width(); ++x)
		{
			// Don't use 0, otherwise we won't find the "shortest and safest path"
			ss_map_[y].push_back(1);
		}
	}
}

void MapTools::onFrame()
{
    m_frame++;
	unit_info_.onFrame();
    for (int x=0; x<m_width; ++x)
    {
        for (int y=0; y<m_height; ++y)
        {
            if (isVisible(sc2::Point2D((float)x, (float)y)))
            {
                m_lastSeen[x][y] = m_frame;
            }
        }
    }

	drawTextScreen(sc2::Point2D(100, 100), std::to_string(m_frame), sc2::Colors::Green);
	drawTextScreen(sc2::Point2D(100, 110), std::to_string(m_frame), sc2::Colors::Green);
	drawTextScreen(sc2::Point2D(100, 120), std::to_string(m_frame), sc2::Colors::Green);

    draw();


	// Reset dps_map_

	for (int y = 0; y < dps_map_.size(); ++y)
	{
		for (int x = 0; x < dps_map_[y].size(); ++x)
		{
			dps_map_[y][x] = 1;
		}
	}


	// Reset gs_map_
	
	for (int y = 0; y < gs_map_.size(); ++y)
	{
		for (int x = 0; x < gs_map_[y].size(); ++x)
		{
			gs_map_[y][x] = 1;
		}
	}
	

	// Update dps_map_
	for (const auto & unit_info_pair : unit_info_.getUnitInfoMap(Players::Enemy))
	{
		const auto unit_info = unit_info_pair.second;
		const int damage = Util::GetAttackDamage(unit_info.type, m_bot);
		if (damage == 0) continue;
		int range = static_cast<int>(Util::GetAttackRange(unit_info.type, m_bot)) + 1;
		//  Melee units are dangerous too.
		if (range == 0 && !Util::IsBuilding(unit_info.type)) range = 2;

		for (float y = 0; y < dps_map_.size(); ++y)
		{
			for (float x = 0; x < dps_map_[y].size(); ++x)
			{
				for (float falloff = 0; falloff < 25; ++falloff)
				{
					// Danger zone falloff only applies to army units. 
					if (!Util::IsWorker(unit_info.unit) && falloff > 1)
						if (Util::DistSq(sc2::Point2D{ x, y }, unit_info.lastPosition) <= static_cast<float>(range*range + falloff * falloff))
						{
							dps_map_[y][x] += static_cast<float>(damage / (falloff * 2 + 1));
							break;
						}
				}
			}
		}
	}

	for (int y = 0; y < height(); ++y)
	{
		for (int x = 0; x < width(); ++x)
		{
			if (!isWalkable(x, y))
				dps_map_[y][x] = 999;
		}
	}


	// Update gs_map_
	for (const auto & unit_info_pair : unit_info_.getUnitInfoMap(Players::Self))
	{
		const auto unit_info = unit_info_pair.second;
		const int damage = Util::GetAttackDamage(unit_info.type, m_bot);
		//const bool Util::canAttackSky(unit->unit_type)
		if (damage == 0) continue;
		int range = static_cast<int>(Util::GetAttackRange(unit_info.type, m_bot)) + 1;
		//  
		if (range == 0 && !Util::IsBuilding(unit_info.type)) range = 2;

		for (float y = 0; y < gs_map_.size(); ++y)
		{
			for (float x = 0; x < gs_map_[y].size(); ++x)
			{
				for (float falloff = 0; falloff < 25; ++falloff)
				{
					// 
					if (!Util::IsWorker(unit_info.unit) && falloff > 1)
						if (Util::DistSq(sc2::Point2D{ x, y }, unit_info.lastPosition) <= static_cast<float>(range*range + falloff * falloff))
						{
							gs_map_[y][x] += static_cast<float>(damage / (falloff * 2 + 1));
							break;
						}
				}
			}
		}
	}

	for (int y = 0; y < height(); ++y)
	{
		for (int x = 0; x < width(); ++x)
		{
			if (!isWalkable(x, y))
				gs_map_[y][x] = 0.01;
		}
	}

	// Update ss_map_
	for (const auto & unit_info_pair : unit_info_.getUnitInfoMap(Players::Self))
	{
		const auto unit_info = unit_info_pair.second;
		const int damage = Util::GetAttackDamage(unit_info.type, m_bot);
		const bool skyAttack = Util::canAttackSky(unit_info.type);
		if (damage == 0||skyAttack==false) continue;
		int range = static_cast<int>(Util::GetAttackRange(unit_info.type, m_bot)) + 1;
		//  
		if (range == 0 && !Util::IsBuilding(unit_info.type)) range = 2;

		for (float y = 0; y < ss_map_.size(); ++y)
		{
			for (float x = 0; x < ss_map_[y].size(); ++x)
			{
				for (float falloff = 0; falloff < 25; ++falloff)
				{
					// 
					if (!Util::IsWorker(unit_info.unit) && falloff > 1)
						if (Util::DistSq(sc2::Point2D{ x, y }, unit_info.lastPosition) <= static_cast<float>(range*range + falloff * falloff))
						{
							ss_map_[y][x] += static_cast<float>(damage / (falloff * 2 + 1));
							break;
						}
				}
			}
		}
	}


}

void MapTools::computeConnectivity()
{
    // the fringe data structe we will use to do our BFS searches
    std::vector<sc2::Point2D> fringe;
    fringe.reserve(m_width*m_height);
    int sectorNumber = 0;

    // for every tile on the map, do a connected flood fill using BFS
    for (int x=0; x<m_width; ++x)
    {
        for (int y=0; y<m_height; ++y)
        {
            // if the sector is not currently 0, or the map isn't walkable here, then we can skip this tile
            if (getSectorNumber(x, y) != 0 || !isWalkable(x, y))
            {
                continue;
            }

            // increase the sector number, so that walkable tiles have sectors 1-N
            sectorNumber++;

            // reset the fringe for the search and add the start tile to it
            fringe.clear();
            fringe.push_back(sc2::Point2D(x+0.5f, y+0.5f));
            m_sectorNumber[x][y] = sectorNumber;

            // do the BFS, stopping when we reach the last element of the fringe
            for (size_t fringeIndex=0; fringeIndex<fringe.size(); ++fringeIndex)
            {
                auto & tile = fringe[fringeIndex];

                // check every possible child of this tile
                for (size_t a=0; a<LegalActions; ++a)
                {
                    sc2::Point2D nextTile(tile.x + actionX[a], tile.y + actionY[a]);

                    // if the new tile is inside the map bounds, is walkable, and has not been assigned a sector, add it to the current sector and the fringe
                    if (isValid(nextTile) && isWalkable(nextTile) && (getSectorNumber(nextTile) == 0))
                    {
                        m_sectorNumber[(int)nextTile.x][(int)nextTile.y] = sectorNumber;
                        fringe.push_back(nextTile);
                    }
                }
            }
        }
    }
}

bool MapTools::isExplored(const sc2::Point2D & pos) const
{
    if (!isValid(pos)) { return false; }

    sc2::Visibility vis = m_bot.Observation()->GetVisibility(pos);
    return vis == sc2::Visibility::Fogged || vis == sc2::Visibility::Visible;
}

bool MapTools::isVisible(const sc2::Point2D & pos) const
{
    if (!isValid(pos)) { return false; }

    return m_bot.Observation()->GetVisibility(pos) == sc2::Visibility::Visible;
}

bool MapTools::isPowered(const sc2::Point2D & pos) const
{
    for (auto & powerSource : m_bot.Observation()->GetPowerSources())
    {
        if (Util::Dist(pos, powerSource.position) < powerSource.radius)
        {
            return true;
        }
    }

    return false;
}

float MapTools::terrainHeight(float x, float y) const
{
    return m_terrainHeight[(int)x][(int)y];
}

//int MapTools::getGroundDistance(const sc2::Point2D & src, const sc2::Point2D & dest) const
//{
//    return (int)Util::Dist(src, dest);
//}

int MapTools::getGroundDistance(const sc2::Point2D & src, const sc2::Point2D & dest) const
{
    if (_allMaps.size() > 50)
    {
        _allMaps.clear();
    }

    return getDistanceMap(dest).getDistance(src);
}

const DistanceMap & MapTools::getDistanceMap(const sc2::Point2D & tile) const
{
	return getDistanceMap(Util::GetTilePosition(tile));
}
const DistanceMap & MapTools::getDistanceMap(const sc2::Point2DI & tile) const
{
	std::pair<int, int> intTile(tile.x, tile.y);

	if (_allMaps.find(intTile) == _allMaps.end())
	{
		_allMaps[intTile] = DistanceMap();
		_allMaps[intTile].computeDistanceMap(m_bot, tile);
	}

	return _allMaps[intTile];
}

int MapTools::getSectorNumber(int x, int y) const
{
    if (!isValid(x, y))
    {
        return 0;
    }

    return m_sectorNumber[x][y];
}

int MapTools::getSectorNumber(const sc2::Point2D & pos) const
{
    return getSectorNumber((int)pos.x, (int)pos.y);
}

bool MapTools::isValid(int x, int y) const
{
    return x >= 0 && y >= 0 && x < m_width && y < m_height;
}

bool MapTools::isValid(const sc2::Point2D & pos) const
{
    return isValid((int)pos.x, (int)pos.y);
}

bool MapTools::isValidTile(int tileX, int tileY) const
{
	return tileX >= 0 && tileY >= 0 && tileX < m_width && tileY < m_height;
}

bool MapTools::isValidTile(const CCTilePosition & tile) const
{
	return isValidTile(tile.x, tile.y);
}
void MapTools::draw() const
{
    sc2::Point2D camera = m_bot.Observation()->GetCameraPos();
    for (float x = camera.x - 16.0f; x < camera.x + 16.0f; ++x)
    {
        for (float y = camera.y - 16.0f; y < camera.y + 16.0f; ++y)
        {
            if (!isValid((int)x, (int)y))
            {
                continue;
            }

            if (m_bot.Config().DrawWalkableSectors)
            {
                std::stringstream ss;
                ss << getSectorNumber((int)x, (int)y);
                m_bot.Debug()->DebugTextOut(ss.str(), sc2::Point3D(x + 0.5f, y + 0.5f, m_maxZ + 0.1f), sc2::Colors::Yellow);
            }

            if (m_bot.Config().DrawTileInfo)
            {
                sc2::Color color = isWalkable((int)x, (int)y) ? sc2::Colors::Green : sc2::Colors::Red;
                if (isWalkable((int)x, (int)y) && !isBuildable((int)x, (int)y))
                {
                    color = sc2::Colors::Yellow;
                }

                drawSquare(x, y, x+1, y+1, color);
            }
        }
    }
}

void MapTools::drawLine(float x1, float y1, float x2, float y2, const sc2::Color & color) const
{
    m_bot.Debug()->DebugLineOut(sc2::Point3D(x1, y1, m_maxZ + 0.2f), sc2::Point3D(x2, y2, m_maxZ + 0.2f), color);
}

void MapTools::drawLine(const sc2::Point2D & min, const sc2::Point2D max, const sc2::Color & color) const
{
    m_bot.Debug()->DebugLineOut(sc2::Point3D(min.x, min.y, m_maxZ + 0.2f), sc2::Point3D(max.x, max.y, m_maxZ + 0.2f), color);
}

void MapTools::drawSquare(float x1, float y1, float x2, float y2, const sc2::Color & color) const
{
    m_bot.Debug()->DebugLineOut(sc2::Point3D(x1, y1, m_maxZ), sc2::Point3D(x1+1, y1, m_maxZ), color);
    m_bot.Debug()->DebugLineOut(sc2::Point3D(x1, y1, m_maxZ), sc2::Point3D(x1, y1+1, m_maxZ), color);
    m_bot.Debug()->DebugLineOut(sc2::Point3D(x1+1, y1+1, m_maxZ), sc2::Point3D(x1+1, y1, m_maxZ), color);
    m_bot.Debug()->DebugLineOut(sc2::Point3D(x1+1, y1+1, m_maxZ), sc2::Point3D(x1, y1+1, m_maxZ), color);
}

void MapTools::drawBox(float x1, float y1, float x2, float y2, const sc2::Color & color) const
{
    m_bot.Debug()->DebugBoxOut(sc2::Point3D(x1, y1, m_maxZ + 2.0f), sc2::Point3D(x2, y2, m_maxZ-5.0f), color);
}

void MapTools::drawBox(const sc2::Point2D & min, const sc2::Point2D max, const sc2::Color & color) const
{
    m_bot.Debug()->DebugBoxOut(sc2::Point3D(min.x, min.y, m_maxZ + 2.0f), sc2::Point3D(max.x, max.y, m_maxZ-5.0f), color);
}

void MapTools::drawSphere(const sc2::Point2D & pos, float radius, const sc2::Color & color) const
{
    m_bot.Debug()->DebugSphereOut(sc2::Point3D(pos.x, pos.y, m_maxZ), radius, color);
}

void MapTools::drawSphere(float x, float y, float radius, const sc2::Color & color) const
{
    m_bot.Debug()->DebugSphereOut(sc2::Point3D(x, y, m_maxZ), radius, color);
}

void MapTools::drawText(const sc2::Point2D & pos, const std::string & str, const sc2::Color & color) const
{
    m_bot.Debug()->DebugTextOut(str, sc2::Point3D(pos.x, pos.y, m_maxZ), color);
}

void MapTools::drawTextScreen(const sc2::Point2D & pos, const std::string & str, const sc2::Color & color) const
{
    m_bot.Debug()->DebugTextOut(str, pos, color);
}

void MapTools::drawTextScreenAdjustSize(const sc2::Point2D & pos, const std::string & str, const sc2::Color & color, uint32_t size = 8) const
{
	m_bot.Debug()->DebugTextOut(str, pos, color, size);
}

bool MapTools::isConnected(int x1, int y1, int x2, int y2) const
{
    if (!isValid(x1, y1) || !isValid(x2, y2))
    {
        return false;
    }

    int s1 = getSectorNumber(x1, y1);
    int s2 = getSectorNumber(x2, y2);

    return s1 != 0 && (s1 == s2);
}
bool MapTools::isConnected(const CCTilePosition & p1, const CCTilePosition & p2) const
{
	return isConnected(p1.x, p1.y, p2.x, p2.y);
}

bool MapTools::isConnected(const sc2::Point2D & p1, const sc2::Point2D & p2) const
{
    return isConnected((int)p1.x, (int)p1.y, (int)p2.x, (int)p2.y);
}

bool MapTools::isBuildable(int x, int y) const
{
    if (!isValid(x, y))
    {
        return false;
    }

    return m_buildable[x][y];
}

bool MapTools::canBuildTypeAtPosition(int x, int y, sc2::UnitTypeID type) const
{
    return m_bot.Query()->Placement(m_bot.Data(type).buildAbility, sc2::Point2D((float)x, (float)y));
}

bool MapTools::isBuildable(const sc2::Point2D & tile) const
{
    return isBuildable((int)tile.x, (int)tile.y);
}

void MapTools::printMap()
{
    std::stringstream ss;
    for (int y(0); y < m_height; ++y)
    {
        for (int x(0); x < m_width; ++x)
        {
            ss << isWalkable(x, y);
        }

        ss << "\n";
    }

    std::ofstream out("map.txt");
    out << ss.str();
    out.close();
}

bool MapTools::isDepotBuildableTile(const sc2::Point2D & tile) const
{
    if (!isValid(tile))
    {
        return false;
    }

    return m_depotBuildable[(int)tile.x][(int)tile.y];
}

bool MapTools::isWalkable(int x, int y) const
{
    if (!isValid(x, y))
    {
        return false;
    }

    return m_walkable[x][y];
}

bool MapTools::isWalkable(const sc2::Point2D & tile) const
{
    return isWalkable((int)tile.x, (int)tile.y);
}

int MapTools::width() const
{
    return m_width;
}

int MapTools::height() const
{
    return m_height;
}

int MapTools::frame() const
{
	return m_frame;
}

const std::vector<sc2::Point2DI> & MapTools::getClosestTilesTo(const sc2::Point2DI & pos) const
{
    return getDistanceMap(pos).getSortedTiles();
}

bool MapTools::isWalkable(const CCTilePosition & tile) const
{
	return isWalkable(tile.x, tile.y);
}
void MapTools::drawBoxAroundUnit(const UnitTag & unitTag, sc2::Color color) const
{
    const sc2::Unit * unit = m_bot.GetUnit(unitTag);

    if (!unit) { return; }

    sc2::Point3D p_min = unit->pos;
    p_min.x -= 2.0f;
    p_min.y -= 2.0f;
    p_min.z -= 2.0f;

    sc2::Point3D p_max = unit->pos;
    p_max.x += 2.0f;
    p_max.y += 2.0f;
    p_max.z += 2.0f;

    drawSquare(unit->pos.x - 2.0f, unit->pos.y - 2.0f, unit->pos.x + 2.0f, unit->pos.y + 2.0f, color);
}

void MapTools::drawSphereAroundUnit(const UnitTag & unitTag, sc2::Color color) const
{
    const sc2::Unit * unit = m_bot.GetUnit(unitTag);

    if (!unit) { return; }

    drawSphere(unit->pos, 1, color);
}

bool MapTools::IsAnyTileAdjacentToTileType(const sc2::Point2DI p, const MapTileType tile_type, const sc2::UnitTypeID building_type) const
{
	const int width = Util::GetUnitTypeWidth(building_type, m_bot);
	const int height = Util::GetUnitTypeHeight(building_type, m_bot);

	// define the rectangle of the building spot
	const int startx = p.x - width/2;
	const int starty = p.y - height/2;
	const int endx = p.x + width/2 ;
	const int endy = p.y + height/2;

	// if this rectangle doesn't fit on the map we can't build here
	if (endx < 0 || endy < 0 || startx >m_width || starty > m_height)
	{
		return false;
	}

	// if we can't build here, or space is reserved, or it's in the resource box, we can't build here.
	// Yes, < is correct. Don't use <=.
	// Due to how starcraft calculates the tile type, things are sort of "shifted down and left" slightly. 
	// Turn on DrawTileInfo to see the "shifting" in action. 
	for (int x = startx; x < endx; ++x)
	{
		for (int y = starty; y < endy; ++y)
		{
			if (IsTileAdjacentToTileType(sc2::Point2DI{ x,y }, tile_type))
			{
				return true;
			}
		}
	}
	return false;
}

bool MapTools::IsTileAdjacentToTileType(const sc2::Point2DI p, const MapTileType tile_type) const
{
	if (p.x > 0 && IsTileTypeOf(p.x - 1, p.y, tile_type))
		return true;
	if (p.x < m_width - 1 && IsTileTypeOf(p.x + 1, p.y, tile_type))
		return true;
	if (p.y > 0 && IsTileTypeOf(p.x, p.y - 1, tile_type))
		return true;
	if (p.y < m_height - 1 && IsTileTypeOf(p.x, p.y + 1, tile_type))
		return true;
	return false;
}

bool MapTools::IsTileTypeOf(const int x, const int y, const MapTileType tile_type) const
{
	if (tile_type == MapTileType::CantWalk
		&& isWalkable(x, y))
		return true;;
	if (tile_type == MapTileType::CantBuild
		&& isBuildable(x, y))
		return true;
	if (tile_type == MapTileType::Ramp &&
		isWalkable(x, y) && !isBuildable(x, y))
		return true;
	return false;
}


sc2::Point2DI MapTools::getLeastRecentlySeenPosition() const
{
    int minSeen = std::numeric_limits<int>::max();
    sc2::Point2DI leastSeen(0.0f, 0.0f);
    const BaseLocation * baseLocation = m_bot.Bases().getPlayerStartingBaseLocation(Players::Self);

    for (auto & tile : baseLocation->getClosestTiles())
    {
		BOT_ASSERT(isValidTile(tile), "How is this tile not valid?");

        int lastSeen = m_lastSeen[(int)tile.x][(int)tile.y];
        if (lastSeen < minSeen)
        {
            minSeen = lastSeen;
            leastSeen = tile;
        }
    }

    return leastSeen;
}
bool MapTools::isBuildable(CCTilePosition & tile) const
{
	return isBuildable(tile.x, tile.y);
}

std::vector<std::vector<float>> MapTools::GetDPSMap() const
{
	return dps_map_;
}

std::vector<std::vector<float>> MapTools::GetGSMap() const
{
	return gs_map_;
}

std::vector<std::vector<float>> MapTools::GetSSMap() const
{
	return ss_map_;
}