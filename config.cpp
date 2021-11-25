#include "config.hpp"

std::default_random_engine& RandEngine()
{
	static std::default_random_engine e;

	return e;
}

float RandFloat()
{
	static std::uniform_real_distribution<float> d(0, 1);

	return d(RandEngine());
}

void SeedRand()
{
	RandEngine().seed(time(NULL));
}