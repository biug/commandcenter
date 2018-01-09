#include "UnitType.h"
#include "CCBot.h"

UnitType::UnitType()
    : m_bot(nullptr)
    , m_type(0)
{

}


UnitType::UnitType(const sc2::UnitTypeID & type, CCBot & bot)
    : m_bot(&bot)
    , m_type(type)
{
    
}

sc2::UnitTypeID UnitType::getAPIUnitType() const
{
    return m_type;
}

bool UnitType::is(const sc2::UnitTypeID & type) const
{
    return m_type == type;
}


bool UnitType::operator < (const UnitType & rhs) const
{
    return m_type < rhs.m_type;
}

bool UnitType::operator == (const UnitType & rhs) const
{
    return m_type == rhs.m_type;
}

bool UnitType::isValid() const
{
    return m_type != 0;
}

std::string UnitType::getName() const
{
    return sc2::UnitTypeToName(m_type);
}

CCRace UnitType::getRace() const
{

    return m_bot->Observation()->GetUnitTypeData()[m_type].race;

}

bool UnitType::isCombatUnit() const
{

    if (isWorker()) { return false; }
    if (isSupplyProvider()) { return false; }
    if (isBuilding()) { return false; }

    if (isEgg() || isLarva()) { return false; }

    return true;

}

bool UnitType::isSupplyProvider() const
{
    return (supplyProvided() < 0) && !isResourceDepot();
}

bool UnitType::isResourceDepot() const
{

    switch (m_type.ToType()) 
    {
        case sc2::UNIT_TYPEID::ZERG_HATCHERY                : return true;
        case sc2::UNIT_TYPEID::ZERG_LAIR                    : return true;
        case sc2::UNIT_TYPEID::ZERG_HIVE                    : return true;
        case sc2::UNIT_TYPEID::TERRAN_COMMANDCENTER         : return true;
        case sc2::UNIT_TYPEID::TERRAN_ORBITALCOMMAND        : return true;
        case sc2::UNIT_TYPEID::TERRAN_ORBITALCOMMANDFLYING  : return true;
        case sc2::UNIT_TYPEID::TERRAN_PLANETARYFORTRESS     : return true;
        case sc2::UNIT_TYPEID::PROTOSS_NEXUS                : return true;
        default: return false;
    }

}

bool UnitType::isRefinery() const
{

    switch (m_type.ToType()) 
    {
        case sc2::UNIT_TYPEID::TERRAN_REFINERY      : return true;
        case sc2::UNIT_TYPEID::PROTOSS_ASSIMILATOR  : return true;
        case sc2::UNIT_TYPEID::ZERG_EXTRACTOR       : return true;
        default: return false;
    }

}

bool UnitType::isDetector() const
{

    switch (m_type.ToType())
    {
        case sc2::UNIT_TYPEID::PROTOSS_OBSERVER        : return true;
        case sc2::UNIT_TYPEID::ZERG_OVERSEER           : return true;
        case sc2::UNIT_TYPEID::TERRAN_MISSILETURRET    : return true;
        case sc2::UNIT_TYPEID::ZERG_SPORECRAWLER       : return true;
        case sc2::UNIT_TYPEID::PROTOSS_PHOTONCANNON    : return true;
        case sc2::UNIT_TYPEID::TERRAN_RAVEN            : return true;
        default: return false;
    }

}

bool UnitType::isGeyser() const
{

    switch (m_type.ToType()) 
    {
        case sc2::UNIT_TYPEID::NEUTRAL_VESPENEGEYSER        : return true;
        case sc2::UNIT_TYPEID::NEUTRAL_PROTOSSVESPENEGEYSER : return true;
        case sc2::UNIT_TYPEID::NEUTRAL_SPACEPLATFORMGEYSER  : return true;
        default: return false;
    }

}

bool UnitType::isMineral() const
{
    switch (m_type.ToType()) 
    {
        case sc2::UNIT_TYPEID::NEUTRAL_MINERALFIELD         : return true;
        case sc2::UNIT_TYPEID::NEUTRAL_MINERALFIELD750      : return true;
        case sc2::UNIT_TYPEID::NEUTRAL_RICHMINERALFIELD     : return true;
        case sc2::UNIT_TYPEID::NEUTRAL_RICHMINERALFIELD750  : return true;
        default: return false;
    }

}

bool UnitType::isWorker() const
{

    switch (m_type.ToType()) 
    {
        case sc2::UNIT_TYPEID::TERRAN_SCV           : return true;
        case sc2::UNIT_TYPEID::PROTOSS_PROBE        : return true;
        case sc2::UNIT_TYPEID::ZERG_DRONE           : return true;
        case sc2::UNIT_TYPEID::ZERG_DRONEBURROWED   : return true;
        default: return false;
    }

}

CCPositionType UnitType::getAttackRange() const
{

    auto & weapons = m_bot->Observation()->GetUnitTypeData()[m_type].weapons;
    
    if (weapons.empty())
    {
        return 0.0f;
    }

    float maxRange = 0.0f;
    for (auto & weapon : weapons)
    {
        if (weapon.range > maxRange)
        {
            maxRange = weapon.range;
        }
    }

    return maxRange;

}
/*
int UnitType::tileWidth() const
{

    if (isMineral()) { return 2; }
    if (isGeyser()) { return 3; }
    else { return (int)(2 * m_bot->Observation()->GetAbilityData()[m_bot->Data(*this).buildAbility].footprint_radius); }

}

int UnitType::tileHeight() const
{

    if (isMineral()) { return 1; }
    if (isGeyser()) { return 3; }
    else { return (int)(2 * m_bot->Observation()->GetAbilityData()[m_bot->Data(*this).buildAbility].footprint_radius); }

}

bool UnitType::isAddon() const
{
	m_bot->Data(*this->getAPIUnitType())
    return m_bot.Data(*this->getAPIUnitType()).isAddon;

}
*/

bool UnitType::isBuilding() const
{

    return m_bot->Data(*this->getAPIUnitType()).isBuilding;

}
int UnitType::supplyProvided() const
{

    return (int)m_bot->Observation()->GetUnitTypeData()[m_type].food_provided;

}

int UnitType::supplyRequired() const
{

    return (int)m_bot->Observation()->GetUnitTypeData()[m_type].food_required;

}

int UnitType::mineralPrice() const
{

    return (int)m_bot->Observation()->GetUnitTypeData()[m_type].mineral_cost;

}

int UnitType::gasPrice() const
{

    return (int)m_bot->Observation()->GetUnitTypeData()[m_type].vespene_cost;

}

UnitType UnitType::GetUnitTypeFromName(const std::string & name, CCBot & bot)
{

    for (const sc2::UnitTypeData & data : bot.Observation()->GetUnitTypeData())
    {
        if (name == data.name)
        {
            return UnitType(data.unit_type_id, bot);
        }
    }


    return UnitType();
}

bool UnitType::isOverlord() const
{

    return m_type == sc2::UNIT_TYPEID::ZERG_OVERLORD;

}

bool UnitType::isLarva() const
{

    return m_type == sc2::UNIT_TYPEID::ZERG_LARVA;

}

bool UnitType::isEgg() const
{

    return m_type == sc2::UNIT_TYPEID::ZERG_EGG;

}

bool UnitType::isQueen() const
{

    return m_type == sc2::UNIT_TYPEID::ZERG_QUEEN;

}

bool UnitType::isTank() const
{

    return m_type == sc2::UNIT_TYPEID::TERRAN_SIEGETANK || m_type == sc2::UNIT_TYPEID::TERRAN_SIEGETANKSIEGED;

}

bool UnitType::isMorphedBuilding() const
{

    
    switch (m_type.ToType())
    {
        case sc2::UNIT_TYPEID::ZERG_LAIR:                   { return true;  }
        case sc2::UNIT_TYPEID::ZERG_HIVE:                   { return true;  }
        case sc2::UNIT_TYPEID::ZERG_GREATERSPIRE:           { return true;  }
        case sc2::UNIT_TYPEID::TERRAN_PLANETARYFORTRESS:    { return true;  }
        case sc2::UNIT_TYPEID::TERRAN_ORBITALCOMMAND:       { return true;  }
        default:                                            { return false; }                                                            
    }

}
