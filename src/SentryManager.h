#pragma once

#include "Common.h"
#include "MicroManager.h"

class CCBot;

struct SentryInfo
{
	float m_hpLastSecond;

	SentryInfo();
	SentryInfo(float hp);
};

class SentryManager : public MicroManager
{
	std::unordered_map<const sc2::Unit*, SentryInfo> SentryInfos;
public:

	SentryManager(CCBot & bot);
	void    executeMicro(const std::vector<const sc2::Unit *> & targets);
	void    assignTargets(const std::vector<const sc2::Unit *> & targets);
	int     getAttackPriority(const sc2::Unit * rangedUnit, const sc2::Unit * target);
	const sc2::Unit * getTarget(const sc2::Unit * rangedUnit, const std::vector<const sc2::Unit *> & targets);
	sc2::Point2D SentryManager::SetForcefieldPosition(const sc2::Unit * rangedUnit, const std::vector<const sc2::Unit *> & targets);
};
