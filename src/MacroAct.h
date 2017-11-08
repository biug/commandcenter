#pragma once

#include "BuildType.h"
#include "MacroCommand.h"

class CCBot;

enum class MacroLocation
{
	Anywhere     // default location
	, Macro        // macro hatchery
	, Expo         // gas expansion hatchery
	, MinOnly      // any expansion hatchery (mineral-only or gas, whatever's next)
	, Hidden       // gas expansion hatchery far from both main bases
	, Main         // starting base
	, Natural      // "natural" first expansion base
	, Third
	, EnemyNatural // enemy natural base
	, EnemyThird
};

namespace MacroActs
{
	enum { Unit, Upgrade, Command, Default };
}

class MacroAct
{
	size_t				_type;

	BuildType			_buildType;

	MacroCommand		_macroCommandType;

	MacroLocation		_macroLocation;

	MacroLocation		getMacroLocationFromString(std::string & s);

public:

	MacroAct();
	MacroAct(const std::string & name, CCBot & bot);
	MacroAct(BuildType t);
	MacroAct(sc2::UnitTypeID t);
	MacroAct(sc2::UnitTypeID t, MacroLocation loc);
	MacroAct(sc2::UpgradeID t);
	MacroAct(MacroCommandType t);
	MacroAct(MacroCommandType t, int amount);
	MacroAct(MacroCommandType t, sc2::UnitTypeID target);
	MacroAct(MacroCommandType t, sc2::Point2D position);

	bool    isUnit()		const;
	bool    isUpgrade()	    const;
	bool    isCommand()	    const;
	bool    isBuilding(CCBot & bot)	const;
	bool    isRefinery(CCBot & bot)	const;
	bool	isSupply()		const;

	const size_t & type() const;
	const sc2::Race & getRace() const;

	const sc2::UnitTypeID & getUnitType() const;
	const sc2::UpgradeID & getUpgradeType() const;
	const MacroCommand getCommandType() const;
	const sc2::Point2D getMacroLocation(CCBot & bot) const;

	int supplyRequired(CCBot & bot) const;
	int mineralPrice(CCBot & bot)   const;
	int gasPrice(CCBot & bot)       const;

	const std::vector<sc2::UnitTypeID> & whatBuilds(CCBot & bot) const;
	std::string getName() const;
};