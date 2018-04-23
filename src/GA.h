#pragma once
#include <sc2api/sc2_api.h>
#include <vector>
#include <iostream>
#include <random>
double AngelToRadius(double angel);

class Candidate {
	
	double fitness;
public:
	std::vector<int> genes;
	Candidate();
	Candidate(std::vector<int>genes);
	Candidate(std::vector<int>genes,double fitness);
	void SetFitness(double fitness);
	void SetGene(int index, int gene);
	int GetGene(int i);
	std::vector<int> GetGenes();
	double GetFitness() const;
};

class Population {
	std::vector<Candidate> candidates;
public:
	Population(const int size);
	void SetCanidate(int index, Candidate c);
	Candidate GetCandidate(int index);
	Candidate GetFittest();
	void SetReward(int i, double reward);
};

class GeneticAlgorithm {
	double mutation_rate;
	int tournament_size;
	bool elitism;
	bool isChasing;
	Population population;
	Candidate Crossover(Candidate indiv1, Candidate indiv2) const;
	void Mutate(Candidate &indiv) const;
	const sc2::Unit * unit;
	const sc2::Unit * target;
	Candidate TournamentSelection(Population pop) const;

public:
	GeneticAlgorithm();
	GeneticAlgorithm(const sc2::Unit * unit ,const sc2::Unit * target);
	Candidate EvolvePopulation(int iteration);
	Population* GetPopulation();
	void SetReward(int i, double reward);
	static sc2::Point2D GA(const sc2::Unit * unit, const sc2::Unit * target);
};