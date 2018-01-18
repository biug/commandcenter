#pragma once

#include "Common.h"
#include "MicroManager.h"

class CCBot;

struct MedivacInfo
{
	float m_hpLastSecond;

	MedivacInfo();
	MedivacInfo(float hp);
};

class MedivacManager : public MicroManager
{
	std::unordered_map<const sc2::Unit*, MedivacInfo> MedivacInfos;
public:

	MedivacManager(CCBot & bot);
	void    executeMicro(const std::vector<const sc2::Unit *> & targets);
	//void    assignTargets(const std::vector<const sc2::Unit *> & targets);
	//int     getAttackPriority(const sc2::Unit * rangedUnit, const sc2::Unit * target);
	const sc2::Unit * getTarget(const sc2::Unit * rangedUnit, const std::vector<const sc2::Unit *> & targets);
	bool MedivacManager::ShouldRetreat(const sc2::Unit * Unit);
	bool MedivacManager::loadUnit(const sc2::Unit * Unit, const sc2::Unit * Medivac);
	bool MedivacManager::unloadUnit(const sc2::Unit * Unit, const sc2::Unit * Medivac);

};
