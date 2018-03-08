#pragma once
#pragma once

#include "Common.h"
#include "MicroManager.h"

class CCBot;

struct HightemplarInfo
{
	float m_hpLastSecond;

	HightemplarInfo();
	HightemplarInfo(float hp);
};

struct MostTargetPosition
{
	int maxtargetnum;
	sc2::Point2D hasmosttargetposition;
};

class HightemplarManager : public MicroManager
{
	std::unordered_map<const sc2::Unit*, HightemplarInfo> HightemplarInfos;
public:

	HightemplarManager(CCBot & bot);
	void    executeMicro(const std::vector<const sc2::Unit *> & targets);
	void    assignTargets(const std::vector<const sc2::Unit *> & targets);
	int     getAttackPriority(const sc2::Unit * rangedUnit, const sc2::Unit * target);
	const sc2::Unit * getTarget(const sc2::Unit * rangedUnit, const std::vector<const sc2::Unit *> & targets);
	MostTargetPosition HightemplarManager::SetPsistormPosition(const sc2::Unit * rangedUnit, const std::vector<const sc2::Unit *> & targets);
	const sc2::Unit * getRallyUnit(const sc2::Unit * rangedUnit, const std::vector<const sc2::Unit *> & targets);
};
