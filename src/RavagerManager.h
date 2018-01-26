#pragma once

#include "Common.h"
#include "MicroManager.h"

class CCBot;

struct RavagerInfo
{
	float m_hpLastSecond;

	RavagerInfo();
	RavagerInfo(float hp);
};

class RavagerManager : public MicroManager
{
	std::unordered_map<const sc2::Unit*, RavagerInfo> RavagerInfos;
public:

	RavagerManager(CCBot & bot);
	void    executeMicro(const std::vector<const sc2::Unit *> & targets);
	void    assignTargets(const std::vector<const sc2::Unit *> & targets);
	int     getAttackPriority(const sc2::Unit * rangedUnit, const sc2::Unit * target);
	const sc2::Unit * getTarget(const sc2::Unit * rangedUnit, const std::vector<const sc2::Unit *> & targets);
};
