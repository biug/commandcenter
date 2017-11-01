#pragma once

#include "Common.h"
#include "MacroAct.h"

class BuildOrder
{
    sc2::Race m_race;
    std::vector<MacroAct> m_buildOrder;

public:

    BuildOrder();
    BuildOrder(const sc2::Race & race);
    BuildOrder(const sc2::Race & race, const std::vector<MacroAct> & buildVector);

    void add(const MacroAct & type);
    size_t size() const;
    const sc2::Race & getRace() const;
    const MacroAct & operator [] (const size_t & index) const;
	MacroAct & operator [] (const size_t & index);
};

