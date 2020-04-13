#ifndef __corona_h__
#define __corona_h__

#include <stdio.h>
#include <inttypes.h>

#include <vector>
#include <random>

#define SANITY_CHECK
#define DEBUG

#define C_PRINTF_OUT stdout
#define cprintf(...) fprintf(C_PRINTF_OUT, __VA_ARGS__)

#define C_ASSERT(V) C_ASSERT_PRINTF(V, "bye!\n")

#define C_ASSERT_PRINTF(V, ...) \
	if (unlikely(!(V))) { \
		cprintf("sanity error!\nfile %s at line %u assertion failed!\n%s\n", __FILE__, __LINE__, #V); \
		cprintf(__VA_ARGS__); \
		exit(1); \
	}

#ifdef SANITY_CHECK
	#define SANITY_ASSERT(V) C_ASSERT(V)
#else
	#define SANITY_ASSERT(V)
#endif

#ifdef DEBUG
	#define dprintf(...) cprintf(__VA_ARGS__)
#else
	#define dprintf(...)
#endif

#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)

#define PU64 "%" PRIu64

#define NON_AC_STAT   0
#define AC_STAT       1

#define OO_ENCAPSULATE(TYPE, VAR) \
	private: \
	TYPE VAR; \
	public: \
	inline void set_##VAR (TYPE VAR) { \
		this->VAR = VAR; \
	} \
	inline TYPE get_##VAR () { \
		return this->VAR; \
	}

// read-only
#define OO_ENCAPSULATE_RO(TYPE, VAR) \
	private: \
	TYPE VAR; \
	public: \
	inline TYPE get_##VAR () { \
		return this->VAR; \
	}

#define AGE_CATS_N 10

inline int32_t get_age_cat (int32_t age)
{
	return (age < 90) ? (age / 10) : 9;
}

enum state_t {
	ST_HEALTHY,
	ST_INFECTED,
	ST_IMMUNE,
	ST_DEAD
};

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

	uint32_t cycles_to_simulate;
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

	// derived cfg
	double probability_infect_per_cycle;
	double probability_death_per_cycle;
	double probability_severe;

	double prob_ac_asymptomatic;
	double prob_ac_mild;
	double prob_ac_severe;
	double prob_ac_critical;

	cfg_t();
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

	uint32_t cycle;

	stats_t();
	void reset();
	void dump();
	void copy_ac (stats_t *from);
	void dump_csv_header (FILE *fp);
	void dump_csv (FILE *fp);
};

class region_t;

class person_t
{
	OO_ENCAPSULATE_RO(double, probability_asymptomatic)
	OO_ENCAPSULATE_RO(double, probability_mild)
	OO_ENCAPSULATE_RO(double, probability_severe)
	OO_ENCAPSULATE_RO(double, probability_critical)
	OO_ENCAPSULATE_RO(double, prob_ac_asymptomatic)
	OO_ENCAPSULATE_RO(double, prob_ac_mild)
	OO_ENCAPSULATE_RO(double, prob_ac_severe)
	OO_ENCAPSULATE_RO(double, prob_ac_critical)
	OO_ENCAPSULATE(uint32_t, age)
	OO_ENCAPSULATE(state_t, state)
	OO_ENCAPSULATE(infected_state_t, infected_state)
	OO_ENCAPSULATE(region_t*, region)
private:
	infected_state_t next_infected_state;
	double infection_cycles;
	double infection_countdown;

public:
	person_t();

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
	friend class person_t;

	OO_ENCAPSULATE_RO(uint64_t, npopulation)

private:
	std::vector<person_t*> people;
	uint64_t must_infect_in_cycle;

public:
	region_t();

	void setup_region (); // coded in scenery

	void cycle();

	void add_people (uint64_t n, uint32_t age);
	void set_population_number (uint64_t npopulation);

	inline person_t* get_person (uint64_t i) {
		return this->people[i];
	}

	void summon ();

	void callback_before_cycle (uint32_t cycle); // coded in scenery
	void callback_after_cycle (uint32_t cycle); // coded in scenery
	void callback_end (); // coded in scenery

	void sir_calc ();
	void sir_init ();
	void process_data ();
};

// probability.cpp

void start_dice_engine ();
double generate_random_between_0_and_1 ();
int roll_dice (double probability);
double calculate_infection_probability (person_t *from);
void load_gdistribution_incubation (double mean, double stddev);
double calculate_incubation_cycles ();

extern cfg_t cfg;
extern stats_t *cycle_stats;
extern stats_t *prev_cycle_stats;
extern uint32_t current_cycle;
extern double r0_factor_per_group[NUMBER_OF_INFECTED_STATES];

extern std::mt19937 rgenerator;

#endif