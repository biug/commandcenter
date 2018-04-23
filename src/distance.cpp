#include <iostream>
#include <cmath>
#include "GA.h"
#include "Threadpool.h"
#include <random>
#include <chrono>
#include <fstream>
#include "distance.h"
using namespace std;
sc2::Point2D CalcChasePosition(const sc2::Unit * unit, double angel, const sc2::Unit * chaser) {
	double distance_x;
	double distance_y;
	sc2::Point2D chaserPos = chaser->pos;
	sc2::Point2D unitPos = unit->pos;
	for (int i = 0; i < 10; i++) {
		unitPos.x += 0.2 * cos(angel);
		unitPos.y -= 0.2 * sin(angel);
		distance_x = chaserPos.x - unitPos.x;
		distance_y = chaserPos.y - unitPos.y;
		chaserPos.x -= distance_x > 0 ? 0.2 * cos(atan(distance_x / distance_y)) : -0.2 * cos(atan(distance_x / distance_y));
		chaserPos.y -= distance_x > 0 ? 0.2 * sin(atan(distance_x / distance_y)) : -0.2 * sin(atan(distance_x / distance_y));
	}
	return chaserPos;
}
sc2::Point2D CalcMovePosition(const sc2::Unit * unit) {
	return sc2::Point2D(unit->pos.x + 2 * cos(AngelToRadius(90) - unit->facing), unit->pos.y - 2 * sin(AngelToRadius(90) - unit->facing));
}

double CalcReward(const sc2::Unit * unit, Candidate can, const sc2::Unit * target) {
	int tmp = 0;
	double angel;
	for (int i = 0; i < 6; i++) {
		tmp *= 2;
		tmp += can.GetGene(i);
	}
	angel = tmp * 360 / 64;
	sc2::Point2D unitPos = CalcMovePosition(unit, angel);
	sc2::Point2D targetPos = CalcChasePosition(unit, angel, target);
	double distance = sqrt((unitPos.x - targetPos.x) * (unitPos.x - targetPos.x) + (unitPos.y - targetPos.y) * (unitPos.y - targetPos.y));
	return distance < 6 ? -distance*distance/6 + 2 * distance : (12 - distance);
}
sc2::Point2D CalcMovePosition(const sc2::Unit * unit, double angel) {
	return sc2::Point2D(unit->pos.x + 2 * cos(AngelToRadius(angel)), unit->pos.y - 2 * sin(AngelToRadius(angel)));
}

