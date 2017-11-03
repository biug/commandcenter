#pragma once

#include "Common.h"

enum class MacroCommandType
{
	None
	, Scout
	, ScoutIfNeeded
	, ScoutLocation
	, StartGas
	, StopGas
	, GasUntil
	, StealGas
	, ExtractorTrickDrone
	, ExtractorTrickZergling
	, Aggressive
	, Defensive
	, PullWorkers
	, PullWorkersLeaving
	, ReleaseWorkers
	, UntilFindEnemy
	, WaitWarpGate
	, RallyAtPylon
	, StartAttack
	, WaitBlink
	, StartBlink
	, KeepTrainWorker
};

class MacroCommand
{
	MacroCommandType	_type;
	int                 _amount;

public:

	static const std::list<MacroCommandType> allCommandTypes()
	{
		return std::list<MacroCommandType>
		{ MacroCommandType::Scout
			, MacroCommandType::ScoutIfNeeded
			, MacroCommandType::ScoutLocation
			, MacroCommandType::StartGas
			, MacroCommandType::StopGas
			, MacroCommandType::GasUntil
			, MacroCommandType::StealGas
			, MacroCommandType::ExtractorTrickDrone
			, MacroCommandType::ExtractorTrickZergling
			, MacroCommandType::Aggressive
			, MacroCommandType::Defensive
			, MacroCommandType::PullWorkers
			, MacroCommandType::PullWorkersLeaving
			, MacroCommandType::ReleaseWorkers
			, MacroCommandType::UntilFindEnemy
			, MacroCommandType::WaitWarpGate
			, MacroCommandType::RallyAtPylon
			, MacroCommandType::StartAttack
			, MacroCommandType::WaitBlink
			, MacroCommandType::StartBlink
			, MacroCommandType::KeepTrainWorker
		};
	}

	// Default constructor for when the value doesn't matter.
	MacroCommand()
		: _type(MacroCommandType::None)
		, _amount(0)
	{
	}

	MacroCommand(MacroCommandType type)
		: _type(type)
		, _amount(0)
	{
		BOT_ASSERT(!hasArgument(type), "missing MacroCommand argument");
	}

	MacroCommand(MacroCommandType type, int amount)
		: _type(type)
		, _amount(amount)
	{
		BOT_ASSERT(hasArgument(type), "extra MacroCommand argument");
	}

	const int getAmount() const
	{
		return _amount;
	}

	const MacroCommandType & getType() const
	{
		return _type;
	}

	// The command has a numeric argument, the _amount.
	static const bool hasArgument(MacroCommandType t)
	{
		return
			t == MacroCommandType::GasUntil ||
			t == MacroCommandType::PullWorkers ||
			t == MacroCommandType::PullWorkersLeaving ||
			t == MacroCommandType::KeepTrainWorker;
	}

	static const std::string getName(MacroCommandType t)
	{
		if (t == MacroCommandType::Scout)
		{
			return "go scout";
		}
		if (t == MacroCommandType::ScoutIfNeeded)
		{
			return "go scout if needed";
		}
		if (t == MacroCommandType::ScoutLocation)
		{
			return "go scout location";
		}
		if (t == MacroCommandType::StartGas)
		{
			return "go start gas";
		}
		if (t == MacroCommandType::StopGas)
		{
			return "go stop gas";
		}
		if (t == MacroCommandType::GasUntil)
		{
			return "go gas until";
		}
		if (t == MacroCommandType::StealGas)
		{
			return "go steal gas";
		}
		if (t == MacroCommandType::ExtractorTrickDrone)
		{
			return "go extractor trick drone";
		}
		if (t == MacroCommandType::ExtractorTrickZergling)
		{
			return "go extractor trick zergling";
		}
		if (t == MacroCommandType::Aggressive)
		{
			return "go aggressive";
		}
		if (t == MacroCommandType::Defensive)
		{
			return "go defensive";
		}
		if (t == MacroCommandType::PullWorkers)
		{
			return "go pull workers";
		}
		if (t == MacroCommandType::PullWorkersLeaving)
		{
			return "go pull workers leaving";
		}
		if (t == MacroCommandType::ReleaseWorkers)
		{
			return "go release workers";
		}
		if (t == MacroCommandType::UntilFindEnemy)
		{
			return "go until find enemy";
		}
		if (t == MacroCommandType::WaitWarpGate)
		{
			return "go wait warpgate";
		}
		if (t == MacroCommandType::RallyAtPylon)
		{
			return "go rally at pylon";
		}
		if (t == MacroCommandType::StartAttack)
		{
			return "go start attack";
		}
		if (t == MacroCommandType::WaitBlink)
		{
			return "go wait blink";
		}
		if (t == MacroCommandType::StartBlink)
		{
			return "go start blink";
		}
		if (t == MacroCommandType::KeepTrainWorker)
		{
			return "go keep train worker";
		}
		BOT_ASSERT(t == MacroCommandType::None, "unrecognized MacroCommandType");
		return "go none";
	}

	const std::string getName() const
	{
		if (hasArgument(_type))
		{
			// Include the amount.
			std::stringstream name;
			name << getName(_type) << " " << _amount;
			return name.str();
		}
		else {
			return getName(_type);
		}
	}

};