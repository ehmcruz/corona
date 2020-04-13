#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <random>
#include <algorithm>

#include <corona.h>

cfg_t cfg;
stats_t *all_cycle_stats;
stats_t *cycle_stats;
stats_t *prev_cycle_stats = NULL;
region_t *region = NULL;
uint32_t current_cycle;

double r0_factor_per_group[NUMBER_OF_INFECTED_STATES];

static const char default_results_file[] = "results-cycles.csv";

char* infected_state_str (int32_t i)
{
	static const char *list[] = {
		"ST_INCUBATION",
		"ST_ASYMPTOMATIC",
		"ST_MILD",
		"ST_SEVERE",
		"ST_CRITICAL"
	};

	C_ASSERT(i < NUMBER_OF_INFECTED_STATES)

	return (char*)list[i];
}

/****************************************************************/

region_t::region_t ()
{
	uint64_t i;

	this->npopulation = 0;

	this->setup_region();

	C_ASSERT(this->people.size() > 0)
	C_ASSERT(this->people.size() == this->npopulation)

	cycle_stats->ac_healthy += this->npopulation;

	for (i=0; i<this->npopulation; i++)
		this->people[i]->set_region(this);

	this->must_infect_in_cycle = 0;

	this->sir_init();
}

void region_t::add_people (uint64_t n, uint32_t age)
{
	uint64_t i;
	person_t *p;

	C_ASSERT( (this->people.size() + n) <= this->npopulation )

	p = new person_t[n];

	for (i=0; i<n; i++) {
		p[i].set_age(age);
		this->people.push_back( p+i );
	}
}

void region_t::set_population_number (uint64_t npopulation)
{
	this->npopulation = npopulation;
	this->people.reserve(npopulation);
}

void region_t::summon ()
{
	if (roll_dice(cfg.probability_summon_per_cycle))
		this->must_infect_in_cycle++;
}

void region_t::cycle ()
{
	uint64_t i, j;
	person_t *p;
#ifdef SANITY_CHECK
	uint64_t sanity_check = 0, sanity_check2 = 0;
#endif

#ifdef SANITY_CHECK
	for (i=0; i<NUMBER_OF_INFECTED_STATES; i++)
		sanity_check2 += cycle_stats->ac_infected_state[i];

	SANITY_ASSERT(cycle_stats->ac_infected == sanity_check2)

	sanity_check2 = 0;
#endif

	this->must_infect_in_cycle = 0;

	for (i=0; i<this->npopulation; i++) {
		p = this->people[i];

	#ifdef SANITY_CHECK
		sanity_check += (p->get_state() == ST_INFECTED);
	#endif

		p->cycle();
	}

	SANITY_ASSERT(sanity_check == prev_cycle_stats->ac_infected)

	this->summon();

	dprintf("must_infect_in_cycle: " PU64 "\n", this->must_infect_in_cycle);

	for (i=0, j=0; i<this->npopulation && j<this->must_infect_in_cycle; i++) {
		p = this->people[i];

		if (p->get_state() == ST_HEALTHY) {
			p->pre_infect();
			j++;
		}
	}

	for (i=0; i<NUMBER_OF_INFECTED_STATES; i++) {
		if (cycle_stats->ac_infected_state[i] > cycle_stats->peak[i])
			cycle_stats->peak[i] = cycle_stats->ac_infected_state[i];
		
		#ifdef SANITY_CHECK
			sanity_check2 += cycle_stats->ac_infected_state[i];
		#endif
	}

	SANITY_ASSERT(cycle_stats->ac_infected == sanity_check2)

	this->sir_calc();

	cycle_stats->dump();

	cprintf("\n");
}

void region_t::sir_init ()
{
	cycle_stats->sir_s = (double)(this->npopulation - 1) / (double)this->npopulation;
	cycle_stats->sir_i = 1.0 - cycle_stats->sir_s;
	cycle_stats->sir_r = 0.0;
}

void region_t::sir_calc ()
{
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

void region_t::process_data ()
{
	int32_t i;
	stats_t *cycle_stats = all_cycle_stats;

	for (i=0; i<cfg.cycles_to_simulate; i++) {
		cycle_stats->sir_s *= (double)this->npopulation;
		cycle_stats->sir_i *= (double)this->npopulation;
		cycle_stats->sir_r *= (double)this->npopulation;

		cycle_stats++;
	}
}

/****************************************************************/

person_t::person_t ()
{
	this->state = ST_HEALTHY;
	this->infected_state = ST_NULL;
	this->infection_countdown = 0.0;
	this->infection_cycles = 0.0;
	this->region = NULL;
	this->age = 0;
}

void person_t::setup_infection_probabilities (double pmild, double psevere, double pcritical)
{
	this->probability_asymptomatic = 1.0 - (pmild + psevere + pcritical);
	this->probability_mild = pmild;
	this->probability_severe = psevere;
	this->probability_critical = pcritical;

	this->prob_ac_asymptomatic = this->probability_asymptomatic;
	this->prob_ac_mild = this->prob_ac_asymptomatic + this->probability_mild;
	this->prob_ac_severe = this->prob_ac_mild + this->probability_severe;
	this->prob_ac_critical = this->prob_ac_severe + this->probability_critical;
	
//	dprintf("age:%i pasymptomatic:%.4f pmild:%.4f psevere:%.4f pcritical:%.4f ", this->age, this->probability_asymptomatic, this->probability_mild, this->probability_severe, this->probability_critical);
//	dprintf("ac_pasymptomatic:%.4f ac_pmild:%.4f ac_psevere:%.4f ac_pcritical:%.4f\n", this->prob_ac_asymptomatic, this->prob_ac_mild, this->prob_ac_severe, this->prob_ac_critical);
}

void person_t::die ()
{
	this->state = ST_DEAD;
	cycle_stats->deaths++;

	cycle_stats->ac_infected--;
	cycle_stats->ac_deaths++;

	cycle_stats->ac_infected_state[ this->infected_state ]--;
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
	if (this->infected_state != ST_INCUBATION) {
		if (roll_dice(calculate_infection_probability(this))) {
			this->region->must_infect_in_cycle++;
		}
	}

	this->infection_countdown -= 1.0;

	switch (this->infected_state) {
		case ST_INCUBATION:
			if (this->infection_countdown <= 0.0) {
				this->infection_countdown = 0.0;
				this->infect();
			}
		break;

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
	SANITY_ASSERT(this->state == ST_HEALTHY && this->infected_state == ST_NULL)

	this->state = ST_INFECTED;
	cycle_stats->infected++;

	this->infected_state = ST_INCUBATION;

	cycle_stats->infected_state[ ST_INCUBATION ]++;
	cycle_stats->ac_infected_state[ ST_INCUBATION ]++;

	cycle_stats->ac_healthy--;
	cycle_stats->ac_infected++;

	this->setup_infection_countdown( calculate_incubation_cycles() );
}

void person_t::infect ()
{
	double p;

	SANITY_ASSERT(this->state == ST_INFECTED && this->infected_state == ST_INCUBATION)

	p = generate_random_between_0_and_1();

	if (p <= this->prob_ac_asymptomatic) {
		this->infected_state = ST_ASYMPTOMATIC;
		this->setup_infection_countdown(cfg.cycles_contagious);
	}
	else if (p <= this->prob_ac_mild) {
		this->infected_state = ST_MILD;
		this->next_infected_state = ST_FAKE_IMMUNE;
		this->setup_infection_countdown(cfg.cycles_contagious);
	}
	else if (p <= this->prob_ac_severe) {
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

	cycle_stats->ac_infected_state[ ST_INCUBATION ]--;
	cycle_stats->ac_infected_state[ this->infected_state ]++;
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
	this->scenery_setup();
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
	
	dprintf("# cycles_incubation_mean = %0.4f\n", this->cycles_incubation_mean);
	dprintf("# cycles_incubation_stddev = %0.4f\n", this->cycles_incubation_stddev);
	
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
	current_cycle = 0;
	region->callback_before_cycle(current_cycle);
	region->get_person(0)->pre_infect();
	region->get_person(0)->infect();
	region->callback_after_cycle(current_cycle);

	for (current_cycle=1; current_cycle<cfg.cycles_to_simulate; current_cycle++) {
		cprintf("Day %i\n", current_cycle);

		prev_cycle_stats = cycle_stats++;

		cycle_stats->copy_ac(prev_cycle_stats);

		region->callback_before_cycle(current_cycle);
		region->cycle();
		region->callback_after_cycle(current_cycle);
	}

	region->process_data();
	region->callback_end();
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

/****************************************************************/

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

	cycle_stats = all_cycle_stats;
}

int main ()
{
	int32_t i;
	FILE *fp;

	cfg.dump();

	start_dice_engine();

	load_stats_engine();

	load_gdistribution_incubation(cfg.cycles_incubation_mean, cfg.cycles_incubation_stddev);

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