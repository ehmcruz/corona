#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <assert.h>

enum state_t {
	ST_HEALTHY,
	ST_INFECTED,
	ST_IMMUNE,
	ST_DEAD
};


double cfg_r0 = 2.4;
double cfg_death_rate = 0.02;
uint32_t cfg_days_contagious = 7;

double probability_infect_per_day;

/*
	probability between 0.0 and 1.0
*/

int roll_dice (double probability)
{
	double dice;

	dice = (double)rand() / (double)RAND_MAX;

	return (dice <= probability);
}

class person_t
{
private:
	state_t state;
	int32_t infection_countdown;

public:
	person_t();

	void cycle_day ();
	void cycle_infected ();
};

person_t::person_t ()
{
	this->state = ST_HEALTHY;
}

void person_t::cycle_infected ()
{
	if (roll_dice(probability_infect_per_day)) {

	}

	this->infection_countdown--;

	if 
}

void person_t::cycle_day ()
{
	switch (this->state) {
		case ST_HEALTHY:
		break;

		case ST_INFECTED:
			this->cycle_infected();
		break;

		case ST_IMMUNE:
		break;

		case ST_DEAD:
		break;

		default:
			assert(0);
	}
}

class region_t
{
private:

public:
	region_t();
};

static void simulate ()
{

}

static void start_dice_engine ()
{
	srand(time(NULL));
}

int main ()
{
	start_dice_engine();

	probability_infect_per_day = cfg_r0 / (double)cfg_days_contagious;

	simulate();

	return 0;
}