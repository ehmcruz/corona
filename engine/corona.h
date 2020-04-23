#ifndef __corona_h__
#define __corona_h__

#include <stdio.h>
#include <inttypes.h>

#include <vector>
#include <random>
#include <string>

#include <boost/graph/graph_traits.hpp>
#include <boost/graph/undirected_graph.hpp>

#include <lib.h>

#define AGE_CATS_N 10

inline int32_t get_age_cat (int32_t age)
{
	return (age < 90) ? (age / 10) : 9;
}

enum state_t {
	// Important !!!
	// whenever this list is modified, modify also state_str()

	ST_HEALTHY,
	ST_INFECTED,
	ST_IMMUNE,
	ST_DEAD,

	NUMBER_OF_STATES
};

char* state_str (int32_t i);

enum infected_state_t {
	// Important !!!
	// whenever this list is modified, modify also infected_state_str()

	ST_INCUBATION,
	ST_ASYMPTOMATIC,
	ST_MILD,
	ST_SEVERE,
	ST_CRITICAL,
	NUMBER_OF_INFECTED_STATES,

	// fake states
	ST_FAKE_IMMUNE,
	ST_NULL
};

char* infected_state_str (int32_t i);

char* critical_per_age_str (int32_t age_group);

class cfg_t {
public:
	// primary cfg
	double r0;
	double death_rate;
	double cycles_contagious;

	double cycles_incubation_mean;
	double cycles_incubation_stddev;

	double cycles_to_simulate;
	double probability_asymptomatic;
	double probability_mild;
	double probability_critical;
	double r0_asymptomatic_factor;
	double cycles_severe_in_hospital;
	double cycles_critical_in_icu;
	double cycles_before_hospitalization;
	double global_r0_factor;
	double probability_summon_per_cycle;
	uint32_t hospital_beds;
	uint32_t icu_beds;

	double family_size_mean;
	double family_size_stddev;

	// derived cfg
	double probability_infect_per_cycle;
	double probability_death_per_cycle;
	double probability_severe;

	double prob_ac_asymptomatic;
	double prob_ac_mild;
	double prob_ac_severe;
	double prob_ac_critical;

	cfg_t();
	void set_defaults ();
	void scenery_setup (); // coded in scenery
	void load_derived();
	void dump ();
};

class stats_t {
public:
	#define CORONA_STAT(TYPE, PRINT, STAT, AC) TYPE STAT;
	#define CORONA_STAT_VECTOR(TYPE, PRINT, LIST, STAT, N, AC) TYPE STAT[N];
	#include "stats.h"
	#undef CORONA_STAT
	#undef CORONA_STAT_VECTOR

	double cycle;

	stats_t (double cycle);
	void reset();
	void dump();
	void copy_ac (stats_t *from);
	static void dump_csv_header (FILE *fp);
	void dump_csv (FILE *fp);
};

class region_t;
class person_t;

enum relation_type_t {
	RELATION_FAMILY,
	RELATION_FRIENDS,
	RELATION_OTHERS
};

struct pop_vertex_data_t {
	person_t *p;
};

struct pop_edge_data_t {
	double probability;
	relation_type_t relation;
};

typedef boost::undirected_graph<pop_vertex_data_t, pop_edge_data_t> pop_graph_t;
typedef boost::graph_traits<pop_graph_t>::vertex_descriptor pop_vertex_t;

class person_t
{
	OO_ENCAPSULATE_RO(double, probability_asymptomatic)
	OO_ENCAPSULATE_RO(uint32_t, id)
	OO_ENCAPSULATE_RO(double, probability_mild)
	OO_ENCAPSULATE_RO(double, probability_severe)
	OO_ENCAPSULATE_RO(double, probability_critical)
	OO_ENCAPSULATE_RO(double, prob_ac_asymptomatic)
	OO_ENCAPSULATE_RO(double, prob_ac_mild)
	OO_ENCAPSULATE_RO(double, prob_ac_severe)
	OO_ENCAPSULATE_RO(double, prob_ac_critical)
	OO_ENCAPSULATE_RO(double, infected_cycle);
	OO_ENCAPSULATE_RO(neighbor_list_t*, neighbor_list)
	OO_ENCAPSULATE(uint32_t, age)
	OO_ENCAPSULATE(state_t, state)
	OO_ENCAPSULATE(infected_state_t, infected_state)
	OO_ENCAPSULATE(region_t*, region)

private:
	infected_state_t next_infected_state;
	double infection_cycles;
	double infection_countdown;

public:
	pop_vertex_t vertex;

public:
	person_t ();
	~person_t ();

	void cycle ();
	void cycle_infected ();
	void die ();
	void recover ();
	void infect ();
	void pre_infect ();

	void setup_infection_probabilities (double pmild, double psevere, double pcritical);

	inline void setup_infection_countdown (double cycles) {
		this->infection_cycles = cycles;
		this->infection_countdown = cycles;
	}
};

class region_t
{
	OO_ENCAPSULATE_RO(uint64_t, npopulation)
	OO_ENCAPSULATE_REFERENCE(std::string, name)

private:
	std::vector<person_t*> people;

public:
	region_t();

	void setup_region (); // coded in scenery
	
	void add_to_population_graph ();
	void create_families ();

	void cycle();

	void add_people (uint64_t n, uint32_t age);
	void set_population_number (uint64_t npopulation);

	// number of elements of reported_deaths_per_age_ must be AGE_CATS_N
	void adjust_population_infection_state_rate_per_age (uint32_t *reported_deaths_per_age_);

	inline person_t* get_person (uint64_t i) {
		return this->people[i];
	}

	void summon ();

	void callback_before_cycle (uint32_t cycle); // coded in scenery
	void callback_after_cycle (uint32_t cycle); // coded in scenery
	void callback_end (); // coded in scenery
};

// network.cpp

void start_population_graph ();

#include <probability.h>

extern cfg_t cfg;
extern stats_t *cycle_stats;
extern double current_cycle;
extern double r0_factor_per_group[NUMBER_OF_INFECTED_STATES];
extern region_t *region;
extern std::vector<person_t*> population;

#endif