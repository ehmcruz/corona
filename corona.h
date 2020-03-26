#ifndef __corona_h__
#define __corona_h__

#include <stdio.h>
#include <inttypes.h>

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

#ifdef DEBUG
	#define dprintf(...) cprintf(__VA_ARGS__)
#else
	#define dprintf(...)
#endif

#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)

#define PU64 "%" PRIu64

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
	void dump ();
};

class stats_t {
public:
	uint64_t deaths;
	uint64_t infected;
	uint64_t new_infected;
	FILE *fp;

	stats_t();
	void reset();
	void dump();
	void global_dump ();
};

class region_t;

class person_t
{
private:
	state_t state;
	double infection_countdown;
	region_t *region;

public:
	person_t();

	void cycle ();
	void cycle_infected ();
	void die();
	void infect();

	inline state_t get_state () {
		return this->state;
	}

	inline void set_region (region_t *region) {
		this->region = region;
	}

	inline region_t* get_region () {
		return this->region;
	}
};

class region_t
{
	friend class person_t;

private:
	person_t *people;
	uint64_t must_infect_in_cycle;

public:
	region_t();

	void cycle();

	inline person_t* get_person (uint64_t i) {
		return (this->people + i);
	}
};

#endif