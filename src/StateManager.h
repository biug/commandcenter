#pragma once

#include "Common.h"

class CCBot;

struct StateManager
{
	CCBot & m_bot;
	bool m_waitWarpGate;

public:
	StateManager(CCBot & bot);
	void clear();
	void onFrame();
};