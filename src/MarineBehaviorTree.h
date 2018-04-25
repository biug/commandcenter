#pragma once
#include "BehaviorTree.h"
#include "common.h"
using namespace BT;

class MarineBehaviorTree
{
	const sc2::Unit * marine;
	BehaviorTree * BT;
public:
	MarineBehaviorTree(const sc2::Unit * marine):marine(marine) {
		BehaviorTreeBuilder * Builder = new BehaviorTreeBuilder();
		BT = Builder
			->ActiveSelector()
			->Sequence()
			->Condition(BT::EConditionMode::IsSeeEnemy, false,marine)
			->Back()
			->ActiveSelector()
			->Sequence()
			->Condition(BT::EConditionMode::IsHealthLow, false,marine)
			->Back()
			->Action(BT::EActionMode::Runaway)
			->Back()
			->Back()
			->Parallel(BT::EPolicy::RequireAll, BT::EPolicy::RequireOne)
			->Condition(BT::EConditionMode::IsEnemyDead, true,marine)
			->Back()
			->Action(BT::EActionMode::Attack)
			->Back()
			->Back()
			->Back()
			->Back()
			->Action(BT::EActionMode::Patrol)
			->End();
		delete Builder;
	}
	void Tick()
	{
		BT->Tick();
	}
};
