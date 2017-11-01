#include "BuildOrder.h"

BuildOrder::BuildOrder()
    : m_race(sc2::Race::Protoss)
{

}

BuildOrder::BuildOrder(const sc2::Race & race)
    : m_race(race)
{

}

BuildOrder::BuildOrder(const sc2::Race & race, const std::vector<MacroAct> & vec)
    : m_race(race)
    , m_buildOrder(vec)
{

}

void BuildOrder::add(const MacroAct & type)
{
    m_buildOrder.push_back(type);
}

const sc2::Race & BuildOrder::getRace() const
{
    return m_race;
}

size_t BuildOrder::size() const
{
    return m_buildOrder.size();
}

const MacroAct & BuildOrder::operator [] (const size_t & index) const
{
    return m_buildOrder[index];
}

MacroAct & BuildOrder::operator [] (const size_t & index)
{
    return m_buildOrder[index];
}