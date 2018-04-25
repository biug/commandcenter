#pragma once

#include<stack>
#include "Behavior.h"
namespace BT
{
	enum class EActionMode
	{
		Attack,
		Patrol,
		Runaway,
	};

	enum class EConditionMode
	{
		IsSeeEnemy,
		IsHealthLow,
		IsEnemyDead,
	};

	class BehaviorTree
	{
	public:
		BehaviorTree(Behavior* InRoot) :Root(InRoot) {}
		void Tick();
		bool HaveRoot() { return Root ? true : false; }
		void SetRoot(Behavior* InNode) { Root = InNode; }
		void Release() { Root->Release(); }
	private:
		Behavior* Root;
	};

	//��Ϊ������������������һ����Ϊ��,ͨ��ǰ�������ʽ���Back()��End()�������й���
	class BehaviorTreeBuilder
	{
	public:
		BehaviorTreeBuilder() { }
		~BehaviorTreeBuilder() { }
		BehaviorTreeBuilder* Sequence();
		BehaviorTreeBuilder* Action(EActionMode ActionModes);
		BehaviorTreeBuilder* Condition(EConditionMode ConditionMode, bool IsNegation, const sc2::Unit * unit);
		BehaviorTreeBuilder* Selector();
		BehaviorTreeBuilder* Repeat(int RepeatNum);
		BehaviorTreeBuilder* ActiveSelector();
		BehaviorTreeBuilder* Filter();
		BehaviorTreeBuilder* Parallel(EPolicy InSucess, EPolicy InFailure);
		BehaviorTreeBuilder* Monitor(EPolicy InSucess, EPolicy InFailure);
		BehaviorTreeBuilder* Back();
		BehaviorTree* End();

	private:
		void AddBehavior(Behavior* NewBehavior);

	private:
		Behavior* TreeRoot = nullptr;
		//���ڴ洢�ڵ�Ķ�ջ
		std::stack<Behavior*> NodeStack;
	};
}