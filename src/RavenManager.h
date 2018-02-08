#pragma once
#pragma once

#include "Common.h"
#include "MicroManager.h"

class CCBot;

struct RavenInfo
{
	float m_hpLastSecond;

	RavenInfo();
	RavenInfo(float hp);
};

class RavenManager : public MicroManager
{
	std::unordered_map<const sc2::Unit*, RavenInfo> RavenInfos;
public:

	RavenManager(CCBot & bot);
	void    executeMicro(const std::vector<const sc2::Unit *> & targets);
	void    assignTargets(const std::vector<const sc2::Unit *> & targets);
	int     getAttackPriority(const sc2::Unit * rangedUnit, const sc2::Unit * target);
	const sc2::Unit * getTarget(const sc2::Unit * rangedUnit, const std::vector<const sc2::Unit *> & targets);
	sc2::Point2D RavenManager::RetreatPosition(const sc2::Unit * rangedUnit);
	sc2::Point2D RavenManager::GetTurretPosition(const sc2::Unit * rangedUnit);
};
