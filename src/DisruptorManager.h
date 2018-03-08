#pragma once

#include "Common.h"
#include "MicroManager.h"

class CCBot;

struct DisruptorInfo
{
	float m_hpLastSecond;

	DisruptorInfo();
	DisruptorInfo(float hp);
};

class DisruptorManager : public MicroManager
{
	std::unordered_map<const sc2::Unit*, DisruptorInfo> DisruptorInfos;
public:

	DisruptorManager(CCBot & bot);
	void    executeMicro(const std::vector<const sc2::Unit *> & targets);
	void    assignTargets(const std::vector<const sc2::Unit *> & targets);
	int     getAttackPriority(const sc2::Unit * rangedUnit, const sc2::Unit * target);
	const sc2::Unit * getTarget(const sc2::Unit * rangedUnit, const std::vector<const sc2::Unit *> & targets);
	sc2::Point2D DisruptorManager::SetPurificationnovaPosition(const sc2::Unit * rangedUnit, const std::vector<const sc2::Unit *> & targets);
};