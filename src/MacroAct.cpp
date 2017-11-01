#include "MacroAct.h"
#include "CCBot.h"

#include <regex>

MacroLocation MacroAct::getMacroLocationFromString(std::string & s)
{
	if (s == "macro")
	{
		return MacroLocation::Macro;
	}
	if (s == "expo")
	{
		return MacroLocation::Expo;
	}
	if (s == "min only")
	{
		return MacroLocation::MinOnly;
	}
	if (s == "hidden")
	{
		return MacroLocation::Hidden;
	}
	if (s == "main")
	{
		return MacroLocation::Main;
	}
	if (s == "natural")
	{
		return MacroLocation::Natural;
	}
	if (s == "third")
	{
		return MacroLocation::Third;
	}
	if (s == "enemy natural")
	{
		return MacroLocation::EnemyNatural;
	}
	if (s == "enemy third")
	{
		return MacroLocation::EnemyThird;
	}

	BOT_ASSERT(false, "config file - bad location '%s' after @", s.c_str());

	return MacroLocation::Anywhere;
}

MacroAct::MacroAct()
	: _type(MacroActs::Default)
	, _macroLocation(MacroLocation::Anywhere)
{
}

// Create a MacroAct from its name, like "drone" or "hatchery @ minonly".
// String comparison here is case-insensitive.
MacroAct::MacroAct(const std::string & name, CCBot & bot)
	: _type(MacroActs::Default)
	, _macroLocation(MacroLocation::Anywhere)
{
	std::string inputName(name);
	std::replace(inputName.begin(), inputName.end(), '_', ' ');
	std::transform(inputName.begin(), inputName.end(), inputName.begin(), ::tolower);

	// Commands like "go gas until 100". 100 is the amount.
	if (inputName.substr(0, 3) == std::string("go "))
	{
		for (const MacroCommandType t : MacroCommand::allCommandTypes())
		{
			std::string commandName = MacroCommand::getName(t);
			if (MacroCommand::hasArgument(t))
			{
				// There's an argument. Match the command name and parse out the argument.
				std::regex commandWithArgRegex(commandName + " (\\d+)");
				std::smatch m;
				if (std::regex_match(inputName, m, commandWithArgRegex)) {
					int amount = GetIntFromString(m[1].str());
					if (amount >= 0) {
						*this = MacroAct(t, amount);
						_type = MacroActs::Command;
						return;
					}
				}
			}
			else
			{
				// No argument. Just compare for equality.
				if (commandName == inputName)
				{
					*this = MacroAct(t);
					_type = MacroActs::Command;
					return;
				}
			}
		}
	}

	MacroLocation specifiedMacroLocation(MacroLocation::Anywhere);    // the default

																	  // Buildings can specify a location, like "hatchery @ expo".
																	  // It's meaningless and ignored for anything except a building.
																	  // Here we parse out the building and its location.
																	  // Since buildings are units, only UnitType below sets _macroLocation.

	std::regex macroLocationRegex("([a-z_ ]+[a-z])\\s+\\@\\s+([a-z][a-z ]+)");
	std::smatch m;
	if (std::regex_match(inputName, m, macroLocationRegex)) {
		specifiedMacroLocation = getMacroLocationFromString(m[2].str());
		// Don't change inputName before using the results from the regex.
		// Fix via gnuborg, who credited it to jaj22.
		inputName = m[1].str();
	}

	_buildType = BuildType(inputName, bot);
	_type = _buildType.isUnit() ? MacroActs::Unit : MacroActs::Upgrade;
	_macroLocation = specifiedMacroLocation;
}

MacroAct::MacroAct(BuildType t)
	: _buildType(t)
	, _type(t.isUnit() ? MacroActs::Unit : MacroActs::Upgrade)
	, _macroLocation(MacroLocation::Anywhere)
{
}

MacroAct::MacroAct(sc2::UnitTypeID t)
	: _buildType(t)
	, _type(MacroActs::Unit)
	, _macroLocation(MacroLocation::Anywhere)
{
}

MacroAct::MacroAct(sc2::UnitTypeID t, MacroLocation loc)
	: _buildType(t)
	, _type(MacroActs::Unit)
	, _macroLocation(loc)
{
}

MacroAct::MacroAct(sc2::UpgradeID t)
	: _buildType(t)
	, _type(MacroActs::Upgrade)
	, _macroLocation(MacroLocation::Anywhere)
{
}

MacroAct::MacroAct(MacroCommandType t)
	: _macroCommandType(t)
	, _type(MacroActs::Command)
	, _macroLocation(MacroLocation::Anywhere)
{
}

MacroAct::MacroAct(MacroCommandType t, int amount)
	: _macroCommandType(t, amount)
	, _type(MacroActs::Command)
	, _macroLocation(MacroLocation::Anywhere)
{
}

const size_t & MacroAct::type() const
{
	return _type;
}

bool MacroAct::isUnit() const
{
	return _type == MacroActs::Unit;
}

bool MacroAct::isUpgrade() const
{
	return _type == MacroActs::Upgrade;
}

bool MacroAct::isCommand() const
{
	return _type == MacroActs::Command;
}

const sc2::Race & MacroAct::getRace() const
{
	return _buildType.getRace();
}

bool MacroAct::isBuilding(CCBot & bot) const
{
	return _type == MacroActs::Unit && bot.Data(_buildType.getUnitTypeID()).isBuilding;
}

bool MacroAct::isRefinery(CCBot & bot)	const
{
	return _type == MacroActs::Unit && bot.Data(_buildType.getUnitTypeID()).isRefinery;
}

// The standard supply unit, ignoring the hatchery (which provides 1 supply).
bool MacroAct::isSupply() const
{
	return isUnit() &&
		(_buildType.getUnitTypeID() == sc2::UNIT_TYPEID::TERRAN_SUPPLYDEPOT
			|| _buildType.getUnitTypeID() == sc2::UNIT_TYPEID::PROTOSS_PYLON
			|| _buildType.getUnitTypeID() == sc2::UNIT_TYPEID::ZERG_OVERLORD);
}

const sc2::UnitTypeID & MacroAct::getUnitType() const
{
	BOT_ASSERT(_type == MacroActs::Unit, "getUnitType of non-unit");
	return _buildType.getUnitTypeID();
}

const sc2::UpgradeID & MacroAct::getUpgradeType() const
{
	BOT_ASSERT(_type == MacroActs::Upgrade, "getUpgradeType of non-upgrade");
	return _buildType.getUpgradeID();
}

const MacroCommand MacroAct::getCommandType() const
{
	BOT_ASSERT(_type == MacroActs::Command, "getCommandType of non-command");
	return _macroCommandType;
}

const sc2::Point2D MacroAct::getMacroLocation(CCBot & bot) const
{
	if (_macroLocation == MacroLocation::Natural) {
		auto loc = bot.Bases().getPlayerNaturalLocation(Players::Self);
		if (loc) return loc->getDepotPosition();
	}
	else if (_macroLocation == MacroLocation::Third) {
		auto loc = bot.Bases().getPlayerThirdLocation(Players::Self);
		if (loc) return loc->getDepotPosition();
	}
	else if (_macroLocation == MacroLocation::EnemyNatural) {
		auto loc = bot.Bases().getPlayerNaturalLocation(Players::Enemy);
		if (loc) return loc->getDepotPosition();
	}
	else if (_macroLocation == MacroLocation::EnemyThird) {
		auto loc = bot.Bases().getPlayerThirdLocation(Players::Enemy);
		if (loc) return loc->getDepotPosition();
	}
	return sc2::Point2D(0, 0);
}

int MacroAct::supplyRequired(CCBot & bot) const
{
	if (isUnit())
	{
		return bot.Data(_buildType.getUnitTypeID()).supplyCost;
	}
	return 0;
}

int MacroAct::mineralPrice(CCBot & bot) const
{
	if (isCommand()) {
		if (_macroCommandType.getType() == MacroCommandType::ExtractorTrickDrone ||
			_macroCommandType.getType() == MacroCommandType::ExtractorTrickZergling) {
			// 50 for the extractor and 50 for the unit. Never mind that you get some back.
			return 100;
		}
		return 0;
	}
	return isUnit() ?
		bot.Data(_buildType.getUnitTypeID()).mineralCost :
		bot.Data(_buildType.getUpgradeID()).mineralCost;
}

int MacroAct::gasPrice(CCBot & bot) const
{
	if (isCommand()) {
		return 0;
	}
	return isUnit() ?
		bot.Data(_buildType.getUnitTypeID()).gasCost :
		bot.Data(_buildType.getUpgradeID()).gasCost;
}

const std::vector<sc2::UnitTypeID> & MacroAct::whatBuilds(CCBot & bot) const
{
	if (isCommand()) {
		return {sc2::UNIT_TYPEID::INVALID};
	}
	return isUnit() ?
		bot.Data(_buildType.getUnitTypeID()).whatBuilds :
		bot.Data(_buildType.getUpgradeID()).whatBuilds;
}

std::string MacroAct::getName() const
{
	if (isUnit())
	{
		return sc2::UnitTypeToName(_buildType.getUnitTypeID());
	}
	if (isUpgrade())
	{
		return sc2::UpgradeIDToName(_buildType.getUpgradeID());
	}
	if (isCommand())
	{
		return _macroCommandType.getName();
	}

	BOT_ASSERT(false, "bad MacroAct");
	return "error";
}
