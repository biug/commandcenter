#include<cmath>
#include<random>
#include"distance.h"
double AngelToRadius(double angel)
{
	return angel*3.1415926 / 180;
}
Candidate::Candidate() {
}

Candidate::Candidate(const std::vector<int> genes) {
	this->genes = genes;
}

Candidate::Candidate(const std::vector<int> genes, const double fitness) {
	this->genes = genes;
	this->fitness = fitness;
}

void Candidate::SetFitness(const double fitness) {
	this->fitness = fitness;
}

void Candidate::SetGene(int index, int gene)
{
	genes[index] = gene;
}

int Candidate::GetGene(const int i)
{
	return genes[i];
}

vector<int> Candidate::GetGenes()
{
	return genes;
}
double Candidate::GetFitness() const
{
	return fitness;
}

Population::Population(const int size)
{
	candidates = std::vector<Candidate>();
	candidates.resize(size);
}

void Population::SetCanidate(const int index, Candidate c)
{
	candidates[index] = c;
}

Candidate Population::GetCandidate(const int index)
{
	return candidates[index];
}

void Population::SetReward(const int index, const double reward)
{
	candidates[index].SetFitness(reward);
}

Candidate Population::GetFittest() {
	Candidate fittest = candidates[0];
	
	for (size_t i = 0; i < candidates.size(); i++) {
		if (fittest.GetFitness() < GetCandidate(i).GetFitness()) {
			fittest = GetCandidate(i);
		}
	}
	return fittest;
}

GeneticAlgorithm::GeneticAlgorithm(const sc2::Unit * unit, const sc2::Unit * target)
	:mutation_rate(10),
	tournament_size(3),
	elitism(true),
	population(6),
	unit(unit),
	target(target)
{
	for (int i = 0; i < 6; i++)
	{
		random_device rd;
		std::vector<int> genes = std::vector<int>();
		genes.resize(6);
		for (int j = 0; j < 6; j++) {
			genes[j] = rd() % 2;
		}
		const Candidate can = Candidate(genes);
		population.SetCanidate(i, can);
		population.SetReward(i, CalcReward(unit, can, target));
	}
}

GeneticAlgorithm::GeneticAlgorithm()
	:mutation_rate(5),
	tournament_size(3),
	elitism(true),
	population(6) 
{
}

Candidate GeneticAlgorithm::EvolvePopulation(int iteration) {
	random_device rd;
	for (int it = 0; it < iteration; it++) {
		Population new_population(6);
		int elitism_offset;
		if (elitism) {
			new_population.SetCanidate(0, population.GetFittest());
			elitism_offset = 1;
		}
		else {
			elitism_offset = 0;
		}
		//cout << endl;
		for (int i = elitism_offset; i < 6; i++) {
			Candidate indiv1 = TournamentSelection(population);
			int j = 8;
			Candidate indiv2;
			do {
				indiv2 = TournamentSelection(population);
				j--;
			} while (indiv1.GetGenes() == indiv2.GetGenes() && j > 0);
			Candidate new_indiv = Crossover(indiv1, indiv2);
			if(it < iteration - 1)
				Mutate(new_indiv);
			new_population.SetCanidate(i, new_indiv);
			new_population.SetReward(i, CalcReward(unit, new_population.GetCandidate(i), target));
		}
		population = new_population;
		//cout << population.GetFittest().GetFitness() << endl;
	}
	return population.GetFittest();
}

Candidate GeneticAlgorithm::Crossover(Candidate indiv1, Candidate indiv2) const
{
	auto genes = std::vector<int>();
	genes.resize(6);
	random_device rd;
	for (int i = 0; i < 6; i++) { 
		if (rd() % 2 == 0) {
			genes[i] = indiv1.GetGene(i);
		}
		else {
			genes[i] = indiv2.GetGene(i);
		}
	}

	auto can = Candidate(genes);
	return can;
}

void GeneticAlgorithm::Mutate(Candidate & indiv) const
{
	random_device rd;
	for (int i = 0; i < 6; i++) {
		if (rd()%100 <= mutation_rate) {
			int gene = !indiv.GetGene(i);
			indiv.SetGene(i, gene);
		}
	}
}

Population* GeneticAlgorithm::GetPopulation()
{
	return &population;
}

void GeneticAlgorithm::SetReward(const int i, const double reward)
{
	population.SetReward(i, reward);
}

Candidate GeneticAlgorithm::TournamentSelection(Population pop) const
{
	Population tournament(3);
	vector<int> tmp = vector<int>(6, 1);
	
	for (int i = 0; i < 3; i++) {
		int random_id;
		do{
			 random_id = rand() % 6;
		}while (tmp[random_id] == 0);
		tmp[random_id] = 0;
		tournament.SetCanidate(i, pop.GetCandidate(random_id));
	}
	Candidate fittest = tournament.GetFittest();
	return fittest;
}
sc2::Point2D GeneticAlgorithm::GA(const sc2::Unit * unit,const sc2::Unit * target) {
	GeneticAlgorithm ga = GeneticAlgorithm(unit, target);
	Candidate can = ga.EvolvePopulation(10);
	int tmp = 0;
	double angel;
	for (int i = 0; i < 6; i++) {
		tmp *= 2;
		tmp += can.GetGene(i);
	}
	angel = tmp * 360 / 64;
	sc2::Point2D unitPos = CalcMovePosition(unit, angel);
	return unitPos;
}