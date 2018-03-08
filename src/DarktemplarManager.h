#pragma once
#pragma once
#pragma once

#include "Common.h"
#include "MicroManager.h"

class CCBot;

struct DarktemplarInfo
{
	float m_hpLastSecond;

	DarktemplarInfo();
	DarktemplarInfo(float hp);
};

class DarktemplarManager : public MicroManager
{
	std::unordered_map<const sc2::Unit*, DarktemplarInfo> DarktemplarInfos;
public:

	DarktemplarManager(CCBot & bot);
	void    executeMicro(const std::vector<const sc2::Unit *> & targets);
	void    assignTargets(const std::vector<const sc2::Unit *> & targets);
	int     getAttackPriority(const sc2::Unit * rangedUnit, const sc2::Unit * target);
	const sc2::Unit * getTarget(const sc2::Unit * rangedUnit, const std::vector<const sc2::Unit *> & targets);
	const sc2::Unit * getRallyUnit(const sc2::Unit * rangedUnit, const std::vector<const sc2::Unit *> & targets);
};
