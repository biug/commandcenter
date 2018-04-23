#pragma once
#include "GA.h"
using namespace std;
sc2::Point2D CalcChasePosition(const sc2::Unit * unit, double angel, const sc2::Unit * chaser);
sc2::Point2D CalcMovePosition(const sc2::Unit * unit);
sc2::Point2D CalcMovePosition(const sc2::Unit * unit, double angel);
double CalcReward(const sc2::Unit * unit, Candidate can, const sc2::Unit * target);
