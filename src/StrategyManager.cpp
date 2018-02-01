#include "StrategyManager.h"
#include "CCBot.h"
#include "JSONTools.h"
#include "Util.h"
#include "BuildType.h"

Strategy::Strategy()
{

}

Strategy::Strategy(const std::string & name, const sc2::Race & race, const BuildOrder & buildOrder)
    : m_name(name)
    , m_race(race)
    , m_buildOrder(buildOrder)
    , m_wins(0)
    , m_losses(0)
	
{

}

// constructor
StrategyManager::StrategyManager(CCBot & bot)
    : m_bot(bot)
	, bases_safe(false)
{

}

void StrategyManager::onStart()
{
    readStrategyFile(m_bot.Config().ConfigFileLocation);
}

void StrategyManager::onFrame()
{
	bases_safe = AreBasesSafe();
	for (const auto & unit : m_bot.UnitInfo().getUnits(Players::Self))
	{
		// Emergency repair units and depots.
		if (Util::IsBuilding(unit->unit_type))
		{
			// If a depot may die, go repair it. 
			if (unit->unit_type == sc2::UNIT_TYPEID::TERRAN_SUPPLYDEPOT || unit->unit_type == sc2::UNIT_TYPEID::TERRAN_SUPPLYDEPOTLOWERED)
			{
				if (unit->health != unit->health_max && unit->build_progress == 1.0)
					Micro::SmartRepair(m_bot.Workers().getClosestBuildableWorkerTo(unit->pos), unit, m_bot);
			}
			/*
			if (!bases_safe)
			{

				m_bot.Actions()->UnitCommand(unit, sc2::ABILITY_ID::LIFT);
			}
			else
			{
				 m_bot.Actions()->UnitCommand(unit, sc2::ABILITY_ID::LAND,unit->pos);
			}*/
		}
	}
}

const BuildOrder & StrategyManager::getOpeningBookBuildOrder() const
{
    auto buildOrderIt = m_strategies.find(m_bot.Config().StrategyName);

    // look for the build order in the build order map
    if (buildOrderIt != std::end(m_strategies))
    {
        return (*buildOrderIt).second.m_buildOrder;
    }
    else
    {
        BOT_ASSERT(false, "Strategy not found: %s, returning empty initial build order", m_bot.Config().StrategyName.c_str());
        return m_emptyBuildOrder;
    }
}



void StrategyManager::addStrategy(const std::string & name, const Strategy & strategy)
{
    m_strategies[name] = strategy;
}

const UnitPairVector StrategyManager::getBuildOrderGoal() const
{
    return std::vector<UnitPair>();
}

const UnitPairVector StrategyManager::getProtossBuildOrderGoal() const
{
    return std::vector<UnitPair>();
}

const UnitPairVector StrategyManager::getTerranBuildOrderGoal() const
{
    return std::vector<UnitPair>();
}

const UnitPairVector StrategyManager::getZergBuildOrderGoal() const
{
    return std::vector<UnitPair>();
}


void StrategyManager::onEnd(const bool isWinner)
{

}

bool StrategyManager::ShouldExpandNow() const
{
	if (m_bot.Observation()->GetMinerals() > 500 && bases_safe)
	{
		return true;
	}
	return false;
}

bool StrategyManager::AreBasesSafe()
{
	for (const auto & enemy_unit : m_bot.UnitInfo().getUnits(Players::Enemy))
	{
		for (const auto  potential_base : m_bot.UnitInfo().getUnits(Players::Self))
		{
			if (Util::IsTownHall(potential_base)
				&& Util::DistSq(potential_base->pos, enemy_unit->pos) < (30 * 30))
			{
				return false;
			}
		}
	}
	return true;
}

void StrategyManager::readStrategyFile(const std::string & filename)
{
    sc2::Race race = m_bot.GetPlayerRace(Players::Self);
    std::string ourRace = Util::GetStringFromRace(race);
    std::string config = JSONTools::ReadFile(filename);
    rapidjson::Document doc;
    bool parsingFailed = doc.Parse(config.c_str()).HasParseError();
    if (parsingFailed)
    {
        std::cerr << "ParseStrategy could not find file: " << filename << ", shutting down.\n";
        return;
    }

    // Parse the Strategy Options
    if (doc.HasMember("Strategy") && doc["Strategy"].IsObject())
    {
        const rapidjson::Value & strategy = doc["Strategy"];

        // read in the various strategic elements
        JSONTools::ReadBool("ScoutHarassEnemy", strategy, m_bot.Config().ScoutHarassEnemy);
        JSONTools::ReadString("ReadDirectory", strategy, m_bot.Config().ReadDir);
        JSONTools::ReadString("WriteDirectory", strategy, m_bot.Config().WriteDir);

        // if we have set a strategy for the current race, use it
        if (strategy.HasMember(ourRace.c_str()) && strategy[ourRace.c_str()].IsString())
        {
            m_bot.Config().StrategyName = strategy[ourRace.c_str()].GetString();
        }

        // check if we are using an enemy specific strategy
        JSONTools::ReadBool("UseEnemySpecificStrategy", strategy, m_bot.Config().UseEnemySpecificStrategy);
        if (m_bot.Config().UseEnemySpecificStrategy && strategy.HasMember("EnemySpecificStrategy") && strategy["EnemySpecificStrategy"].IsObject())
        {
            // TODO: Figure out enemy name
            const std::string enemyName = "ENEMY NAME";
            const rapidjson::Value & specific = strategy["EnemySpecificStrategy"];

            // check to see if our current enemy name is listed anywhere in the specific strategies
            if (specific.HasMember(enemyName.c_str()) && specific[enemyName.c_str()].IsObject())
            {
                const rapidjson::Value & enemyStrategies = specific[enemyName.c_str()];

                // if that enemy has a strategy listed for our current race, use it
                if (enemyStrategies.HasMember(ourRace.c_str()) && enemyStrategies[ourRace.c_str()].IsString())
                {
                    m_bot.Config().StrategyName = enemyStrategies[ourRace.c_str()].GetString();
                    m_bot.Config().FoundEnemySpecificStrategy = true;
                }
            }
        }

        // Parse all the Strategies
        if (strategy.HasMember("Strategies") && strategy["Strategies"].IsObject())
        {
            const rapidjson::Value & strategies = strategy["Strategies"];
            for (auto itr = strategies.MemberBegin(); itr != strategies.MemberEnd(); ++itr)
            {
                const std::string &         name = itr->name.GetString();
                const rapidjson::Value &    val  = itr->value;

                sc2::Race strategyRace;
                if (val.HasMember("Race") && val["Race"].IsString())
                {
                    strategyRace = Util::GetRaceFromString(val["Race"].GetString());
                }
                else
                {
                    BOT_ASSERT(false, "Strategy must have a Race string: %s", name.c_str());
                    continue;
                }

                BuildOrder buildOrder(strategyRace);
                if (val.HasMember("OpeningBuildOrder") && val["OpeningBuildOrder"].IsArray())
                {
                    const rapidjson::Value & build = val["OpeningBuildOrder"];

                    for (rapidjson::SizeType b(0); b < build.Size(); ++b)
                    {
                        if (build[b].IsString())
                        {
                            MacroAct buildType(build[b].GetString(), m_bot);
                            buildOrder.add(buildType);
                        }
                        else
                        {
                            BOT_ASSERT(false, "Build order item must be a string %s", name.c_str());
                            continue;
                        }
                    }
                }

                addStrategy(name, Strategy(name, strategyRace, buildOrder));
            }
        }
    }
}