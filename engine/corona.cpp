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

cfg_t *cfg;

std::vector<stats_t> *cycle_stats_ptr = nullptr;
static std::vector<stats_t> *prev_cycle_stats_ptr = nullptr;
static std::list< std::vector<stats_t> > all_cycles_stats;
static std::vector<stats_zone_t> stats_zone_list;

static std::chrono::steady_clock::time_point tbegin, tbefore_sim, tend;

#define prev_cycle_stats (*prev_cycle_stats_ptr)

std::vector<region_t*> regions;
double current_cycle = 0.0;

uint64_t people_per_age[AGES_N];
uint64_t people_per_age_cat[AGE_CATS_N];

std::vector<person_t*> population;

static std::deque< std::pair<person_t*, person_t*> > to_infect_in_cycle;
static std::deque< person_t* > to_recover_die_in_cycle;

static std::vector<person_t*> pop_infected;
static uint32_t pop_infected_n = 0;

static const char default_results_file[] = "results-cycles";
static const char *results_file;

static global_stats_t global_stats;

std::string scenery_results_fname;

char* state_str (int32_t i)
{
	static const char *list[] = {
		"ST_HEALTHY",
		"ST_INFECTED",
		"ST_IMMUNE",
		"ST_DEAD"
	};

	static_assert((sizeof(list) / sizeof(*list)) == NUMBER_OF_STATES);

	C_ASSERT(i < NUMBER_OF_STATES)

	return (char*)list[i];
}

char* infected_state_str (int32_t i)
{
	static const char *list[] = {
		"ST_INCUBATION",
		"ST_ASYMPTOMATIC",
		"ST_PRESYMPTOMATIC",
		"ST_MILD",
		"ST_SEVERE",
		"ST_CRITICAL"
	};

	static_assert((sizeof(list) / sizeof(*list)) == NUMBER_OF_INFECTED_STATES);

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

	static_assert((sizeof(list) / sizeof(*list)) == AGE_CATS_N);

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
		"S0",
		"S1",
		"S2",
		"S3",
		"S4",
		"W",
		"W0",
		"W1",
		"T",
		"O",
	};

	static_assert((sizeof(list) / sizeof(*list)) == NUMBER_OF_RELATIONS);

	C_ASSERT(i < NUMBER_OF_RELATIONS)

	return (char*)list[i];
}

static std::string factor_per_relation_group_str_matrix[NUMBER_OF_RELATIONS][NUMBER_OF_INFECTED_STATES];

static void load_factor_per_relation_group_str ()
{
	for (uint32_t r=0; r<NUMBER_OF_RELATIONS; r++) {
		for (uint32_t i=0; i<NUMBER_OF_INFECTED_STATES; i++) {
			std::string& s = factor_per_relation_group_str_matrix[r][i];

			s = relation_type_str(r);
			s += "_";
			s += infected_state_str(i);
		}
	}
}

char* factor_per_relation_group_str (int32_t relation, int32_t group)
{
	C_ASSERT(relation < NUMBER_OF_RELATIONS)
	C_ASSERT(group < NUMBER_OF_INFECTED_STATES)

	return (char*)factor_per_relation_group_str_matrix[relation][group].c_str();
}

/****************************************************************/

bool try_to_summon ()
{
	bool r = false;

	if (roll_dice(cfg->probability_summon_per_cycle_step)) {
		person_t *p;

		p = pick_random_person();

		if (p->get_state() == ST_HEALTHY) {
			to_infect_in_cycle.push_back( std::make_pair(nullptr, p) );
			r = true;
		}
	}

	return r;
}

static void rm_infected (person_t *p)
{
	C_ASSERT(pop_infected_n > 0)

	pop_infected[ p->get_infected_state_vec_pos() ] = pop_infected[ --pop_infected_n ];
	pop_infected[ p->get_infected_state_vec_pos() ]->set_infected_state_vec_pos( p->get_infected_state_vec_pos() );
	pop_infected[ pop_infected_n ] = nullptr;
}

static void run_cycle_step ()
{
	uint64_t i;

#ifdef SANITY_CHECK
	uint64_t sanity_check_infected, sanity_critical;
#endif

//dprintf("tracker -------------------- %.2f\n", current_cycle);
//dprintf("tracker list"); for (person_t *p: pop_infected) { if (!p) break; dprintf("%u, ", p->get_id()); } dprintf("\n");

#ifdef SANITY_CHECK
{
	uint64_t sanity_check;

	for (stats_t& stats: cycle_stats) {
		sanity_check = 0;

		for (i=0; i<NUMBER_OF_STATES; i++)
			sanity_check += stats.ac_state[i];
		
		SANITY_ASSERT(stats.zone->get_population_size() == sanity_check)
	}
}
#endif

#ifdef SANITY_CHECK
{
	uint64_t sanity_check;

	for (stats_t& stats: cycle_stats) {
		sanity_check = 0;

		for (i=0; i<NUMBER_OF_INFECTED_STATES; i++)
			sanity_check += stats.ac_infected_state[i];

		SANITY_ASSERT(stats.ac_state[ST_INFECTED] == sanity_check)
	}
}
#endif

#ifdef SANITY_CHECK
	sanity_check_infected = 0;
	sanity_critical = 0;
#endif

#ifdef SANITY_CHECK
	for (person_t *p: population) {
		if (p->get_state() == ST_INFECTED) {
			C_ASSERT_PRINTF( p->get_infected_state_vec_pos() < pop_infected_n, "failed id=%u\n", p->get_id() )
			C_ASSERT_PRINTF( pop_infected[ p->get_infected_state_vec_pos() ] == p, "failed id=%u\n", p->get_id() )
		}
	}
#endif

#if 1
	for (person_t *p: pop_infected) {
		if (unlikely(p == nullptr))
			break;
		//dprintf("tracker cycle pid %i\n", p->get_id());

	#ifdef SANITY_CHECK
		sanity_check_infected += (p->get_state() == ST_INFECTED);
	#endif

		p->cycle_infected();

	#ifdef SANITY_CHECK
		sanity_critical += (p->get_state() == ST_INFECTED && p->get_infected_state() == ST_CRITICAL);
	#endif
	}
#else
	for (person_t *p: population) {
		//dprintf("tracker cycle pid %i\n", p->get_id());

	#ifdef SANITY_CHECK
		sanity_check_infected += (p->get_state() == ST_INFECTED);
	#endif

		p->cycle();

	#ifdef SANITY_CHECK
		sanity_critical += (p->get_state() == ST_INFECTED && p->get_infected_state() == ST_CRITICAL);
	#endif
	}
#endif

	//SANITY_ASSERT(sanity_check_infected == prev_cycle_stats[GLOBAL_STATS].ac_state[ST_INFECTED])

	for (person_t *p: to_recover_die_in_cycle) {
		//dprintf("tracker remove pid %i\n", p->get_id());
		rm_infected(p);
	}

	to_recover_die_in_cycle.clear();

	//dprintf("tracker list"); for (person_t *p: pop_infected) { if (!p) break; dprintf("%u, ", p->get_id()); } dprintf("\n");

	try_to_summon();

	dprintf("must_infect_in_cycle: " PU64 "\n", (uint64_t)to_infect_in_cycle.size());

	for (auto& infection: to_infect_in_cycle) {
		person_t *p = infection.second;

		C_ASSERT(p->get_state() == ST_HEALTHY || (p->get_state() == ST_INFECTED && p->get_infected_state() == ST_INCUBATION && p->get_infected_cycle() == current_cycle))

		if (p->get_state() == ST_HEALTHY)
			p->pre_infect(infection.first);
	}

	to_infect_in_cycle.clear();

	for (stats_t& stats: cycle_stats) {
	#ifdef SANITY_CHECK
		sanity_check_infected = 0;
	#endif

		for (i=0; i<NUMBER_OF_INFECTED_STATES; i++) {
			if (stats.ac_infected_state[i] > stats.peak[i])
				stats.peak[i] = stats.ac_infected_state[i];
			
			#ifdef SANITY_CHECK
				sanity_check_infected += stats.ac_infected_state[i];
			#endif
		}

		SANITY_ASSERT(stats.ac_state[ST_INFECTED] == sanity_check_infected)
	}

#ifdef SANITY_CHECK
{
	uint64_t sanity_check;

	for (stats_t& stats: cycle_stats) {
		sanity_check = 0;

		for (i=0; i<NUMBER_OF_STATES; i++)
			sanity_check += stats.ac_state[i];
		
		SANITY_ASSERT(stats.zone->get_population_size() == sanity_check)
	}
}
#endif

#ifdef SANITY_CHECK
{
	uint64_t sanity_check = 0;
	
	for (i=0; i<AGE_CATS_N; i++)
		sanity_check += cycle_stats[GLOBAL_STATS].ac_critical_per_age[i];
	
	SANITY_ASSERT( sanity_critical == sanity_check )
}
#endif
	
	for (i=0; i<AGE_CATS_N; i++) {
		if (cycle_stats[GLOBAL_STATS].ac_critical_per_age[i] > cycle_stats[GLOBAL_STATS].peak_critical_per_age[i])
			cycle_stats[GLOBAL_STATS].peak_critical_per_age[i] = cycle_stats[GLOBAL_STATS].ac_critical_per_age[i];
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

	for (i=0; i<AGE_CATS_N; i++)
		this->region_people_per_age_cat[i] = 0;

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

	people_per_age_cat[ get_age_cat(age) ] += n;
	this->region_people_per_age_cat[ get_age_cat(age) ] += n;
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
	this->adjust_population_infection_state_rate_per_age(reported_deaths_per_age, this->region_people_per_age_cat);
}

void region_t::adjust_population_infection_state_rate_per_age (uint32_t *reported_deaths_per_age, uint64_t *people_per_age)
{
	double deaths_weight_per_age[AGE_CATS_N];
	double predicted_critical_per_age[AGE_CATS_N];
	double pmild, psevere, pcritical, sum;
	uint32_t i;

	for (i=0; i<AGE_CATS_N; i++)
		dprintf("people_per_age[%s] = " PU64 "\n", critical_per_age_str(i), people_per_age[i]);

	adjust_biased_weights_to_fit_mean<uint32_t, uint64_t, AGE_CATS_N> (
		reported_deaths_per_age,
		people_per_age,
		cfg->probability_critical * (double)this->get_npopulation(),
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
	       (cfg->probability_critical * (double)this->get_npopulation()),
	       sum,
	       cfg->probability_critical,
	       sum / (double)this->get_npopulation()
	       );
//if (this->name == "Paranavai") exit(1);
	for (person_t *p: this->people) {
		pcritical = deaths_weight_per_age[ get_age_cat( p->get_age() ) ];
		//pcritical = cfg->probability_critical;
		psevere = pcritical * (cfg->probability_severe / cfg->probability_critical);
		pmild = pcritical * (cfg->probability_mild / cfg->probability_critical);

		double sum = pmild + psevere + pcritical;

		#define max_prob 0.99

		if (sum >= max_prob) {
			/*
				sum        ->     max_prob
				pcritical  ->     new_pcritical
			*/
			double r = max_prob / sum;
			pmild *= r;
			psevere *= r;
			pcritical *= r;
		}

		#undef max_prob

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

void region_t::track_stats ()
{
	stats_zone_list.emplace_back();
	stats_zone_t& zone = stats_zone_list.back();

	zone.get_name() = this->get_name();

	for (person_t *p: this->people) {
		zone.add_person(p);
	}
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
	this->id = 0;
	this->state = ST_HEALTHY;
	this->infected_state = ST_NULL;
	this->infection_countdown = 0.0;
	this->infection_cycles = 0.0;
	this->region = NULL;
	this->age = 0;
	this->infected_cycle = -1.0;
	this->n_victims = 0;

	for (int32_t& sid: this->sids)
		sid = -1;

	this->setup_infection_probabilities(cfg->probability_mild, cfg->probability_severe, cfg->probability_critical);
//DMSG(std::endl)
	this->set_death_probability_severe_in_hospital_per_cycle_step( cfg->death_rate_severe_in_hospital_ );
	this->set_death_probability_critical_in_icu_per_cycle_step( cfg->death_rate_critical_in_icu_ );

	this->set_death_probability_severe_outside_hospital_per_cycle_step( cfg->death_rate_severe_outside_hospital_ );
	this->set_death_probability_critical_outside_icu_per_cycle_step( cfg->death_rate_critical_outside_icu_ );

	this->health_unit = nullptr;
	this->neighbor_list = nullptr;
}

person_t::~person_t ()
{
	this->neighbor_list = nullptr;
}

void person_t::add_sid (int32_t sid)
{
	uint32_t i = 0;

	for (auto it=this->sids.begin(); it!=this->sids.end() && *it!=-1; ++it) {
		if (*it == sid) // trying to add twice to the same zone
			return;
		i++;
	}

	C_ASSERT(i < this->sids.size())

	this->sids[i] = sid;
}

void person_t::setup_infection_probabilities (double pmild, double psevere, double pcritical)
{
	C_ASSERT_P((pmild + psevere + pcritical) <= 1.0, "pmild:" << pmild << " psevere:" << psevere << " pcritical:" << pcritical << " sum:" << (pmild+psevere+pcritical))
	C_ASSERT(pmild >= 0.0)
	C_ASSERT(psevere >= 0.0)
	C_ASSERT(pcritical >= 0.0)

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

void person_t::force_infect ()
{
	if (current_cycle == 0.0)
		this->pre_infect(nullptr);
	else
		to_infect_in_cycle.push_back( std::make_pair(nullptr, this) );
}

void person_t::die ()
{
	C_ASSERT(this->state == ST_INFECTED && (this->infected_state == ST_SEVERE || this->infected_state == ST_CRITICAL))

	this->remove_from_infected_list();

	for (auto it=this->sids.begin(); it!=this->sids.end() && *it!=-1; ++it) {
		stats_t& stats = cycle_stats[*it];

		if (this->infected_state == ST_CRITICAL)
			stats.ac_critical_per_age[ get_age_cat(this->age) ]--;

		stats.state[ST_DEAD]++;
	
		stats.ac_state[ST_INFECTED]--;
		stats.ac_state[ST_DEAD]++;
	
		stats.ac_infected_state[ this->infected_state ]--;
	
		stats.r.acc(this->n_victims);
	}

	this->state = ST_DEAD;

	if (this->health_unit != nullptr) {
		this->health_unit->leave(this);
		this->health_unit = nullptr;
	}
//cprintf("cycle %.1f my victims: %u\n", current_cycle, this->n_victims);
}

void person_t::remove_from_infected_list ()
{
	to_recover_die_in_cycle.push_back( this );
}

void person_t::recover ()
{
	C_ASSERT(this->state == ST_INFECTED && (this->infected_state == ST_ASYMPTOMATIC || this->infected_state == ST_MILD || this->infected_state == ST_SEVERE))

	this->remove_from_infected_list();

	for (auto it=this->sids.begin(); it!=this->sids.end() && *it!=-1; ++it) {
		stats_t& stats = cycle_stats[*it];

		stats.state[ST_IMMUNE]++;

		stats.ac_state[ST_INFECTED]--;
		stats.ac_state[ST_IMMUNE]++;
		
		stats.ac_infected_state[ this->infected_state ]--;

		stats.r.acc(this->n_victims);
	}

	this->state = ST_IMMUNE;

	if (this->health_unit != nullptr) {
		this->health_unit->leave(this);
		this->health_unit = nullptr;
	}
//cprintf("cycle %.1f my victims: %u\n", current_cycle, this->n_victims);
}

bool person_t::take_vaccine (double success_rate)
{
	bool r = false;

	switch (this->get_state()) {
		case ST_HEALTHY:
			if (roll_dice(success_rate)) {
				for (auto it=this->sids.begin(); it!=this->sids.end() && *it!=-1; ++it) {
					stats_t& stats = cycle_stats[*it];

					stats.ac_state[ST_HEALTHY]--;
					stats.ac_state[ST_IMMUNE]++;

					stats.state[ST_IMMUNE]++;
				}

				this->state = ST_IMMUNE;
			}
			r = true;
		break;

		case ST_INFECTED:
			switch (this->get_infected_state()) {
				case ST_ASYMPTOMATIC:
					if (roll_dice(success_rate)) {
						rm_infected(this);

						for (auto it=this->sids.begin(); it!=this->sids.end() && *it!=-1; ++it) {
							stats_t& stats = cycle_stats[*it];

							stats.ac_infected_state[ ST_ASYMPTOMATIC ]--;
							stats.ac_state[ST_INFECTED]--;

							stats.ac_state[ST_IMMUNE]++;

							stats.state[ST_IMMUNE]++;
						}

						this->state = ST_IMMUNE;
					}
					r = true;
				break;
			
				case ST_INCUBATION:
					if (roll_dice(success_rate)) {
						rm_infected(this);

						for (auto it=this->sids.begin(); it!=this->sids.end() && *it!=-1; ++it) {
							stats_t& stats = cycle_stats[*it];

							stats.ac_infected_state[ ST_INCUBATION ]--;
							stats.ac_state[ST_INFECTED]--;

							stats.ac_state[ST_IMMUNE]++;

							stats.state[ST_IMMUNE]++;
						}

						this->state = ST_IMMUNE;
					}
					r = true;
				break;

				case ST_PRESYMPTOMATIC:
					if (roll_dice(success_rate)) {
						rm_infected(this);

						for (auto it=this->sids.begin(); it!=this->sids.end() && *it!=-1; ++it) {
							stats_t& stats = cycle_stats[*it];

							stats.ac_infected_state[ ST_PRESYMPTOMATIC ]--;
							stats.ac_state[ST_INFECTED]--;

							stats.ac_state[ST_IMMUNE]++;

							stats.state[ST_IMMUNE]++;
						}

						this->state = ST_IMMUNE;
					}
					r = true;
				break;
			}
		break;

		case ST_IMMUNE:
			r = true;
		break;

		case ST_DEAD:
		break;
	}

	return r;
}

void person_t::try_to_enter_health_unit ()
{
	C_ASSERT(this->health_unit == nullptr)
	C_ASSERT(this->state == ST_INFECTED)
	C_ASSERT(this->infected_state == ST_SEVERE || this->infected_state == ST_CRITICAL)

	this->health_unit = this->region->enter_health_unit(this);
}

void person_t::cycle_infected ()
{
	C_ASSERT(this->state == ST_INFECTED)

	if (this->infected_state != ST_INCUBATION) {
		for (auto it = this->get_neighbor_list()->begin(); *it != nullptr; ++it) {
			if (it.check_probability())
				to_infect_in_cycle.push_back( std::make_pair(this, *it) );
		}
	}

	this->infection_countdown -= cfg->step;

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

		case ST_PRESYMPTOMATIC:
			if (this->infection_countdown <= 0.0) {
				this->infection_countdown = 0.0;
				this->symptoms_arise(false);
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
						for (auto it=this->sids.begin(); it!=this->sids.end() && *it!=-1; ++it) {
							stats_t& stats = cycle_stats[*it];

							stats.ac_infected_state[ST_MILD]--;
							stats.ac_infected_state[ST_SEVERE]++;

							stats.ac_total_infected_state[ST_SEVERE]++;
						}

						this->infected_state = ST_SEVERE;

						this->try_to_enter_health_unit();

						this->severe_start_cycle = current_cycle;

						this->setup_infection_countdown(cfg->cycles_severe_in_hospital->generate());
					break;

					case ST_CRITICAL:
						for (auto it=this->sids.begin(); it!=this->sids.end() && *it!=-1; ++it) {
							stats_t& stats = cycle_stats[*it];

							stats.ac_infected_state[ST_MILD]--;
							stats.ac_infected_state[ST_CRITICAL]++;

							stats.ac_critical_per_age[ get_age_cat(this->age) ]++;

							stats.ac_total_infected_state[ST_CRITICAL]++;
						}

						this->infected_state = ST_CRITICAL;

						this->try_to_enter_health_unit();

						this->critical_start_cycle = current_cycle;

						this->setup_infection_countdown(cfg->cycles_critical_in_icu->generate());
					break;

					default:
						C_ASSERT(0)
				}
			}
		break;

		case ST_SEVERE:
			if (this->health_unit != nullptr && roll_dice(this->death_probability_severe_in_hospital_per_cycle)) {
				global_stats.cycles_severe.acc(current_cycle - this->severe_start_cycle);
				this->die();
			}
			else if (this->health_unit == nullptr && roll_dice(this->death_probability_severe_outside_hospital_per_cycle)) {
				global_stats.cycles_severe.acc(current_cycle - this->severe_start_cycle);
				this->die();
			}
			else if (this->infection_countdown <= 0.0) {
				this->infection_countdown = 0.0;

				global_stats.cycles_severe.acc(current_cycle - this->severe_start_cycle);
				this->recover();
			}
			else if (this->health_unit == nullptr) // yeah, I know I reapeat this condition, but it is easier this way
				this->try_to_enter_health_unit();
		break;

		case ST_CRITICAL:
			if (this->health_unit != nullptr && roll_dice(this->death_probability_critical_in_icu_per_cycle)) {
				global_stats.cycles_critical.acc(current_cycle - this->critical_start_cycle);
				this->die();
			}
			else if (this->health_unit == nullptr && roll_dice(this->death_probability_critical_outside_icu_per_cycle)) {
				global_stats.cycles_critical.acc(current_cycle - this->critical_start_cycle);
				this->die();
			}
			else if (this->infection_countdown <= 0.0) {
				// ok, I got better
				// so instead of critical, I'm now severe

				this->infected_state = ST_SEVERE;

				global_stats.cycles_critical.acc(current_cycle - this->critical_start_cycle);

				this->severe_start_cycle = current_cycle;

				this->setup_infection_countdown(cfg->cycles_severe_in_hospital->generate());

				for (auto it=this->sids.begin(); it!=this->sids.end() && *it!=-1; ++it) {
					stats_t& stats = cycle_stats[*it];

					stats.ac_infected_state[ ST_CRITICAL ]--;
					stats.ac_infected_state[ ST_SEVERE ]++;

					stats.ac_critical_per_age[ get_age_cat(this->age) ]--;
				}
			}
			else if (this->health_unit == nullptr) // yeah, I know I reapeat this condition, but it is easier this way
				this->try_to_enter_health_unit();
		break;

		default:
			C_ASSERT(0)
	}
}

void person_t::pre_infect (person_t *from)
{
	double incub_cycles;

	SANITY_ASSERT(this->state == ST_HEALTHY && this->infected_state == ST_NULL && this->infected_cycle == -1.0)

//printf("tracker add pid %i\n", this->get_id());

	if (likely(from != nullptr)) {
		from->n_victims++;
		incub_cycles = cfg->cycles_incubation->generate();
		global_stats.cycles_between_generations.acc(current_cycle - from->infected_cycle);
	}
	else     // forced infection, reduce the delay of incubation as much as possible
		incub_cycles = cfg->step;

	this->infected_state_vec_pos = pop_infected_n;
	pop_infected[ pop_infected_n++ ] = this;

	this->infected_cycle = current_cycle;
	this->state = ST_INFECTED;
	this->infected_state = ST_INCUBATION;

	for (auto it=this->sids.begin(); it!=this->sids.end() && *it!=-1; ++it) {
		stats_t& stats = cycle_stats[*it];

		stats.state[ST_INFECTED]++;

		stats.infected_state[ ST_INCUBATION ]++;
		stats.ac_infected_state[ ST_INCUBATION ]++;

		stats.ac_state[ST_HEALTHY]--;
		stats.ac_state[ST_INFECTED]++;
	}

	this->setup_infection_countdown( incub_cycles );
}

void person_t::symptoms_arise (bool fast_track)
{
	SANITY_ASSERT(this->state == ST_INFECTED && this->infected_state == ST_PRESYMPTOMATIC)

	C_ASSERT(fast_track == false) // let's disable fast track for now, I think there may be  bug in it

	this->infected_state = this->next_infected_state;
	this->next_infected_state = this->final_infected_state;
	this->setup_infection_countdown(this->final_countdown);

	SANITY_ASSERT(this->infected_state == ST_MILD)

	if (fast_track == false) {
		for (auto it=this->sids.begin(); it!=this->sids.end() && *it!=-1; ++it) {
			stats_t& stats = cycle_stats[*it];

			stats.infected_state[ this->infected_state ]++;

			stats.ac_infected_state[ ST_PRESYMPTOMATIC ]--;
			stats.ac_infected_state[ this->infected_state ]++;

			stats.reported++;
			stats.ac_reported++;
		}
	}
}

void person_t::infect ()
{
	double p;

	SANITY_ASSERT(this->state == ST_INFECTED && this->infected_state == ST_INCUBATION)

	p = generate_random_between_0_and_1();

	if (p <= this->prob_ac_asymptomatic) {
		this->infected_state = ST_ASYMPTOMATIC;
		this->setup_infection_countdown(cfg->cycles_contagious->generate());
	}
	else if (p <= this->prob_ac_mild) {
		this->infected_state = ST_PRESYMPTOMATIC;
		this->next_infected_state = ST_MILD;
		this->final_infected_state = ST_FAKE_IMMUNE;

		double tmp_contagious = cfg->cycles_contagious->generate();
		double tmp_pre_symptomatic = cfg->cycles_pre_symptomatic->generate();

		if (likely(tmp_contagious > tmp_pre_symptomatic))
			this->final_countdown = tmp_contagious - tmp_pre_symptomatic;
		else
			this->final_countdown = 0.0;
		
		this->setup_infection_countdown(tmp_pre_symptomatic);
	}
	else if (p <= this->prob_ac_severe) {
		this->infected_state = ST_PRESYMPTOMATIC;
		this->next_infected_state = ST_MILD;
		this->final_infected_state = ST_SEVERE;
		this->final_countdown = cfg->cycles_before_hospitalization->generate();
		this->setup_infection_countdown(cfg->cycles_pre_symptomatic->generate());
	}
	else {
		this->infected_state = ST_PRESYMPTOMATIC;
		this->next_infected_state = ST_MILD;
		this->final_infected_state = ST_CRITICAL;
		this->final_countdown = cfg->cycles_before_hospitalization->generate();
		this->setup_infection_countdown(cfg->cycles_pre_symptomatic->generate());
	}

	// if there is no delay to have symptons
	if (this->infected_state != ST_ASYMPTOMATIC && this->get_infection_countdown() == 0.0)
		this->symptoms_arise(true);

	for (auto it=this->sids.begin(); it!=this->sids.end() && *it!=-1; ++it) {
		stats_t& stats = cycle_stats[*it];

		stats.infected_state[ this->infected_state ]++;

		stats.ac_infected_state[ ST_INCUBATION ]--;
		stats.ac_infected_state[ this->infected_state ]++;
	}
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

stats_zone_t* create_new_stats_zone ()
{
	stats_zone_list.emplace_back();
	stats_zone_t *zone = &stats_zone_list.back();
	return zone;
}

/****************************************************************/

double get_affective_r0 (std::bitset<NUMBER_OF_FLAGS>& flags)
{
	double r0;

	switch (cfg->network_type) {
		case NETWORK_TYPE_FULLY_CONNECTED:
			r0 = cfg->r0 * cfg->global_r0_factor;
		break;

		case NETWORK_TYPE_NETWORK:
			r0 = network_get_affective_r0(flags);
		break;

		default:
			r0 = 0.0; // avoid warning
			C_ASSERT(0)
	}

	return r0;
}

/****************************************************************/

double get_affective_r0_fast ()
{
	double r0;

	switch (cfg->network_type) {
		case NETWORK_TYPE_FULLY_CONNECTED:
			r0 = cfg->r0 * cfg->global_r0_factor;
		break;

		case NETWORK_TYPE_NETWORK:
			r0 = network_get_affective_r0_fast();
		break;

		default:
			r0 = 0.0; // avoid warning
			C_ASSERT(0)
	}

	return r0;
}

/****************************************************************/

static void simulate ()
{
	uint32_t i;

	current_cycle = 0.0;

	callback_before_cycle(current_cycle);
	callback_after_cycle(current_cycle);

	current_cycle += cfg->step;

	for ( ; current_cycle<cfg->cycles_to_simulate; current_cycle+=cfg->step) {
		cprintf("Cycle %.2f\n", current_cycle);

		prev_cycle_stats_ptr = cycle_stats_ptr;

		all_cycles_stats.emplace_back();
		cycle_stats_ptr = &all_cycles_stats.back();

		i = 0;
		for (stats_zone_t& zone: stats_zone_list) {
			cycle_stats.emplace_back();
			stats_t& stats = cycle_stats.back();
		
			C_ASSERT(stats.cycle == current_cycle)

			cycle_stats[i].copy_ac(&prev_cycle_stats[i]);
			i++;
		}

		callback_before_cycle(current_cycle);

		run_cycle_step();
		
		callback_after_cycle(current_cycle);

		cycle_stats[GLOBAL_STATS].dump();
		cprintf("\n");
	}

	callback_end();
}

/****************************************************************/

static void load_regions ()
{
	uint64_t i, total;

	regions.reserve( cfg->n_regions );

	for (i=0; i<cfg->n_regions; i++)
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

	pop_infected.resize(population.size(), nullptr);

	stats_zone_t& zone = stats_zone_list.front();
	C_ASSERT( zone.get_sid() == GLOBAL_STATS )

	for (person_t *p: population) {
		zone.add_person(p);
	}

	if (cfg->network_type == NETWORK_TYPE_FULLY_CONNECTED) {
		neighbor_list_fully_connected_t *v;
		v = new neighbor_list_fully_connected_t[ population.size() ];

		for (i=0; i<population.size(); i++) {
			v[i].set_person( population[i] );
			population[i]->set_neighbor_list( v+i );
		}
	}
	else if (cfg->network_type == NETWORK_TYPE_NETWORK) {
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

	i = 0;
	for (person_t *p: population)
		p->set_id(i++);

	for (region_t *region: regions)
		region->setup_health_units();

	network_start_population_graph();

	for (region_t *region: regions)
		region->setup_relations();

	setup_inter_region_relations();

	network_after_all_regular_connetions();

	setup_extra_relations();

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

static void load_stats_engine_stage_1 ()
{
	stats_zone_t *zone = create_new_stats_zone();

	zone->get_name() = "global";

	C_ASSERT( zone->get_sid() == GLOBAL_STATS )
}

static void load_stats_engine_stage_2 ()
{
	all_cycles_stats.emplace_back();
	cycle_stats_ptr = &all_cycles_stats.back();

	for (stats_zone_t& zone: stats_zone_list) {
		cycle_stats.emplace_back();
		stats_t& stats = cycle_stats.back();

		stats.zone = &zone;
		stats.ac_state[ST_HEALTHY] = zone.get_population_size();

		C_ASSERT(stats.cycle == 0.0)
	}
}

void panic (const char *str)
{
	tend = std::chrono::steady_clock::now();
	cprintf("%s\n", str);

	std::chrono::duration<double> time_sim = std::chrono::duration_cast<std::chrono::duration<double>>(tend - tbegin);

	cprintf("Execution time %.1fs\n", time_sim.count());
	cprintf("Corona panic\n");

	exit(1);
}

int main (int argc, char **argv)
{
	FILE *fp;
	int64_t i;

	namespace po = boost::program_options;
	po::options_description cmd_line_args("Corona Simulator -- Options");
	po::variables_map vm;

	tbegin = std::chrono::steady_clock::now();

	try {
		cmd_line_args.add_options()
			("help,h", "Help screen")
			("fresults", po::value<std::string>()->default_value(default_results_file), "Results file output")
			("cycles,c", po::value<double>()->default_value(0.0), "Number of cycles to simulate")
			("cfgdump", "Only dump the config");

		setup_cmd_line_args(cmd_line_args);

		po::store(po::parse_command_line(argc, argv, cmd_line_args), vm);
		po::notify(vm);

		if (vm.count("help")) {
			std::cout << cmd_line_args << std::endl;
			return 0;
		}
//		if (vm.count("fresults"))
//			std::cout << "Results file output: " << vm["fresults"].as<std::string>() << std::endl;
	}
	catch (const boost::program_options::error &ex) {
		DMSG(ex.what() << std::endl);
	}

	results_file = vm["fresults"].as<std::string>().c_str();

	CMSG("results_file: " << results_file << std::endl)

	cfg = new cfg_t;

	if (vm["cycles"].as<double>() != 0.0)
		cfg->cycles_to_simulate = vm["cycles"].as<double>();

	C_ASSERT(cfg->cycles_to_simulate > 0.0);

	if (vm.count("cfgdump")) {
		cfg->dump();
		return 0;
	}

	for (i=0; i<AGES_N; i++)
		people_per_age[i] = 0;

	for (i=0; i<AGE_CATS_N; i++)
		people_per_age_cat[i] = 0;

	load_factor_per_relation_group_str();

	generate_entropy();

//benchmark_random_engine();

	load_stats_engine_stage_1();

	load_regions();

	load_stats_engine_stage_2();

	cfg->dump();
//exit(1);
	tbefore_sim = std::chrono::steady_clock::now();

	simulate();

	tend = std::chrono::steady_clock::now();

	i = 0;
	for (stats_zone_t& zone: stats_zone_list) {
		std::string fname(results_file);
		fname += '-';
		fname += scenery_results_fname;
		fname += zone.get_name();
		fname += ".csv";

		fp = fopen(fname.c_str(), "w");
		C_ASSERT(fp != NULL)

		all_cycles_stats.front()[i].dump_csv_header(fp);
		for (auto& vector: all_cycles_stats) {
			stats_t& stats = vector[i];
			stats.dump_csv(fp);
		}

		fclose(fp);
		i++;
	}

	std::chrono::duration<double> time_load = std::chrono::duration_cast<std::chrono::duration<double>>(tbefore_sim - tbegin);
	std::chrono::duration<double> time_sim = std::chrono::duration_cast<std::chrono::duration<double>>(tend - tbefore_sim);

	cprintf("\n");
	global_stats.print();
	cprintf("\n");
	cprintf("load time: %.2fs\n", time_load.count());
	cprintf("simulation time: %.2fs\n", time_sim.count());

	return 0;
}