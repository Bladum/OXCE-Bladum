/*
 * Copyright 2010-2016 OpenXcom Developers.
 *
 * This file is part of OpenXcom.
 *
 * OpenXcom is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * OpenXcom is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with OpenXcom.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "ResearchProject.h"
#include "../Mod/RuleResearch.h"
#include "../Engine/RNG.h"

namespace OpenXcom
{
const int PROGRESS_LIMIT_UNKNOWN = 20;
const int PROGRESS_LIMIT_POOR = 40;
const int PROGRESS_LIMIT_AVERAGE = 60;
const int PROGRESS_LIMIT_GOOD = 80;

const int BREAKTHROUGHT_NONE = 10;		// 10% = 0
const int BREAKTHROUGHT_HALF = 30;		// 20% = 50%
const int BREAKTHROUGHT_NORMAL = 70;	// 40% = 100%	
const int BREAKTHROUGHT_DOUBLE = 90;	// 20% = 200%
const int BREAKTHROUGHT_QUAD = 100;		// 10% = 400%

ResearchProject::ResearchProject(RuleResearch * p, int c) : _project(p), _assigned(0), _spent(0), _cost(c)
{

}

/**
 * Called every day to compute time spent on this ResearchProject
 * @return true if the ResearchProject is finished
 */
bool ResearchProject::step()
{
	int progressToday = _assigned;

	int rand = RNG::generate(0, 99);
	if (rand < BREAKTHROUGHT_NONE)
		progressToday = 0;
	else if (rand < BREAKTHROUGHT_HALF)
		progressToday = (int)(progressToday * 0.5f);
	else if (rand < BREAKTHROUGHT_NORMAL)
		progressToday = progressToday;
	else if (rand < BREAKTHROUGHT_DOUBLE)
		progressToday = progressToday * 2;
	else if (rand < BREAKTHROUGHT_QUAD)
		progressToday = progressToday * 4;

	_spent += progressToday;
	if (_spent >= getCost())
	{
		return true;
	}
	return false;
}

/**
 * Changes the number of scientist to the ResearchProject
 * @param nb number of scientist assigned to this ResearchProject
 */
void ResearchProject::setAssigned (int nb)
{
	_assigned = nb;
}

const RuleResearch * ResearchProject::getRules() const
{
	return _project;
}

/**
 * Returns the number of scientist assigned to this project
 * @return Number of assigned scientist.
 */
int ResearchProject::getAssigned() const
{
	return _assigned;
}

/**
 * Returns the time already spent on this project
 * @return the time already spent on this ResearchProject(in man/day)
 */
int ResearchProject::getSpent() const
{
	return _spent;
}

/**
 * Changes the cost of the ResearchProject
 * @param spent new project cost(in man/day)
 */
void ResearchProject::setSpent (int spent)
{
	_spent = spent;
}

/**
 * Returns the cost of the ResearchProject
 * @return the cost of the ResearchProject(in man/day)
 */
int ResearchProject::getCost() const
{
	return _cost;
}

/**
 * Changes the cost of the ResearchProject
 * @param f new project cost(in man/day)
 */
void ResearchProject::setCost(int f)
{
	_cost = f;
}

/**
 * Loads the research project from a YAML file.
 * @param node YAML node.
 */
void ResearchProject::load(const YAML::Node& node)
{
	setAssigned(node["assigned"].as<int>(getAssigned()));
	setSpent(node["spent"].as<int>(getSpent()));
	setCost(node["cost"].as<int>(getCost()));
}

/**
 * Saves the research project to a YAML file.
 * @return YAML node.
 */
YAML::Node ResearchProject::save() const
{
	YAML::Node node;
	node["project"] = getRules()->getName();
	node["assigned"] = getAssigned();
	node["spent"] = getSpent();
	node["cost"] = getCost();
	return node;
}

/**
 * Return a string describing Research progress.
 * @return a string describing Research progress.
 */
std::string ResearchProject::getResearchProgress() const
{
	int progress = (int) ( getSpent() * 100 / getRules()->getCost() );	
	
	// DISPLAY PROGRESS AS PERCENT
	std::string ddd = std::to_string( progress ).c_str();
	ddd += "%";
	return ddd;
	
	// DISPLAY PROGRESS AS TEXT 
	if (getAssigned() == 0)
	{
		return "STR_NONE";
	}
	else if (progress < PROGRESS_LIMIT_UNKNOWN)
	{
		return "STR_UNKNOWN";
	}
	else if (progress < PROGRESS_LIMIT_POOR)
	{
		return "STR_POOR";
	}
	else if (progress < PROGRESS_LIMIT_AVERAGE)
	{
		return "STR_AVERAGE";
	}
	else if (progress < PROGRESS_LIMIT_GOOD)
	{
		return "STR_GOOD";
	}
	return "STR_EXCELLENT";
}

}
