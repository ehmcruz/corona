#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>

#include <deque>
#include <list>

#include <random>
#include <algorithm>

#include <corona.h>

cfg_t cfg;
stats_t *cycle_stats = nullptr;
static stats_t *prev_cycle_stats = nullptr;
static std::list<stats_t> all_cycle_stats;
region_t *region = nullptr;
double current_cycle;

std::vector<person_t*> population;
static std::deque<person_t*> to_infect_in_cycle;

double r0_factor_per_group[NUMBER_OF_INFECTED_STATES];

static const char default_results_file[] = "results-cycles.csv";

char* state_str (int32_t i)
{
	static const char *list[] = {
		"ST_HEALTHY",
		"ST_INFECTED",
		"ST_IMMUNE",
		"ST_DEAD"
	};

	C_ASSERT(i < NUMBER_OF_STATES)

	return (char*)list[i];
}

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

	std::shuffle(this->people.begin(), this->people.end(), rgenerator);

	C_ASSERT(this->people.size() > 0)
	C_ASSERT(this->people.size() == this->npopulation)

	for (i=0; i<this->npopulation; i++)
		this->people[i]->set_region(this);
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

	cycle_stats->ac_state[ST_HEALTHY] += n;
}

void region_t::set_population_number (uint64_t npopulation)
{
	this->npopulation = npopulation;
	this->people.reserve(npopulation);
}

void region_t::adjust_population_infection_state_rate_per_age (uint32_t *reported_deaths_per_age_)
{
	uint64_t people_per_age[AGE_CATS_N];
	uint32_t reported_deaths_per_age[AGE_CATS_N];
	double deaths_weight_per_age[AGE_CATS_N], sum, k;
	double predicted_critical_per_age[AGE_CATS_N];
	double pmild, psevere, pcritical;
	uint32_t i;

	for (i=0; i<AGE_CATS_N; i++)
		people_per_age[i] = 0;

	for (person_t *p: this->people) {
		people_per_age[ get_age_cat(p->get_age()) ]++;
	}

	for (i=0; i<AGE_CATS_N; i++) {
		if (people_per_age[i] == 0)
			reported_deaths_per_age[i] = 0;
		else
			reported_deaths_per_age[i] = reported_deaths_per_age_[i];
	}

	sum = 0.0;
	for (i=0; i<AGE_CATS_N; i++) {
		if (people_per_age[i])
			deaths_weight_per_age[i] = (double)reported_deaths_per_age[i] / (double)people_per_age[i];
		else
			deaths_weight_per_age[i] = 0.0;
		
		//sum += (double)deaths_weight_per_age[i] * (double)people_per_age[i];
		sum += (double)reported_deaths_per_age[i];
		
		dprintf("city %s people_per_age %s: " PU64 " death weight:%.4f\n", this->name.c_str(), critical_per_age_str(i), people_per_age[i], deaths_weight_per_age[i]);
	}

	/*
		(k * (sum weight*people_per_age) / total_people) = critical_rate
		k * (sum weight*people_per_age) = critical_rate * total_people
		k = (critical_rate * total_people) / (sum weight*people_per_age)
	*/

	k = (cfg.probability_critical * (double)this->get_npopulation()) / sum;

	dprintf("sum:%.4f k:%.4f\n", sum, k);

	sum = 0.0;
	for (i=0; i<AGE_CATS_N; i++) {
		deaths_weight_per_age[i] *= k;
		predicted_critical_per_age[i] = (double)people_per_age[i] * deaths_weight_per_age[i];
		sum += predicted_critical_per_age[i];

		dprintf("city %s people_per_age %s: " PU64 " death weight:%.4f critical:%2.f\n", this->name.c_str(), critical_per_age_str(i), people_per_age[i], deaths_weight_per_age[i], predicted_critical_per_age[i]);
	}

	dprintf("total critical predicted:%.4f sum:%.2f critical_rate:%.4f critical_rate_test:%.4f\n",
	       (cfg.probability_critical * (double)this->get_npopulation()),
	       sum,
	       cfg.probability_critical,
	       sum / (double)this->get_npopulation()
	       );
//exit(1);
	for (person_t *p: this->people) {
		pcritical = deaths_weight_per_age[ get_age_cat( p->get_age() ) ];
		//pcritical = cfg.probability_critical;
		psevere = pcritical * (cfg.probability_severe / cfg.probability_critical);
		pmild = pcritical * (cfg.probability_mild / cfg.probability_critical);

		p->setup_infection_probabilities(pmild, psevere, pcritical);
	}
}

void region_t::summon ()
{
	if (roll_dice(cfg.probability_summon_per_cycle)) {
		person_t *p;

		p = pick_random_person();

		if (p->get_state() == ST_HEALTHY)
			to_infect_in_cycle.push_back(p);
	}
}

void region_t::cycle ()
{
	uint64_t i;

#ifdef SANITY_CHECK
	uint64_t sanity_check = 0, sanity_check2 = 0;
#endif

#ifdef SANITY_CHECK
	sanity_check = 0;
	for (i=0; i<NUMBER_OF_STATES; i++)
		sanity_check += cycle_stats->ac_state[i];
	SANITY_ASSERT(population.size() == sanity_check)
	sanity_check = 0;
#endif

#ifdef SANITY_CHECK
	for (i=0; i<NUMBER_OF_INFECTED_STATES; i++)
		sanity_check2 += cycle_stats->ac_infected_state[i];

	SANITY_ASSERT(cycle_stats->ac_state[ST_INFECTED] == sanity_check2)

	sanity_check2 = 0;
#endif

	for (person_t* p: this->people) {
	#ifdef SANITY_CHECK
		sanity_check += (p->get_state() == ST_INFECTED);
	#endif

		p->cycle();
	}

	SANITY_ASSERT(sanity_check == prev_cycle_stats->ac_state[ST_INFECTED])

	this->summon();

	dprintf("must_infect_in_cycle: " PU64 "\n", (uint64_t)to_infect_in_cycle.size());

	for (person_t *p: to_infect_in_cycle) {
		C_ASSERT(p->get_state() == ST_HEALTHY || (p->get_state() == ST_INFECTED && p->get_infected_state() == ST_INCUBATION && p->get_infected_cycle() == current_cycle))

		if (p->get_state() == ST_HEALTHY)
			p->pre_infect();
	}

	to_infect_in_cycle.clear();

	for (i=0; i<NUMBER_OF_INFECTED_STATES; i++) {
		if (cycle_stats->ac_infected_state[i] > cycle_stats->peak[i])
			cycle_stats->peak[i] = cycle_stats->ac_infected_state[i];
		
		#ifdef SANITY_CHECK
			sanity_check2 += cycle_stats->ac_infected_state[i];
		#endif
	}

	SANITY_ASSERT(cycle_stats->ac_state[ST_INFECTED] == sanity_check2)

#ifdef SANITY_CHECK
	sanity_check = 0;
	for (i=0; i<NUMBER_OF_STATES; i++)
		sanity_check += cycle_stats->ac_state[i];
	SANITY_ASSERT(population.size() == sanity_check)
#endif
	
	for (i=0; i<AGE_CATS_N; i++) {
		if (cycle_stats->ac_critical_per_age[i] > cycle_stats->peak_critical_per_age[i])
			cycle_stats->peak_critical_per_age[i] = cycle_stats->ac_critical_per_age[i];
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
	this->infected_cycle = -1.0;

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
	C_ASSERT(this->state == ST_INFECTED && (this->infected_state == ST_SEVERE || this->infected_state == ST_CRITICAL))

	this->state = ST_DEAD;
	cycle_stats->state[ST_DEAD]++;

	cycle_stats->ac_state[ST_INFECTED]--;
	cycle_stats->ac_state[ST_DEAD]++;

	cycle_stats->ac_infected_state[ this->infected_state ]--;
}

void person_t::recover ()
{
	C_ASSERT(this->state == ST_INFECTED && (this->infected_state == ST_ASYMPTOMATIC || this->infected_state == ST_MILD || this->infected_state == ST_SEVERE))

	this->state = ST_IMMUNE;
	cycle_stats->state[ST_IMMUNE]++;

	cycle_stats->ac_state[ST_INFECTED]--;
	cycle_stats->ac_state[ST_IMMUNE]++;
	
	cycle_stats->ac_infected_state[ this->infected_state ]--;
}

void person_t::cycle_infected ()
{
	if (this->infected_state != ST_INCUBATION) {
		for (auto it = this->get_neighbor_list()->begin(); *it != nullptr; ++it) {
			if (it.check_probability())
				to_infect_in_cycle.push_back(*it);
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
	SANITY_ASSERT(this->state == ST_HEALTHY && this->infected_state == ST_NULL && this->infected_cycle == -1.0)

	this->infected_cycle = current_cycle;

	this->state = ST_INFECTED;
	cycle_stats->state[ST_INFECTED]++;

	this->infected_state = ST_INCUBATION;

	cycle_stats->infected_state[ ST_INCUBATION ]++;
	cycle_stats->ac_infected_state[ ST_INCUBATION ]++;

	cycle_stats->ac_state[ST_HEALTHY]--;
	cycle_stats->ac_state[ST_INFECTED]++;

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
	current_cycle = 0.0;
	region->callback_before_cycle(current_cycle);
	region->get_person(0)->pre_infect();
	region->get_person(0)->infect();
	region->callback_after_cycle(current_cycle);

	for (current_cycle=1.0; current_cycle<cfg.cycles_to_simulate; current_cycle+=1.0) {
		cprintf("Cycle %.2f\n", current_cycle);

		prev_cycle_stats = cycle_stats;

		all_cycle_stats.push_back(stats_t(current_cycle));
		cycle_stats = &all_cycle_stats.back();
		C_ASSERT(cycle_stats->cycle == current_cycle)

		cycle_stats->copy_ac(prev_cycle_stats);

		region->callback_before_cycle(current_cycle);
		region->cycle();
		region->callback_after_cycle(current_cycle);

		cycle_stats->dump();
		cprintf("\n");
	}

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

	for (auto it = region->get_person(0)->get_neighbor_list()->begin(); *it != nullptr; ++it) {
		cprintf("person id %u\n", (*it)->get_id());
	}

//exit(0);
}

static void load_stats_engine ()
{
	all_cycle_stats.push_back(stats_t(0.0));
	cycle_stats = &all_cycle_stats.back();
	C_ASSERT(cycle_stats->cycle == 0.0)
}

int main ()
{
	FILE *fp;

	cfg.dump();

	start_dice_engine();

	load_stats_engine();

	load_gdistribution_incubation(cfg.cycles_incubation_mean, cfg.cycles_incubation_stddev);

	load_region();

	simulate();

	fp = fopen(default_results_file, "w");
	C_ASSERT(fp != NULL)

	stats_t::dump_csv_header(fp);
	for (auto it=all_cycle_stats.begin(); it!=all_cycle_stats.end(); ++it)
		it->dump_csv(fp);

	fclose(fp);

	return 0;
}