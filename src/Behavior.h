#pragma once
#include<vector>
#include<string>
#include<iostream>
#include"Common.h"

namespace BT
{
	//��Ϊ������״̬
	enum class EStatus :uint8_t
	{
		Invalid,   //��ʼ״̬
		Success,   //�ɹ�
		Failure,   //ʧ��
		Running,   //����
		Aborted,   //��ֹ
	};

	//Parallel�ڵ�ɹ���ʧ�ܵ�Ҫ����ȫ���ɹ�/ʧ�ܣ�����һ���ɹ�/ʧ��
	enum class EPolicy :uint8_t
	{
		RequireOne,
		RequireAll,
	};

	class Behavior
	{
	public:
		//�ͷŶ�����ռ��Դ
		virtual void Release() = 0;
		//��װ��������ֹ���Ƶ�����Լ
		EStatus Tick();

		EStatus GetStatus() { return Status; }

		void Reset() { Status = EStatus::Invalid; }
		void Abort() { OnTerminate(EStatus::Aborted); Status = EStatus::Aborted; }

		bool IsTerminate() { return Status == EStatus::Success || Status == EStatus::Failure; }
		bool IsRunning() { return Status == EStatus::Running; }
		bool IsSuccess() { return Status == EStatus::Success; }
		bool IsFailuer() { return Status == EStatus::Failure; }

		virtual std::string Name() = 0;
		virtual void AddChild(Behavior* Child) {};

	protected:
		//�������������Create()�ͷŶ��������Release()
		Behavior() :Status(EStatus::Invalid) {}
		virtual ~Behavior() {}

		virtual void OnInitialize() {};
		virtual EStatus Update() = 0;
		virtual void OnTerminate(EStatus Status) {};

	protected:
		EStatus Status;
	};

	//װ����
	class Decorator :public Behavior
	{
	public:
		virtual void AddChild(Behavior* InChild) { Child = InChild; }
	protected:
		Decorator() {}
		virtual ~Decorator() {}
		Behavior* Child;
	};

	//�ظ�ִ���ӽڵ��װ����
	class Repeat :public Decorator
	{
	public:

		static Behavior* Create(int InLimited) { return new Repeat(InLimited); }
		virtual void Release() { Child->Release(); delete this; }
		virtual std::string Name() override { return "Repeat"; }

	protected:
		Repeat(int InLimited) :Limited(InLimited) {}
		virtual ~Repeat() {}
		virtual void OnInitialize() { Count = 0; }
		virtual EStatus Update()override;
		virtual Behavior* Create() { return nullptr; }
	protected:
		int Limited = 3;
		int Count = 0;
	};

	//���Ͻڵ����
	class Composite :public Behavior
	{
	public:
		virtual void AddChild(Behavior* InChild) override { Children.push_back(InChild); }
		void RemoveChild(Behavior* InChild);
		void ClearChild() { Children.clear(); }
		virtual void Release()
		{
			for (auto it : Children)
			{
				it->Release();
			}

			delete this;
		}

	protected:
		Composite() {}
		virtual ~Composite() {}
		using Behaviors = std::vector<Behavior*>;
		Behaviors Children;
	};

	//˳����������ִ�����нڵ�ֱ������һ��ʧ�ܻ���ȫ���ɹ�λ��
	class Sequence :public Composite
	{
	public:
		virtual std::string Name() override { return "Sequence"; }
		static Behavior* Create() { return new Sequence(); }
	protected:
		Sequence() {}
		virtual ~Sequence() {}
		virtual void OnInitialize() override { CurrChild = Children.begin(); }
		virtual EStatus Update() override;

	protected:
		Behaviors::iterator CurrChild;
	};

	//�����������ض������¾ܾ�ִ������Ϊ�Ľڵ㣬����ͨ������������ͷ��������˳������ʵ��
	class Filter :public Sequence
	{
	public:
		static Behavior* Create() { return new Filter(); }
		void AddCondition(Behavior* Condition) { Children.insert(Children.begin(), Condition); }
		void AddAction(Behavior* Action) { Children.push_back(Action); }
		virtual std::string Name() override { return "Fliter"; }

	protected:
		Filter() {}
		virtual ~Filter() {}
	};

	//ѡ����:����ִ��ÿ���ӽڵ�ֱ������һ��ִ�гɹ����߷���Running״̬
	class Selector :public Composite
	{
	public:
		static Behavior* Create() { return new Selector(); }
		virtual std::string Name() override { return "Selector"; }

	protected:
		Selector() {}
		virtual ~Selector() {}
		virtual void OnInitialize() override { CurrChild = Children.begin(); }
		virtual EStatus Update() override;

	protected:
		Behaviors::iterator CurrChild;
	};

	//�������������Ϊ����ִ��
	class Parallel :public Composite
	{
	public:
		static Behavior* Create(EPolicy InSucess, EPolicy InFailure) { return new Parallel(InSucess, InFailure); }
		virtual std::string Name() override { return "Parallel"; }

	protected:
		Parallel(EPolicy InSucess, EPolicy InFailure) :SucessPolicy(InSucess), FailurePolicy(InFailure) {}
		virtual ~Parallel() {}
		virtual EStatus Update() override;
		virtual void OnTerminate(EStatus InStatus) override;

	protected:
		EPolicy SucessPolicy;
		EPolicy FailurePolicy;
	};

	//������:������������������Ч�ԣ�һ�����������㣬��ʱ�˳���ǰ��Ϊ
	class Monitor :public Parallel
	{
	public:
		static Behavior* Create(EPolicy InSucess, EPolicy InFailure) { return new Monitor(InSucess, InFailure); }
		void AddCondition(Behavior* Condition) { Children.insert(Children.begin(), Condition); }
		void AddAction(Behavior* Action) { Children.push_back(Action); }
		virtual std::string Name() override { return "Monitor"; }

	protected:
		Monitor(EPolicy InSucess, EPolicy InFailure) :Parallel(InSucess, InFailure) {}
		virtual ~Monitor() {}
	};

	//����ѡ������ִ�й����в��ϼ������ȼ���Ϊ�Ŀ�����,�����ȼ���Ϊ���жϵ����ȼ���Ϊ
	class ActiveSelector :public Selector
	{
	public:
		static Behavior* Create() { return new ActiveSelector(); }
		//��ʼʱ�ѵ�ǰ�ڵ���Ϊend
		virtual void OnInitialize() override { CurrChild = Children.end(); }
		virtual std::string Name() override { return "ActiveSelector"; }
	protected:
		ActiveSelector() {}
		virtual ~ActiveSelector() {}
		virtual EStatus Update() override;
	};

	//��������
	class Condition :public Behavior
	{
	public:
		virtual void Release() { delete this; }

	protected:
		Condition(bool InIsNegation,const sc2::Unit * unit) :IsNegation(InIsNegation),unit(unit) {}
		virtual ~Condition() {}

	protected:
		//�Ƿ�ȡ��
		bool  IsNegation = false;
		const sc2::Unit * unit;
	};

	//��������
	class Action :public Behavior
	{
	public:
		virtual void Release() { delete this; }

	protected:
		Action() {}
		virtual ~Action() {}
	};

	//������������
	class Condition_IsSeeEnemy :public Condition
	{
	public:
		static Behavior* Create(bool InIsNegation, const sc2::Unit * unit) { return new Condition_IsSeeEnemy(InIsNegation,unit); }
		virtual std::string Name() override { return "Condtion_IsSeeEnemy"; }

	protected:
		Condition_IsSeeEnemy(bool InIsNegation, const sc2::Unit * unit) :Condition(InIsNegation,unit) {}
		virtual ~Condition_IsSeeEnemy() {}
		virtual EStatus Update() override;
	};

	//Ѫ��������
	class Condition_IsHealthLow :public Condition
	{
	public:
		static Behavior* Create(bool InIsNegation, const sc2::Unit * unit) { return new Condition_IsHealthLow(InIsNegation, unit); }
		virtual std::string Name() override { return "Condition_IsHealthLow"; }

	protected:
		Condition_IsHealthLow(bool InIsNegation,const sc2::Unit * unit) :Condition(InIsNegation,unit) {}
		virtual ~Condition_IsHealthLow() {}
		virtual EStatus Update() override;

	};

	// ������������
	class Condition_IsEnemyDead :public Condition
	{
	public:
		static Behavior* Create(bool InIsNegation, const sc2::Unit * unit) { return new Condition_IsEnemyDead(InIsNegation,unit); }
		virtual std::string Name() override { return "Condition_IsHealthLow"; }

	protected:
		Condition_IsEnemyDead(bool InIsNegation, const sc2::Unit * unit) :Condition(InIsNegation,unit) {}
		virtual ~Condition_IsEnemyDead() {}
		virtual EStatus Update() override;

	};

	//��������
	class Action_Attack :public Action
	{
	public:
		static Behavior* Create() { return new Action_Attack(); }
		virtual std::string Name() override { return "Action_Attack"; }

	protected:
		Action_Attack() {}
		virtual ~Action_Attack() {}
		virtual EStatus Update() override;
	};

	//���ܶ���
	class Action_Runaway :public Action
	{
	public:
		static Behavior* Create() { return new Action_Runaway(); }
		virtual std::string Name() override { return "Action_Runaway"; }

	protected:
		Action_Runaway() {}
		virtual ~Action_Runaway() {}
		virtual EStatus Update() override;
	};

	//Ѳ�߶���
	class Action_Patrol :public Action
	{
	public:
		static Behavior* Create() { return new Action_Patrol(); }
		virtual std::string Name() override { return "Action_Patrol"; }

	protected:
		Action_Patrol() {}
		virtual ~Action_Patrol() {}
		virtual EStatus Update() override;
	};
}


#pragma once
