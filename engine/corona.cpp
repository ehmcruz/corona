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
std::vector<person_t*> population;

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

char* critical_per_age_str (int32_t age_group)
{
	static const char *list[AGE_CATS_N] = {
		"00-09",
		"10-19",
		"20-29",
		"30-39",
		"40-49",
		"50-59",
		"60-69",
		"70-79",
		"80-89",
		"90+",
	};

	C_ASSERT(age_group < AGE_CATS_N)
	
	return (char*)list[age_group];
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

	for (person_t* p: this->people) {
	#ifdef SANITY_CHECK
		sanity_check += (p->get_state() == ST_INFECTED);
	#endif

		p->cycle();
	}

	SANITY_ASSERT(sanity_check == prev_cycle_stats->ac_infected)

	this->summon();

	dprintf("must_infect_in_cycle: " PU64 "\n", this->must_infect_in_cycle);

	j = 0;
	for (auto it=this->people.begin(); it!=this->people.end() && j<this->must_infect_in_cycle; ++it) {
		p = *it;

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
	
	for (i=0; i<AGE_CATS_N; i++) {
		if (cycle_stats->ac_critical_per_age[i] > cycle_stats->peak_critical_per_age[i])
			cycle_stats->peak_critical_per_age[i] = cycle_stats->ac_critical_per_age[i];
	}

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
	static uint32_t id_ = 0;

	this->id = id_++;

	this->state = ST_HEALTHY;
	this->infected_state = ST_NULL;
	this->infection_countdown = 0.0;
	this->infection_cycles = 0.0;
	this->region = NULL;
	this->age = 0;

	this->neighbor_list = new neighbor_list_fully_connected_t(this);
}

person_t::~person_t ()
{
	delete this->neighbor_list;
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
	
//	dprintf("age:%i pasymptomatic:%.4f pmild:%.4f psevere:%.4f pcritical:%.4f\n", this->age, this->probability_asymptomatic, this->probability_mild, this->probability_severe, this->probability_critical);
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
	#if 0
		double prob = (cfg.probability_infect_per_cycle * cfg.global_r0_factor
				         * r0_factor_per_group[ this->get_infected_state() ]) / population.size();
		for (auto it = this->get_neighbor_list()->begin(); *it != nullptr; ++it) {
			person_t *p = *it;
			if (unlikely(p->get_state() == ST_HEALTHY && roll_dice(prob))) {
				this->region->must_infect_in_cycle++;
			}
		}
	#else
		if (roll_dice(calculate_infection_probability(this))) {
			this->region->must_infect_in_cycle++;
		}
	#endif
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

						cycle_stats->ac_critical_per_age[ get_age_cat(this->age) ]++;

						this->infected_state = ST_CRITICAL;
						this->setup_infection_countdown(cfg.cycles_critical_in_icu);
					break;

					default:
						C_ASSERT(0)
				}
			}
		break;

		case ST_CRITICAL:
			// if (roll_dice(cfg.probability_death_per_cycle))
			// 	this->die();
			// else
			if (this->infection_countdown <= 0.0) {
				this->infected_state = ST_SEVERE;

				this->setup_infection_countdown(cfg.cycles_severe_in_hospital);
				
				cycle_stats->ac_infected_state[ ST_CRITICAL ]--;
				cycle_stats->ac_infected_state[ ST_SEVERE ]++;
//dprintf("blah %i e %i\n", this->age, get_age_cat(this->age));
				cycle_stats->ac_critical_per_age[ get_age_cat(this->age) ]--;
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

static void load_region()
{
	uint64_t i, total;

	region = new region_t();

	start_population_graph();

	region->add_to_population_graph();

	total = region->get_npopulation();
	population.reserve(total);

	for (i=0; i<total; i++)
		population.push_back(region->get_person(i));

	C_ASSERT(population.size() == total)

	neighbor_list_t::iterator_t it;

	for (it = region->get_person(0)->get_neighbor_list()->begin(); *it != nullptr; ++it) {
		cprintf("person id %u\n", (*it)->get_id());
	}

//exit(0);
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