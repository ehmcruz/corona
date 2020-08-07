#include <corona.h>

#include <math.h>

cfg_t::cfg_t ()
{
	#define CORONA_CFG(TYPE, PRINT, STAT) this->STAT = 0;
	#define CORONA_CFG_OBJ(TYPE, STAT) this->STAT = nullptr;
	#define CORONA_CFG_VECTOR(TYPE, PRINT, LIST, STAT, N) for (uint32_t i=0; i<N; i++) { this->STAT[i] = 0; }
	#include <cfg.h>
	#undef CORONA_CFG
	#undef CORONA_CFG_OBJ
	#undef CORONA_CFG_VECTOR

	this->set_defaults();
	this->scenery_setup();
	this->load_derived();
}

void cfg_t::set_defaults ()
{
	// defaults following COVID known parameters
	
	this->r0 = 3.0;
	this->cycles_contagious = 4.0;
	this->cycles_pre_symptomatic = 1.0;
	this->cycles_to_simulate = 180;

	this->cycles_incubation = new gamma_double_dist_t(4.58, 3.24, 1.0, 14.0);

	this->cycles_severe_in_hospital = 8.0;
	this->cycles_critical_in_icu = 8.0;
	this->cycles_before_hospitalization = 5.0;
	this->global_r0_factor = 1.0;
	this->probability_summon_per_cycle = 0.05;

	this->death_rate_severe_in_hospital = 0.15;
	this->death_rate_critical_in_hospital = 0.36;
	this->death_rate_severe_outside_hospital = 0.5;
	this->death_rate_critical_outside_hospital = 0.99;

	this->probability_asymptomatic = 0.85;
	this->probability_mild = 0.809 * (1.0 - this->probability_asymptomatic);
	this->probability_critical = 0.044 * (1.0 - this->probability_asymptomatic);

	this->r0_asymptomatic_factor = 1.0;

	this->relation_type_weights[RELATION_FAMILY] = 3;
	this->relation_type_weights[RELATION_BUDDY] = 2;
	this->relation_type_weights[RELATION_UNKNOWN] = 1;
	this->relation_type_weights[RELATION_SCHOOL] = 2;
	this->relation_type_weights[RELATION_TRAVEL] = 1;
	this->relation_type_weights[RELATION_OTHERS] = 1;

	this->network_type = NETWORK_TYPE_FULLY_CONNECTED;

	this->n_regions = 1;
}

/*
probability to die = death_rate_condition_in_hospital

probability to die = 1 - (probability to live_per_cycle)^n

probability to live_per_cycle = (1 - death_rate_condition_in_hospital_per_cycle)

probability to die = 1 - (1 - death_rate_condition_in_hospital_per_cycle)^n
1 - (1 - death_rate_condition_in_hospital_per_cycle)^n = probability to die
- (1 - death_rate_condition_in_hospital_per_cycle)^n = probability to die - 1
(1 - death_rate_condition_in_hospital_per_cycle)^n = 1 - probability to die
1 - death_rate_condition_in_hospital_per_cycle = (1 - probability to die)^(1/n)
- death_rate_condition_in_hospital_per_cycle = (1 - probability to die)^(1/n) - 1
death_rate_condition_in_hospital_per_cycle = 1 - (1 - probability to die)^(1/n)
*/

static inline double calc_death_rate_per_cycle (double death_rate, double n_tries)
{
	return (1.0 - pow(1 - death_rate, 1.0 / n_tries));
}

void cfg_t::load_derived ()
{
	int32_t i;

	for (i=0; i<NUMBER_OF_INFECTED_STATES; i++)
		r0_factor_per_group[i] = 1.0;
	r0_factor_per_group[ST_ASYMPTOMATIC] = this->r0_asymptomatic_factor;

	this->probability_infect_per_cycle = this->r0 / this->cycles_contagious;
	this->probability_severe = 1.0 - (this->probability_asymptomatic + this->probability_mild + this->probability_critical);

	this->prob_ac_asymptomatic = this->probability_asymptomatic;
	this->prob_ac_mild = this->prob_ac_asymptomatic + this->probability_mild;
	this->prob_ac_severe = this->prob_ac_mild + this->probability_severe;
	this->prob_ac_critical = this->prob_ac_severe + this->probability_critical;

	this->death_rate_severe_in_hospital_per_cycle = calc_death_rate_per_cycle(this->death_rate_severe_in_hospital, this->cycles_severe_in_hospital);
	this->death_rate_critical_in_hospital_per_cycle = calc_death_rate_per_cycle(this->death_rate_critical_in_hospital, this->cycles_critical_in_icu);
	this->death_rate_severe_outside_hospital_per_cycle = calc_death_rate_per_cycle(this->death_rate_severe_outside_hospital, this->cycles_severe_in_hospital);
	this->death_rate_critical_outside_hospital_per_cycle = calc_death_rate_per_cycle(this->death_rate_critical_outside_hospital, this->cycles_critical_in_icu);
}

void cfg_t::dump ()
{
	#define CORONA_CFG(TYPE, PRINT, STAT) cprintf(#STAT ": " PRINT "\n", this->STAT);
	#define CORONA_CFG_OBJ(TYPE, STAT) cprintf(#STAT ": "); this->STAT->print_params(stdout); cprintf("\n");
	#define CORONA_CFG_VECTOR(TYPE, PRINT, LIST, STAT, N) for (uint32_t i=0; i<N; i++) { cprintf(#STAT ".%s: " PRINT "\n", LIST##_str(i), this->STAT[i]); }
	#include <cfg.h>
	#undef CORONA_CFG
	#undef CORONA_CFG_OBJ
	#undef CORONA_CFG_VECTOR
//exit(1);
}