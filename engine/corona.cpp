#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>

#include <deque>
#include <list>

#include <random>
#include <algorithm>
#include <chrono>

#include <corona.h>

cfg_t cfg;
stats_t *cycle_stats = nullptr;
static stats_t *prev_cycle_stats = nullptr;
static std::list<stats_t> all_cycle_stats;
std::vector<region_t*> regions;
double current_cycle;

uint64_t people_per_age[AGES_N];

std::vector<person_t*> population;
static std::deque<person_t*> to_infect_in_cycle;

double r0_factor_per_group[NUMBER_OF_INFECTED_STATES];

static const char default_results_file[] = "results-cycles.csv";
static char *results_file;

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
		"00_09",
		"10_19",
		"20_29",
		"30_39",
		"40_49",
		"50_59",
		"60_69",
		"70_79",
		"80_89",
		"90_",
	};

	C_ASSERT(age_group < AGE_CATS_N)
	
	return (char*)list[age_group];
}

char* relation_type_str (int32_t i)
{
	static const char *list[] = {
		"F",
		"B",
		"U",
		"S",
		"T",
		"O",
	};

	C_ASSERT(i < NUMBER_OF_RELATIONS)

	return (char*)list[i];
}

/****************************************************************/

bool try_to_summon ()
{
	bool r = false;

	if (roll_dice(cfg.probability_summon_per_cycle)) {
		person_t *p;

		p = pick_random_person();

		if (p->get_state() == ST_HEALTHY) {
			to_infect_in_cycle.push_back(p);
			r = true;
		}
	}

	return r;
}

static void cycle ()
{
	uint64_t i;

#ifdef SANITY_CHECK
	uint64_t sanity_check = 0, sanity_check2 = 0, sanity_critical = 0;
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

	for (person_t *p: population) {
	#ifdef SANITY_CHECK
		sanity_check += (p->get_state() == ST_INFECTED);
	#endif

		p->cycle();

	#ifdef SANITY_CHECK
		sanity_critical += (p->get_state() == ST_INFECTED && p->get_infected_state() == ST_CRITICAL);
	#endif
	}

	SANITY_ASSERT(sanity_check == prev_cycle_stats->ac_state[ST_INFECTED])

	try_to_summon();

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

#ifdef SANITY_CHECK
{
	uint64_t sanity_check = 0;
	for (i=0; i<AGE_CATS_N; i++)
		sanity_check += cycle_stats->ac_critical_per_age[i];
	SANITY_ASSERT( sanity_critical == sanity_check )
}
#endif
	
	for (i=0; i<AGE_CATS_N; i++) {
		if (cycle_stats->ac_critical_per_age[i] > cycle_stats->peak_critical_per_age[i])
			cycle_stats->peak_critical_per_age[i] = cycle_stats->ac_critical_per_age[i];
	}
}

/****************************************************************/

region_t::region_t (uint32_t id)
{
	uint64_t i;

	this->id = id;

	this->npopulation = 0;

	for (i=0; i<AGES_N; i++)
		this->region_people_per_age[i] = 0;

	this->setup_population();

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

	if (unlikely(age >= AGES_N))
		age = AGES_N - 1;

	C_ASSERT( (this->people.size() + n) <= this->npopulation )

	p = new person_t[n];

	for (i=0; i<n; i++) {
		p[i].set_age(age);
		this->people.push_back( p+i );
	}

	people_per_age[age] += n;
	this->region_people_per_age[age] += n;

	cycle_stats->ac_state[ST_HEALTHY] += n;
}

void region_t::set_population_number (uint64_t npopulation)
{
	this->npopulation = npopulation;
	this->people.reserve(npopulation);
}

person_t* region_t::pick_random_person ()
{
	std::uniform_int_distribution<uint64_t> distribution(0, this->people.size()-1);

	return this->people[ distribution(rgenerator) ];
}

void region_t::adjust_population_infection_state_rate_per_age (uint32_t *reported_deaths_per_age)
{
	uint64_t people_per_age[AGE_CATS_N];
	double deaths_weight_per_age[AGE_CATS_N];
	double predicted_critical_per_age[AGE_CATS_N];
	double pmild, psevere, pcritical, sum;
	uint32_t i;

	for (i=0; i<AGE_CATS_N; i++)
		people_per_age[i] = 0;

	for (person_t *p: this->people) {
		people_per_age[ get_age_cat(p->get_age()) ]++;
	}

	adjust_biased_weights_to_fit_mean<uint32_t, uint64_t, AGE_CATS_N> (
		reported_deaths_per_age,
		people_per_age,
		cfg.probability_critical * (double)this->get_npopulation(),
		deaths_weight_per_age
		);

	/*
		(k * (sum weight*people_per_age) / total_people) = critical_rate
		k * (sum weight*people_per_age) = critical_rate * total_people
		k = (critical_rate * total_people) / (sum weight*people_per_age)
	*/

	sum = 0.0;
	for (i=0; i<AGE_CATS_N; i++) {
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

health_unit_t* region_t::enter_health_unit (person_t *p)
{
	C_ASSERT(p->get_state() == ST_INFECTED && (p->get_infected_state() == ST_SEVERE || p->get_infected_state() == ST_CRITICAL))

	for (auto it=this->health_units.begin(); it!=this->health_units.end(); ++it) {
		health_unit_t *hu = *it;
		if (p->get_infected_state() == hu->get_type()) {
			if (hu->enter(p))
				return hu;
		}
	}

	return nullptr;
}

region_t* region_t::get (std::string& name)
{
	auto it = std::find_if(regions.begin(), regions.end(), [name] (region_t *r) -> bool { return (r->get_name() == name); } );

	C_ASSERT(it != regions.end())
	//C_ASSERT((*it)->get_name() == name)

	return *it;
}

/****************************************************************/

health_unit_t::health_unit_t (uint32_t n_units, infected_state_t type)
{
	this->n_units = n_units;
	this->type = type;
	this->n_occupied = 0;
}

bool health_unit_t::enter (person_t *p)
{
	bool r;

	if (this->n_occupied < this->n_units) {
		r = true;

		C_ASSERT(p->get_health_unit() == nullptr)
		C_ASSERT(p->get_infected_state() == this->type)

		this->n_occupied++;
		p->set_health_unit(this);
	}
	else
		r = false;

	return r;
}

void health_unit_t::leave (person_t *p)
{
	C_ASSERT(p->get_health_unit() == this)
	C_ASSERT(this->n_occupied > 0)
	this->n_occupied--;
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

	this->setup_infection_probabilities(cfg.probability_mild, cfg.probability_severe, cfg.probability_critical);

	this->health_unit = nullptr;
	this->neighbor_list = nullptr;
}

person_t::~person_t ()
{
	this->neighbor_list = nullptr;
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

	if (this->infected_state == ST_CRITICAL)
		cycle_stats->ac_critical_per_age[ get_age_cat(this->age) ]--;

	this->state = ST_DEAD;
	cycle_stats->state[ST_DEAD]++;

	cycle_stats->ac_state[ST_INFECTED]--;
	cycle_stats->ac_state[ST_DEAD]++;

	cycle_stats->ac_infected_state[ this->infected_state ]--;

	if (this->health_unit != nullptr) {
		this->health_unit->leave(this);
		this->health_unit = nullptr;
	}
}

void person_t::recover ()
{
	C_ASSERT(this->state == ST_INFECTED && (this->infected_state == ST_ASYMPTOMATIC || this->infected_state == ST_MILD || this->infected_state == ST_SEVERE))

	this->state = ST_IMMUNE;
	cycle_stats->state[ST_IMMUNE]++;

	cycle_stats->ac_state[ST_INFECTED]--;
	cycle_stats->ac_state[ST_IMMUNE]++;
	
	cycle_stats->ac_infected_state[ this->infected_state ]--;

	if (this->health_unit != nullptr) {
		this->health_unit->leave(this);
		this->health_unit = nullptr;
	}
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

						this->health_unit = this->region->enter_health_unit(this);

						this->setup_infection_countdown(cfg.cycles_severe_in_hospital);
					break;

					case ST_CRITICAL:
						cycle_stats->ac_infected_state[ST_MILD]--;
						cycle_stats->ac_infected_state[ST_CRITICAL]++;

						cycle_stats->ac_critical_per_age[ get_age_cat(this->age) ]++;

						this->infected_state = ST_CRITICAL;

						this->health_unit = this->region->enter_health_unit(this);

						this->setup_infection_countdown(cfg.cycles_critical_in_icu);
					break;

					default:
						C_ASSERT(0)
				}
			}
		break;

		case ST_SEVERE:
			if (this->health_unit != nullptr && roll_dice(cfg.death_rate_severe_in_hospital_per_cycle))
				this->die();
			else if (this->health_unit == nullptr && roll_dice(cfg.death_rate_severe_outside_hospital_per_cycle))
				this->die();
			else if (this->infection_countdown <= 0.0) {
				this->infection_countdown = 0.0;
				this->recover();
			}
			else if (this->health_unit == nullptr) // yeah, I know I reapeat this condition, but it is easier this way
				this->health_unit = this->region->enter_health_unit(this);
		break;

		case ST_CRITICAL:
			if (this->health_unit != nullptr && roll_dice(cfg.death_rate_critical_in_hospital_per_cycle))
				this->die();
			else if (this->health_unit == nullptr && roll_dice(cfg.death_rate_critical_outside_hospital_per_cycle))
				this->die();
			else if (this->infection_countdown <= 0.0) {
				this->infected_state = ST_SEVERE;

				this->setup_infection_countdown(cfg.cycles_severe_in_hospital);
				
				cycle_stats->ac_infected_state[ ST_CRITICAL ]--;
				cycle_stats->ac_infected_state[ ST_SEVERE ]++;
//dprintf("blah %i e %i\n", this->age, get_age_cat(this->age));
				cycle_stats->ac_critical_per_age[ get_age_cat(this->age) ]--;
			}
			else if (this->health_unit == nullptr) // yeah, I know I reapeat this condition, but it is easier this way
				this->health_unit = this->region->enter_health_unit(this);
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

	this->setup_infection_countdown( cfg.cycles_incubation->generate() );
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

	callback_before_cycle(current_cycle);
	callback_after_cycle(current_cycle);

	for (current_cycle=1.0; current_cycle<cfg.cycles_to_simulate; current_cycle+=1.0) {
		cprintf("Cycle %.2f\n", current_cycle);

		prev_cycle_stats = cycle_stats;

		all_cycle_stats.push_back(stats_t(current_cycle));
		cycle_stats = &all_cycle_stats.back();
		C_ASSERT(cycle_stats->cycle == current_cycle)

		cycle_stats->copy_ac(prev_cycle_stats);

		callback_before_cycle(current_cycle);

		cycle();
		
		callback_after_cycle(current_cycle);

		cycle_stats->dump();
		cprintf("\n");
	}

	callback_end();
}

/****************************************************************/

static void load_regions ()
{
	uint64_t i, total;

	regions.reserve( cfg.n_regions );

	for (i=0; i<cfg.n_regions; i++)
		regions.push_back( new region_t(i) );

	total = 0;
	for (region_t *region: regions)
		total += region->get_npopulation();

	dprintf("total population size is " PU64 "\n", total);
//exit(1);

	population.reserve(total);

	for (region_t *region: regions) {
		for (i=0; i<region->get_npopulation(); i++)
			population.push_back(region->get_person(i));
	}

	C_ASSERT(population.size() == total)

	if (cfg.network_type == NETWORK_TYPE_FULLY_CONNECTED) {
		neighbor_list_fully_connected_t *v;
		v = new neighbor_list_fully_connected_t[ population.size() ];

		for (i=0; i<population.size(); i++) {
			v[i].set_person( population[i] );
			population[i]->set_neighbor_list( v+i );
		}
	}
	else if (cfg.network_type == NETWORK_TYPE_NETWORK) {
		neighbor_list_network_t *v;
		v = new neighbor_list_network_t[ population.size() ];

		for (i=0; i<population.size(); i++) {
			v[i].set_person( population[i] );
			population[i]->set_neighbor_list( v+i );
		}
	}
	else {
		C_ASSERT(0)
	}

	for (region_t *region: regions)
		region->setup_health_units();

	network_start_population_graph();

	for (region_t *region: regions)
		region->setup_relations();

	setup_inter_region_relations();

	network_after_all_connetions();

#if 0
{
	extern double blah;
	for (person_t *p: population) {
		auto it = p->get_neighbor_list()->begin();
	}
cprintf("blah %.2f\n", blah / (double)population.size()); exit(1);
}
#endif

/*	for (auto it = region->get_person(0)->get_neighbor_list()->begin(); *it != nullptr; ++it) {
		cprintf("person id %u\n", (*it)->get_id());
	}*/

//exit(0);
}

static void load_stats_engine ()
{
	all_cycle_stats.push_back(stats_t(0.0));
	cycle_stats = &all_cycle_stats.back();
	C_ASSERT(cycle_stats->cycle == 0.0)
}

int main (int argc, char **argv)
{
	FILE *fp;
	std::chrono::steady_clock::time_point tbegin, tbefore_sim, tend;
	int64_t i;

	if (argc == 1)
		results_file = (char*)default_results_file;
	else if (argc == 2)
		results_file = argv[1];
	else {
		cprintf("Usage: %s <results_file>\n", argv[0]);
		exit(1);
	}

	for (i=0; i<AGES_N; i++)
		people_per_age[i] = 0;

	tbegin = std::chrono::steady_clock::now();

	start_dice_engine();

	load_stats_engine();

	load_regions();

	cfg.dump();

	tbefore_sim = std::chrono::steady_clock::now();

	simulate();

	tend = std::chrono::steady_clock::now();

	fp = fopen(results_file, "w");
	C_ASSERT(fp != NULL)

	all_cycle_stats.front().dump_csv_header(fp);
	for (auto it=all_cycle_stats.begin(); it!=all_cycle_stats.end(); ++it)
		it->dump_csv(fp);

	fclose(fp);

	std::chrono::duration<double> time_load = std::chrono::duration_cast<std::chrono::duration<double>>(tbefore_sim - tbegin);
	std::chrono::duration<double> time_sim = std::chrono::duration_cast<std::chrono::duration<double>>(tend - tbefore_sim);

	cprintf("load time: %.2fs\n", time_load.count());
	cprintf("simulation time: %.2fs\n", time_sim.count());

	return 0;
}