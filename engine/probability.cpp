#include <random>

#include <corona.h>

static std::mt19937 rgenerator;

static std::uniform_real_distribution<double> rdistribution(0.0, 1.0);

static std::gamma_distribution<double> *gdistribution_incubation;

void start_dice_engine ()
{
	//srand(time(NULL));
	rgenerator.seed(time(NULL));
}

static double gamma_calculate_alpha (double mean, double stddev)
{
	return ( (mean*mean) / (stddev*stddev) );
}

static double gamma_calculate_betha (double mean, double stddev)
{
	return ( (stddev*stddev) / mean );
}

void load_gdistribution_incubation (double mean, double stddev)
{
	double alpha, betha;

	alpha = gamma_calculate_alpha(mean, stddev);
	betha = gamma_calculate_betha(mean, stddev);

	dprintf("gamma distribution (incubation) mean:%.2f stddev:%.2f alpha:%.2f betha:%.2f\n", mean, stddev, alpha, betha);

	gdistribution_incubation = new std::gamma_distribution<double> (alpha, betha);
}

double calculate_incubation_cycles ()
{
	double c;

	c = (*gdistribution_incubation)(rgenerator);
	//dprintf("gamma distribution (incubation) cycles: %.2f\n", c);
	
	return c;
}

double generate_random_between_0_and_1 ()
{
	//return ( (double)rand() / (double)RAND_MAX );
	return rdistribution(rgenerator);
}

/*
	probability between 0.0 and 1.0
*/

int roll_dice (double probability)
{
	return (generate_random_between_0_and_1() <= probability);
}

double calculate_infection_probability (person_t *from)
{
	double p = cfg.probability_infect_per_cycle * cfg.global_r0_factor * ((double)(cycle_stats->ac_healthy) / (double)cfg.population) * r0_factor_per_group[ from->get_infected_state() ];
	return p;
}
