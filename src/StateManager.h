#pragma once

#include "Common.h"

class CCBot;

struct StateManager
{
	CCBot & m_bot;
	bool m_waitWarpGate;
	bool m_waitBlink;
	bool m_rallyAtPylon;
	bool m_startAttack;
	bool m_startBlink;
	bool m_keepTrainWorker;
	bool m_rschWarpGate;
	bool m_rschBlink;

public:
	StateManager(CCBot & bot);
	void clear();
	void onFrame();
	void OnUpgradeCompleted(sc2::UpgradeID upgradeID);

	bool shouldAttack();
};