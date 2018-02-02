#pragma once

#include "Common.h"
#include "MicroManager.h"

class CCBot;

struct PhoenixInfo
{
	float m_hpLastSecond;

	PhoenixInfo();
	PhoenixInfo(float hp);
};

class PhoenixManager : public MicroManager
{
	std::unordered_map<const sc2::Unit*, PhoenixInfo> PhoenixInfos;
public:

	PhoenixManager(CCBot & bot);
	void    executeMicro(const std::vector<const sc2::Unit *> & targets);
	void    assignTargets(const std::vector<const sc2::Unit *> & targets);
	int     getAttackPriority(const sc2::Unit * rangedUnit, const sc2::Unit * target);
	const sc2::Unit * getTarget(const sc2::Unit * rangedUnit, const std::vector<const sc2::Unit *> & targets);
	sc2::Point2D PhoenixManager::RetreatPosition(const sc2::Unit * rangedUnit);
};
