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

int GetIntFromString(const std::string & s);