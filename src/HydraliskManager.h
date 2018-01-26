#pragma once

#include "Common.h"
#include "MicroManager.h"

class CCBot;

struct HydraliskInfo
{
	float m_hpLastSecond;

	HydraliskInfo();
	HydraliskInfo(float hp);
};

class HydraliskManager : public MicroManager
{
	std::unordered_map<const sc2::Unit*, HydraliskInfo> HydraliskInfos;
public:

	HydraliskManager(CCBot & bot);
	void    executeMicro(const std::vector<const sc2::Unit *> & targets);
	void    assignTargets(const std::vector<const sc2::Unit *> & targets);
	int     getAttackPriority(const sc2::Unit * rangedUnit, const sc2::Unit * target);
	const sc2::Unit * getTarget(const sc2::Unit * rangedUnit, const std::vector<const sc2::Unit *> & targets);
};
