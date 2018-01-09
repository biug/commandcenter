#pragma once

#include "sc2api/sc2_api.h"
#include "BotAssert.h"
#include <iostream>
#include <vector>
#include <cassert>
#include <set>
#include <fstream>
#include <streambuf>
#include <string>
#include <unordered_set>
#include <unordered_map>

typedef uint64_t UnitTag;

namespace Players
{
    enum {Self = 0, Enemy = 1};
}
#pragma once

#include "BotAssert.h"
#include <iostream>
#include <vector>
#include <cassert>
#include <set>
#include <fstream>
#include <streambuf>
#include <string>
#include <array>
#include "sc2api/sc2_api.h"
typedef sc2::Point2D        CCPosition;
typedef sc2::Point2DI       CCTilePosition;
typedef sc2::Color          CCColor;
typedef sc2::UpgradeID      CCUpgrade;
typedef sc2::Tag            CCUnitID;
typedef sc2::Race           CCRace;
typedef float               CCHealth;
typedef float               CCPositionType;



typedef size_t CCPlayer;
int GetIntFromString(const std::string & s);