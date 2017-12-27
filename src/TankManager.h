#pragma once

#include "Common.h"
#include "MicroManager.h"

class CCBot;

struct TankInfo
{
	float m_hpLastSecond;

	TankInfo();
	TankInfo(float hp);
};

class TankManager : public MicroManager
{
	std::unordered_map<const sc2::Unit*, TankInfo> tankInfos;
public:

	TankManager(CCBot & bot);
	void    executeMicro(const std::vector<const sc2::Unit *> & targets);
	void    assignTargets(const std::vector<const sc2::Unit *> & targets);
	int     getAttackPriority(const sc2::Unit * rangedUnit, const sc2::Unit * target);
	const sc2::Unit * getTarget(const sc2::Unit * rangedUnit, const std::vector<const sc2::Unit *> & targets);
};
