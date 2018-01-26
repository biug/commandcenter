#pragma once

#include "Common.h"
#include "MicroManager.h"

class CCBot;

struct VikingInfo
{
	float m_hpLastSecond;

	VikingInfo();
	VikingInfo(float hp);
};

class VikingManager : public MicroManager
{
	std::unordered_map<const sc2::Unit*, VikingInfo> VikingInfos;
public:

	VikingManager(CCBot & bot);
	void    executeMicro(const std::vector<const sc2::Unit *> & targets);
	void    assignTargets(const std::vector<const sc2::Unit *> & targets);
	bool	ShouldRetreat(const sc2::Unit * Unit);
	int     getAttackPriority(const sc2::Unit * rangedUnit, const sc2::Unit * target);
	const sc2::Unit * getTarget(const sc2::Unit * rangedUnit, const std::vector<const sc2::Unit *> & targets);
};
#pragma once
