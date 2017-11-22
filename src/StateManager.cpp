#include "StateManager.h"

StateManager::StateManager(CCBot & bot)
	: m_bot(bot)
{
	clear();
}

void StateManager::clear()
{
	m_waitWarpGate = false;
	m_waitBlink = false;
	m_rallyAtPylon = false;
	m_startAttack = false;
	m_startBlink = false;
	m_keepTrainWorker = false;
	m_rschWarpGate = false;
	m_rschBlink = false;
	m_patrol = false;
	m_scout = false;
	m_numKeepTrainWorker = 0;
	m_stimpack = false;
	m_chronoTarget = sc2::UNIT_TYPEID::INVALID;
}

void StateManager::onFrame()
{
}

void StateManager::OnUpgradeCompleted(sc2::UpgradeID upgradeID)
{
	switch (upgradeID.ToType()) {
	case sc2::UPGRADE_ID::WARPGATERESEARCH: {
		m_rschWarpGate = true;
	}
	case sc2::UPGRADE_ID::BLINKTECH: {
		m_rschBlink = true;
	}
	case sc2::UPGRADE_ID::STIMPACK: {
		m_stimpack = true;
	}
	default:
		break;
	}
}

bool StateManager::shouldAttack()
{
	return m_startAttack && (!m_waitBlink || m_rschBlink);
}