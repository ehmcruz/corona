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

class cfg_t {
public:
	// primary cfg
	double r0;
	double death_rate;
	double days_contagious;
	uint64_t population;
	uint32_t days_to_simulate;

	// derived cfg
	double probability_infect_per_day;
	double probability_death_per_day;

	cfg_t();
	void load_derived();
};

class stats_t {
public:
	uint64_t deaths;
	uint64_t infected;
	uint64_t new_infected;

	stats_t();
	void reset();
};

class region_t;

class person_t
{
private:
	state_t state;
	int32_t infection_countdown;

public:
	person_t();

	void cycle ();
	void cycle_infected ();
	void die();
	void infect();

	inline state_t get_state () {
		return this->state;
	}
};

class region_t
{
private:
	person_t *people;
	uint64_t must_infect_in_cycle;
	region_t *region;

public:
	region_t();

	void cycle();

	inline void set_region (region_t *region) {
		this->region = region;
	}

	inline region_t* get_region () {
		return this->region;
	}
};

cfg_t cfg;
stats_t cycle_stats;
stats_t global_stats;
region_t *region = NULL;

/*
	probability between 0.0 and 1.0
*/

int roll_dice (double probability)
{
	double dice;

	dice = (double)rand() / (double)RAND_MAX;

	return (dice <= probability);
}

stats_t::stats_t ()
{
	this->reset();
}

void stats_t::reset ()
{
	this->deaths = 0;
	this->infected = 0;
	this->new_infected = 0;
}

region_t::region_t ()
{
	uint64_t i;

	this->people = new person_t[ cfg.population ];

	for (i=0; i<cfg.population; i++)
		this->people[i].set_region(this);

	this->must_infect_in_cycle = 0;
}

void region_t::cycle ()
{
	uint64_t i;
	person_t *p;

	cycle_stats.reset();

	this->must_infect_in_cycle = 0;

	for (i=0; i<cfg.population; i++) {
		p = this->people[i];
		p->cycle();

		if (p->get_state() == ST_INFECTED)
			cycle_stats.infected++;
	}

	for (i=0; i<this->must_infect_in_cycle; i++) {
		p = this->people[i];

		if (p->get_state() == ST_HEALTHY) {
			p->infect();
		}
	}
}

person_t::person_t ()
{
	this->state = ST_HEALTHY;
	this->infection_countdown = 0;
}

void person_t::die ()
{
	this->state = ST_DEAD;
	cycle_stats.deaths++;
	global_stats.deaths++;
}

void person_t::cycle_infected ()
{
	if (roll_dice(probability_infect_per_day)) {
		this->region->must_infect_in_cycle++;
	}

	if (roll_dice(cfg.probability_death_per_day))
		this->die();
	else {
		this->infection_countdown--;

		if (this->infection_countdown <= 0) {
			this->state = ST_IMMUNE;
		}
	}
}

void person_t::infect ()
{
	this->state = ST_INFECTED;
	cycle_stats.new_infected++;
	cycle_stats.infected++;

	global_stats.infected++;
}

void person_t::cycle ()
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

static void simulate ()
{
	int32_t i;

	for (i=0; i<cfg.days_to_simulate; i++) {
		region->cycle();
	}
}

static void start_dice_engine ()
{
	srand(time(NULL));
}

cfg_t::cfg_t ()
{
	this->r0 = 2.4;
	this->death_rate = 0.02;
	this->days_contagious = 7.0;
	this->population = 100000;
	this->days_to_simulate = 365;

	this->load_derived();
}

void cfg::load_derived ()
{
	this->probability_infect_per_day = this->r0 / this->days_contagious;
	this->probability_death_per_day = this->death_rate / this->days_contagious;
}

static void load_region()
{
	region = new region_t();
}

int main ()
{
	start_dice_engine();
	load_region();

	simulate();

	return 0;
}