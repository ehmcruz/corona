#include <corona.h>
#include <math.h>

#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics.hpp>

using namespace boost::accumulators;

//static accumulator_set<double, features<tag::mean, tag::median, tag::variance, tag::count, tag::sum> > acc_quarentine;

void cfg_t::scenery_setup ()
{
	this->r0 = 3.0;
	this->death_rate = 0.02;
	this->cycles_contagious = 4.0;
	this->population = 100000;
	this->cycles_to_simulate = 360;

	this->cycles_incubation_mean = 4.58;
	this->cycles_incubation_stddev = 3.24;

	this->cycles_severe_in_hospital = 8.0;
	this->cycles_critical_in_icu = 8.0;
	this->cycles_before_hospitalization = 5.0;
	this->global_r0_factor = 1.0;
	this->probability_summon_per_cycle = 0.03;
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
}

void region_t::callback_before_cycle (uint32_t cycle)
{
	static int32_t has_already_locked = 0, lock_start_cycle;

	if (has_already_locked == 0 && cycle_stats->ac_infected_state[ST_CRITICAL] >= 3) {
		has_already_locked = 1;
		cfg.global_r0_factor = 0.35;

		lock_start_cycle = cycle;
	}
	else if (has_already_locked == 1 && cycle_stats->ac_infected_state[ST_CRITICAL] == 0) {
		cfg.global_r0_factor = 1.0;
	}
}

void region_t::callback_after_cycle (uint32_t cycle)
{

}

void region_t::callback_end ()
{
	
}
