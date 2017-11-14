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
	, PullWorkers
	, PullWorkersLeaving
	, ReleaseWorkers
	, WaitWarpGate
	, RallyAtPylon
	, StartAttack
	, WaitBlink
	, StartBlink
	, KeepTrainWorker
	, ChronoBoost
	, GoOnPatrol
};

class MacroCommand
{
	MacroCommandType	_type;
	int                 _amount;
	sc2::UNIT_TYPEID	_target;
	sc2::Point2D		_position;

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
			, MacroCommandType::PullWorkers
			, MacroCommandType::PullWorkersLeaving
			, MacroCommandType::ReleaseWorkers
			, MacroCommandType::WaitWarpGate
			, MacroCommandType::RallyAtPylon
			, MacroCommandType::StartAttack
			, MacroCommandType::WaitBlink
			, MacroCommandType::StartBlink
			, MacroCommandType::KeepTrainWorker
			, MacroCommandType::ChronoBoost
			, MacroCommandType::GoOnPatrol
		};
	}

	// Default constructor for when the value doesn't matter.
	MacroCommand()
		: _type(MacroCommandType::None)
		, _amount(0)
		, _target(sc2::UNIT_TYPEID::INVALID)
		, _position(0, 0)
	{
	}

	MacroCommand(MacroCommandType type)
		: _type(type)
		, _amount(0)
		, _target(sc2::UNIT_TYPEID::INVALID)
		, _position(0, 0)
	{
		BOT_ASSERT(!hasArgument(type), "missing MacroCommand argument");
	}

	MacroCommand(MacroCommandType type, int amount)
		: _type(type)
		, _amount(amount)
		, _target(sc2::UNIT_TYPEID::INVALID)
		, _position(0, 0)
	{
		BOT_ASSERT(hasAmount(type), "extra MacroCommand argument");
	}

	MacroCommand(MacroCommandType type, sc2::UnitTypeID target)
		: _type(type)
		, _amount(0)
		, _target(target)
		, _position(0, 0)
	{
		BOT_ASSERT(hasTarget(type), "extra MacroCommand argument");
	}

	MacroCommand(MacroCommandType type, const sc2::Point2D & position)
		: _type(type)
		, _amount(0)
		, _target(sc2::UNIT_TYPEID::INVALID)
		, _position(position)
	{
		BOT_ASSERT(hasPosition(type), "extra MacroCommand argument");
	}

	const int getAmount() const
	{
		return _amount;
	}

	const sc2::UnitTypeID getTarget() const
	{
		return _target;
	}

	const sc2::Point2D getPosition() const
	{
		return _position;
	}

	const MacroCommandType & getType() const
	{
		return _type;
	}

	// The command has a numeric argument, the _amount.
	static const bool hasAmount(MacroCommandType t)
	{
		return
			t == MacroCommandType::GasUntil ||
			t == MacroCommandType::PullWorkers ||
			t == MacroCommandType::PullWorkersLeaving ||
			t == MacroCommandType::KeepTrainWorker;
	}

	static const bool hasTarget(MacroCommandType t)
	{
		return t == MacroCommandType::ChronoBoost;
	}

	static const bool hasPosition(MacroCommandType t)
	{
		return false;
	}

	static const bool hasArgument(MacroCommandType t)
	{
		return hasAmount(t) || hasTarget(t) || hasPosition(t);
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
		if (t == MacroCommandType::ChronoBoost)
		{
			return "go chrono boost";
		}
		if (t == MacroCommandType::GoOnPatrol)
		{
			return "go on patrol";
		}
		BOT_ASSERT(t == MacroCommandType::None, "unrecognized MacroCommandType");
		return "go none";
	}

	const std::string getName() const
	{
		if (hasAmount(_type))
		{
			// Include the amount.
			std::stringstream name;
			name << getName(_type) << " " << _amount;
			return name.str();
		}
		else if (hasTarget(_type))
		{
			std::stringstream name;
			std::string type = sc2::UnitTypeToName(_target);
			std::transform(type.begin(), type.end(), type.begin(), ::tolower);
			name << getName(_type) << " " << type;
			return name.str();
		}
		else if (hasPosition(_type))
		{
			std::stringstream name;
			name << getName(_type) << " " << _position.x << "," << _position.y;
		}
		else {
			return getName(_type);
		}
	}

};