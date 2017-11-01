#include "StateManager.h"

StateManager::StateManager(CCBot & bot)
	: m_bot(bot)
{
	clear();
}

void StateManager::clear()
{
	m_waitWarpGate = false;
}

void StateManager::onFrame()
{
	clear();
}