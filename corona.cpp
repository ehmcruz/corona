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

double r0_factor_per_group[NUMBER_OF_INFECTED_STATES];

static const char default_results_file[] = "results-cycles.csv";

char* infected_state_str (int32_t i)
{
	static const char *list[] = {
		"ST_ASYMPTOMATIC",
		"ST_MILD",
		"ST_SEVERE",
		"ST_CRITICAL"
	};

	C_ASSERT(i < NUMBER_OF_INFECTED_STATES)

	return (char*)list[i];
}

/****************************************************************/

double generate_random_between_0_and_1 ()
{
	return ( (double)rand() / (double)RAND_MAX );
}

/*
	probability between 0.0 and 1.0
*/

int roll_dice (double probability)
{
	return (generate_random_between_0_and_1() <= probability);
}

inline double calculate_infection_probability (person_t *from)
{
	double p = cfg.probability_infect_per_cycle * ((double)(cfg.population - global_stats.infected) / (double)cfg.population) * r0_factor_per_group[ from->get_infected_state() ];
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

		if (p->get_state() == ST_INFECTED)
			cycle_stats->infected++;

		p->cycle();
	}

	dprintf("must_infect_in_cycle: " PU64 "\n", this->must_infect_in_cycle);

	for (i=0, j=0; i<cfg.population && j<this->must_infect_in_cycle; i++) {
		p = this->people + i;

		if (p->get_state() == ST_HEALTHY) {
			p->pre_infect();
			j++;
		}
	}

	for (i=0; i<NUMBER_OF_INFECTED_STATES; i++) {
		if (cycle_stats->ac_infected_state[i] > cycle_stats->peak[i])
			cycle_stats->peak[i] = cycle_stats->ac_infected_state[i];
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

	for (i=0; i<cfg.cycles_to_simulate; i++) {
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
	this->infection_cycles = 0.0;
	this->region = NULL;
}

void person_t::die ()
{
	this->state = ST_DEAD;
	cycle_stats->deaths++;

	cycle_stats->ac_infected--;
	cycle_stats->ac_deaths++;

	cycle_stats->ac_infected_state[ this->infected_state ]--;
	
	global_stats.deaths++;
}

void person_t::recover ()
{
	this->state = ST_IMMUNE;
	cycle_stats->immuned++;

	cycle_stats->ac_infected--;
	cycle_stats->ac_immuned++;
	
	cycle_stats->ac_infected_state[ this->infected_state ]--;
}

void person_t::cycle_infected ()
{
	if (roll_dice(calculate_infection_probability(this))) {
		this->region->must_infect_in_cycle++;
	}

	this->infection_countdown -= 1.0;

	switch (this->infected_state) {
		case ST_ASYMPTOMATIC:
		case ST_SEVERE:
			if (this->infection_countdown <= 0.0) {
				this->infection_countdown = 0.0;
				this->recover();
			}
		break;

		case ST_MILD:
			if (this->infection_countdown <= 0.0) {
				this->infection_countdown = 0.0;

				switch (this->next_infected_state) {
					case ST_FAKE_IMMUNE:
						this->recover();
					break;

					case ST_SEVERE:
						cycle_stats->ac_infected_state[ST_MILD]--;
						cycle_stats->ac_infected_state[ST_SEVERE]++;

						this->infected_state = ST_SEVERE;
						this->setup_infection_countdown(cfg.cycles_severe_in_hospital);
					break;

					case ST_CRITICAL:
						cycle_stats->ac_infected_state[ST_MILD]--;
						cycle_stats->ac_infected_state[ST_CRITICAL]++;

						this->infected_state = ST_CRITICAL;
						this->setup_infection_countdown(cfg.cycles_critical_in_icu);
					break;

					default:
						C_ASSERT(0)
				}
			}
		break;

		case ST_CRITICAL:
			if (roll_dice(cfg.probability_death_per_cycle))
				this->die();
			else if (this->infection_countdown <= 0.0) {
				this->infected_state = ST_SEVERE;

				this->setup_infection_countdown(cfg.cycles_severe_in_hospital);
				
				cycle_stats->ac_infected_state[ ST_CRITICAL ]--;
				cycle_stats->ac_infected_state[ ST_SEVERE ]++;
			}
		break;

		default:
			C_ASSERT(0)
	}
}

void person_t::pre_infect ()
{
	this->state = ST_PRE_INFECTION;
	cycle_stats->new_infected++;

	cycle_stats->ac_healthy--;
	cycle_stats->ac_infected++;

	global_stats.infected++;

	this->setup_infection_countdown(cfg.cycles_pre_infection);
}

void person_t::infect ()
{
	double p;

	if (unlikely(this->state != ST_PRE_INFECTION))
		this->pre_infect();

	this->state = ST_INFECTED;

	p = generate_random_between_0_and_1();

	if (p <= cfg.prob_ac_asymptomatic) {
		this->infected_state = ST_ASYMPTOMATIC;
		this->setup_infection_countdown(cfg.cycles_contagious);
	}
	else if (p <= cfg.prob_ac_mild) {
		this->infected_state = ST_MILD;
		this->next_infected_state = ST_FAKE_IMMUNE;
		this->setup_infection_countdown(cfg.cycles_contagious);
	}
	else if (p <= cfg.prob_ac_severe) {
		this->infected_state = ST_MILD;
		this->next_infected_state = ST_SEVERE;
		this->setup_infection_countdown(cfg.cycles_before_hospitalization);
	}
	else {
		this->infected_state = ST_MILD;
		this->next_infected_state = ST_CRITICAL;
		this->setup_infection_countdown(cfg.cycles_before_hospitalization);
	}

	cycle_stats->infected_state[ this->infected_state ]++;
	cycle_stats->ac_infected_state[ this->infected_state ]++;
}

void person_t::cycle ()
{
	switch (this->state) {
		case ST_HEALTHY:
		break;

		case ST_PRE_INFECTION:
			this->infection_countdown -= 1.0;

			if (this->infection_countdown <= 0.0) {
				this->infection_countdown = 0.0;
				this->infect();
				//dprintf("I got IMMUNE\n");
			}
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
	this->r0 = 3.0;
	this->death_rate = 0.02;
	this->cycles_contagious = 4.0;
	this->population = 100000;
	this->cycles_to_simulate = 180;
	this->cycles_pre_infection = 4.0;
	this->cycles_severe_in_hospital = 8.0;
	this->cycles_critical_in_icu = 8.0;
	this->cycles_before_hospitalization = 5.0;
	this->hospital_beds = 70;
	this->icu_beds = 20;
	
	this->probability_asymptomatic = 0.85;
	this->probability_mild = 0.809 * (1.0 - this->probability_asymptomatic);
	this->probability_critical = 0.044 * (1.0 - this->probability_asymptomatic);

// flu
#if 0
	this->probability_asymptomatic = 1.0;
	this->probability_mild = 0.0;
	this->probability_critical = 0.0;
#endif

	this->r0_asymptomatic_factor = 1.0;

	this->load_derived();
}

void cfg_t::load_derived ()
{
	int32_t i;

	for (i=0; i<NUMBER_OF_INFECTED_STATES; i++)
		r0_factor_per_group[i] = 1.0;
	r0_factor_per_group[ST_ASYMPTOMATIC] = this->r0_asymptomatic_factor;

	this->probability_infect_per_cycle = this->r0 / this->cycles_contagious;
	this->probability_death_per_cycle = this->death_rate / this->cycles_contagious;
	this->probability_severe = 1.0 - (this->probability_asymptomatic + this->probability_mild + this->probability_critical);

	this->prob_ac_asymptomatic = this->probability_asymptomatic;
	this->prob_ac_mild = this->prob_ac_asymptomatic + this->probability_mild;
	this->prob_ac_severe = this->prob_ac_mild + this->probability_severe;
	this->prob_ac_critical = this->prob_ac_severe + this->probability_critical;
}

void cfg_t::dump ()
{
	dprintf("# ro = %0.4f\n", this->r0);
	dprintf("# death_rate = %0.4f\n", this->death_rate);
	dprintf("# cycles_contagious = %0.4f\n", this->cycles_contagious);
	dprintf("# cycles_pre_infection = %0.4f\n", this->cycles_pre_infection);
	dprintf("# population = " PU64 "\n", this->population);
	dprintf("# cycles_to_simulate = %u\n", this->cycles_to_simulate);
	dprintf("# probability_asymptomatic = %0.4f\n", this->probability_asymptomatic);
	dprintf("# probability_mild = %0.4f\n", this->probability_mild);
	dprintf("# probability_severe = %0.4f\n", this->probability_severe);
	dprintf("# probability_critical = %0.4f\n", this->probability_critical);

	dprintf("# probability_infect_per_cycle = %0.4f\n", this->probability_infect_per_cycle);
	dprintf("# probability_death_per_cycle = %0.4f\n", this->probability_death_per_cycle);

	dprintf("# prob_ac_asymptomatic = %0.4f\n", this->prob_ac_asymptomatic);
	dprintf("# prob_ac_mild = %0.4f\n", this->prob_ac_mild);
	dprintf("# prob_ac_severe = %0.4f\n", this->prob_ac_severe);
	dprintf("# prob_ac_critical = %0.4f\n", this->prob_ac_critical);
}

/****************************************************************/

static void simulate ()
{
	cycle_stats = all_cycle_stats;

	region->get_person(0)->infect();
	//cycle_stats->infected = 0; // will be considered during the cycle

	for (current_cycle=0; current_cycle<cfg.cycles_to_simulate; current_cycle++) {
		cprintf("Day %i\n", current_cycle);

		if (likely(current_cycle > 0))
			cycle_stats->copy_ac(prev_cycle_stats);

		region->cycle();

		prev_cycle_stats = cycle_stats++;
	}

	cycle_stats = prev_cycle_stats--;

	region->process_data();
}

/****************************************************************/

stats_t::stats_t ()
{
	this->reset();
}

void stats_t::reset ()
{
	#define CORONA_STAT(TYPE, PRINT, STAT, AC) this->STAT = 0;
	#define CORONA_STAT_VECTOR(TYPE, PRINT, LIST, STAT, N, AC) { int32_t i; for (i=0; i<N; i++) this->STAT[i] = 0; }
	#include "stats.h"
	#undef CORONA_STAT
	#undef CORONA_STAT_VECTOR
}

void stats_t::copy_ac (stats_t *from)
{
	#define CORONA_STAT(TYPE, PRINT, STAT, AC) if (AC == AC_STAT) this->STAT = from->STAT;
	#define CORONA_STAT_VECTOR(TYPE, PRINT, LIST, STAT, N, AC) if (AC == AC_STAT) { int32_t i; for (i=0; i<N; i++) this->STAT[i] = from->STAT[i]; }
	#include "stats.h"
	#undef CORONA_STAT
	#undef CORONA_STAT_VECTOR
}

void stats_t::dump ()
{
	#define CORONA_STAT(TYPE, PRINT, STAT, AC) cprintf(#STAT ":" PRINT " ", this->STAT);
	#define CORONA_STAT_VECTOR(TYPE, PRINT, LIST, STAT, N, AC) { int32_t i; for (i=0; i<N; i++) cprintf(#STAT ".%s:" PRINT " ", LIST##_str(i), this->STAT[i]); }
	#include "stats.h"
	#undef CORONA_STAT
	#undef CORONA_STAT_VECTOR

	cprintf("\n");
}

void stats_t::dump_csv_header (FILE *fp)
{
	fprintf(fp, "cycle,");

	#define CORONA_STAT(TYPE, PRINT, STAT, AC) fprintf(fp, #STAT ",");
	#define CORONA_STAT_VECTOR(TYPE, PRINT, LIST, STAT, N, AC) { int32_t i; for (i=0; i<N; i++) fprintf(fp, #STAT "_%s,", LIST##_str(i)); }
	#include "stats.h"
	#undef CORONA_STAT
	#undef CORONA_STAT_VECTOR

	fprintf(fp, "\n");
}

void stats_t::dump_csv (FILE *fp)
{
	fprintf(fp, "%u,", this->cycle);

	#define CORONA_STAT(TYPE, PRINT, STAT, AC) fprintf(fp, PRINT ",", this->STAT);
	#define CORONA_STAT_VECTOR(TYPE, PRINT, LIST, STAT, N, AC) { int32_t i; for (i=0; i<N; i++) fprintf(fp, PRINT ",", this->STAT[i]); }
	#include "stats.h"
	#undef CORONA_STAT
	#undef CORONA_STAT_VECTOR

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
	
	all_cycle_stats = new stats_t[ cfg.cycles_to_simulate ];

	for (i=0; i<cfg.cycles_to_simulate; i++)
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
	for (i=0; i<cfg.cycles_to_simulate; i++)
		all_cycle_stats[i].dump_csv(fp);

	fclose(fp);

	return 0;
}