#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>

#include "corona.h"

cfg_t cfg;
stats_t *all_cycle_stats;
stats_t *cycle_stats;
stats_t *prev_cycle_stats;
stats_t global_stats;
region_t *region = NULL;
uint32_t current_cycle;

static const char default_results_file[] = "results-cycles.csv";

/****************************************************************/

/*
	probability between 0.0 and 1.0
*/

int roll_dice (double probability)
{
	double dice;

	dice = (double)rand() / (double)RAND_MAX;

	return (dice <= probability);
}

inline double calculate_infection_probability ()
{
	double p = cfg.probability_infect_per_day * ((double)(cfg.population - global_stats.infected) / (double)cfg.population);
	return p;
}

/****************************************************************/

region_t::region_t ()
{
	uint64_t i;

	this->people = new person_t[ cfg.population ];

	for (i=0; i<cfg.population; i++)
		this->people[i].set_region(this);

	this->must_infect_in_cycle = 0;

	this->sir_init();
}

void region_t::cycle ()
{
	uint64_t i, j;
	person_t *p;

	this->must_infect_in_cycle = 0;

	for (i=0; i<cfg.population; i++) {
		p = this->people + i;
		p->cycle();

		if (p->get_state() == ST_INFECTED)
			cycle_stats->infected++;
	}

	dprintf("must_infect_in_cycle: " PU64 "\n", this->must_infect_in_cycle);

	for (i=0, j=0; i<cfg.population && j<this->must_infect_in_cycle; i++) {
		p = this->people + i;

		if (p->get_state() == ST_HEALTHY) {
			p->infect();
			j++;
		}
	}

	this->sir_calc();

	cycle_stats->dump();
	global_stats.global_dump();

	cprintf("\n");
}

void region_t::sir_init ()
{

}

void region_t::sir_calc ()
{
	if (unlikely(current_cycle == 0)) {
		cycle_stats->sir_s = (double)(cfg.population - 1) / (double)cfg.population;
		cycle_stats->sir_i = 1.0 - cycle_stats->sir_s;
		cycle_stats->sir_r = 0.0;
	}
	else {
		double h = 1.0;
		double B = cfg.r0/10.0;
		double L = 0.1;
		stats_t *prev = prev_cycle_stats;

		/*
		
		dS = -B.S.I
		dt

		dI = B.S.I - LI
		dt

		dR = LI
		dt

		y(t+h) = y(t) + h.dy
		                  dt

		*/

		cycle_stats->sir_s = prev->sir_s - h * (B * prev->sir_s * prev->sir_i);
		cycle_stats->sir_i = prev->sir_i + h * (B * prev->sir_s * prev->sir_i - L*prev->sir_i);
		cycle_stats->sir_r = prev->sir_r + h * (L * prev->sir_i);
	}
}

void region_t::process_data ()
{
	int32_t i;
	stats_t *cycle_stats = all_cycle_stats;

	for (i=0; i<cfg.days_to_simulate; i++) {
		cycle_stats->sir_s *= (double)cfg.population;
		cycle_stats->sir_i *= (double)cfg.population;
		cycle_stats->sir_r *= (double)cfg.population;

		cycle_stats++;
	}
}

/****************************************************************/

person_t::person_t ()
{
	this->state = ST_HEALTHY;
	this->infection_countdown = 0.0;
	this->region = NULL;
}

void person_t::die ()
{
	this->state = ST_DEAD;
	cycle_stats->deaths++;
	global_stats.deaths++;
}

void person_t::cycle_infected ()
{
	if (roll_dice(calculate_infection_probability())) {
		this->region->must_infect_in_cycle++;
	}

	if (roll_dice(cfg.probability_death_per_day))
		this->die();
	else {
		this->infection_countdown -= 1.0;

		if (this->infection_countdown <= 0.0) {
			this->infection_countdown = 0.0;
			this->state = ST_IMMUNE;
			//dprintf("I got IMMUNE\n");
		}
	}
}

void person_t::infect ()
{
	this->state = ST_INFECTED;
	cycle_stats->new_infected++;
	cycle_stats->infected++;
	cycle_stats->ac_healthy--;

	global_stats.infected++;

	this->infection_countdown = cfg.days_contagious;
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
			C_ASSERT(0);
	}
}

/****************************************************************/

cfg_t::cfg_t ()
{
	this->r0 = 2.4;
	this->death_rate = 0.02;
	this->days_contagious = 7.0;
	this->population = 100000;
	this->days_to_simulate = 180;

	this->load_derived();
}

void cfg_t::load_derived ()
{
	this->probability_infect_per_day = this->r0 / this->days_contagious;
	this->probability_death_per_day = this->death_rate / this->days_contagious;
}

void cfg_t::dump ()
{
	dprintf("# ro = %0.4f\n", this->r0);
	dprintf("# death_rate = %0.4f\n", this->death_rate);
	dprintf("# days_contagious = %0.4f\n", this->days_contagious);
	dprintf("# population = " PU64 "\n", this->population);
	dprintf("# days_to_simulate = %u\n", this->days_to_simulate);

	dprintf("# probability_infect_per_day = %0.4f\n", this->probability_infect_per_day);
	dprintf("# probability_death_per_day = %0.4f\n", this->probability_death_per_day);
}

/****************************************************************/

static void simulate ()
{
	cycle_stats = all_cycle_stats;

	region->get_person(0)->infect();
	cycle_stats->infected = 0; // will be considered during the cycle

	for (current_cycle=0; current_cycle<cfg.days_to_simulate; current_cycle++) {
		cprintf("Day %i\n", current_cycle);

		if (likely(current_cycle > 0))
			cycle_stats->copy_ac(prev_cycle_stats);

		region->cycle();

		prev_cycle_stats = cycle_stats++;
	}

	region->process_data();
}

/****************************************************************/

stats_t::stats_t ()
{
	this->reset();
}

void stats_t::reset ()
{
	#define CORONA_STAT(TYPE, PRINT, STAT) this->STAT = 0;
	#include "stats.h"
	#undef CORONA_STAT
}

void stats_t::copy_ac (stats_t *from)
{
	this->ac_deaths = from->ac_deaths;
	this->ac_immuned = from->ac_immuned;
	this->ac_healthy = from->ac_healthy;
}

void stats_t::dump ()
{
	#define CORONA_STAT(TYPE, PRINT, STAT) cprintf(#STAT ":" PRINT " ", this->STAT);
	#include "stats.h"
	#undef CORONA_STAT

	cprintf("\n");
}

void stats_t::dump_csv_header (FILE *fp)
{
	fprintf(fp, "cycle,");

	#define CORONA_STAT(TYPE, PRINT, STAT) fprintf(fp, #STAT ",");
	#include "stats.h"
	#undef CORONA_STAT

	fprintf(fp, "\n");
}

void stats_t::dump_csv (FILE *fp)
{
	fprintf(fp, "%u,", this->cycle);

	#define CORONA_STAT(TYPE, PRINT, STAT) fprintf(fp, PRINT ",", this->STAT);
	#include "stats.h"
	#undef CORONA_STAT

	fprintf(fp, "\n");
}

void stats_t::global_dump ()
{
	cprintf("total_deaths:" PU64 " total_infected:" PU64 "\n", this->deaths, this->infected);
}

/****************************************************************/

static void start_dice_engine ()
{
	srand(time(NULL));
}

static void load_region()
{
	region = new region_t();
}

static void load_stats_engine ()
{
	int32_t i;
	
	all_cycle_stats = new stats_t[ cfg.days_to_simulate ];

	for (i=0; i<cfg.days_to_simulate; i++)
		all_cycle_stats[i].cycle = i;

	all_cycle_stats[0].ac_healthy = cfg.population;
}

int main ()
{
	int32_t i;
	FILE *fp;

	cfg.dump();

	start_dice_engine();

	load_stats_engine();

	load_region();

	simulate();

	fp = fopen(default_results_file, "w");
	C_ASSERT(fp != NULL)

	all_cycle_stats[0].dump_csv_header(fp);
	for (i=0; i<cfg.days_to_simulate; i++)
		all_cycle_stats[i].dump_csv(fp);

	fclose(fp);

	return 0;
}