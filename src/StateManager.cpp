#include "StateManager.h"

StateManager::StateManager(CCBot & bot)
	: m_bot(bot)
{
	clear();
}

void StateManager::clear()
{
	m_waitWarpGate = false;
	m_rallyAtPylon = false;
	m_startAttack = false;

	m_rschWarpGate = false;
	m_rschBlink = false;
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
	default:
		break;
	}
}