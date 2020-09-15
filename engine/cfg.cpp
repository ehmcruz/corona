#include <corona.h>

#include <math.h>

cfg_t::cfg_t ()
{
	#define CORONA_CFG(TYPE, PRINT, STAT) this->STAT = 0;
	#define CORONA_CFG_OBJ(TYPE, STAT) this->STAT = nullptr;
	#define CORONA_CFG_VECTOR(TYPE, PRINT, LIST, STAT, N) for (uint32_t i=0; i<N; i++) { this->STAT[i] = 0; }
	#define CORONA_CFG_MATRIX(TYPE, PRINT, PRINTFUNC, STAT, NROWS, NCOLS) for (uint32_t i=0; i<NROWS; i++) { for (uint32_t j=0; j<NCOLS; j++) this->STAT[i][j] = 0; }
	#include <cfg.h>
	#undef CORONA_CFG
	#undef CORONA_CFG_OBJ
	#undef CORONA_CFG_VECTOR
	#undef CORONA_CFG_MATRIX

	this->set_defaults();
	this->scenery_setup();
	this->load_derived();
}

void cfg_t::set_defaults ()
{
	// defaults following COVID known parameters
	
	this->r0 = 3.0;

	this->cycles_contagious = new const_double_dist_t(4.0);
	this->cycles_pre_symptomatic = new const_double_dist_t(1.0);

	this->cycles_to_simulate = 180.0;
	this->cycle_division = 1.0;

	this->cycles_incubation = new gamma_double_dist_t(4.58, 3.24, 1.0, 14.0);

	this->cycles_severe_in_hospital = new const_double_dist_t(8.0);
	this->cycles_critical_in_icu = new const_double_dist_t(8.0);
	this->cycles_before_hospitalization = new const_double_dist_t(5.0);

	this->global_r0_factor = 1.0;
	this->probability_summon_per_cycle_ = 0.05;

	this->death_rate_severe_in_hospital_ = 0.15;
	this->death_rate_critical_in_icu_ = 0.36;
	this->death_rate_severe_outside_hospital_ = 0.90;
	this->death_rate_critical_outside_icu_ = 0.99;

	this->probability_asymptomatic = 0.85;
	this->probability_mild = 0.809 * (1.0 - this->probability_asymptomatic);
	this->probability_critical = 0.044 * (1.0 - this->probability_asymptomatic);

	for (uint32_t i=0; i<NUMBER_OF_RELATIONS; i++) // set defaults
		this->relation_type_weights[i] = 1.0;
	this->relation_type_weights[RELATION_FAMILY] = 3.0;
	this->relation_type_weights[RELATION_BUDDY] = 2.0;
	this->relation_type_weights[RELATION_UNKNOWN] = 1.0;
	
	for (uint32_t r=RELATION_SCHOOL; r<=RELATION_SCHOOL_4; r++)
		this->relation_type_weights[r] = 2.0;
	
	this->relation_type_weights[RELATION_TRAVEL] = 1.0;
	this->relation_type_weights[RELATION_OTHERS] = 1.0;

	for (uint32_t i=0; i<NUMBER_OF_INFECTED_STATES; i++) // set defaults
		this->r0_factor_per_group[i] = 1.0;
	this->r0_factor_per_group[ST_ASYMPTOMATIC] = 1.0;
	this->r0_factor_per_group[ST_PRESYMPTOMATIC] = 1.0;
	this->r0_factor_per_group[ST_MILD] = 1.0;
	this->r0_factor_per_group[ST_SEVERE] = 0.5;
	this->r0_factor_per_group[ST_CRITICAL] = 0.5;

	for (uint32_t r=0; r<NUMBER_OF_RELATIONS; r++) {
		for (uint32_t i=0; i<NUMBER_OF_INFECTED_STATES; i++)
			this->factor_per_relation_group[r][i] = 1.0;
	}

	this->network_type = NETWORK_TYPE_FULLY_CONNECTED;

	this->n_regions = 1;
}

void cfg_t::load_derived ()
{
	this->step = 1.0 / this->cycle_division;

	this->probability_infect_per_cycle_step = this->r0 / (this->cycles_contagious->get_expected() * this->cycle_division);
	this->probability_severe = 1.0 - (this->probability_asymptomatic + this->probability_mild + this->probability_critical);

	this->probability_summon_per_cycle_step = this->probability_summon_per_cycle_ / this->cycle_division;

	this->prob_ac_asymptomatic = this->probability_asymptomatic;
	this->prob_ac_mild = this->prob_ac_asymptomatic + this->probability_mild;
	this->prob_ac_severe = this->prob_ac_mild + this->probability_severe;
	this->prob_ac_critical = this->prob_ac_severe + this->probability_critical;
}

void cfg_t::dump ()
{
	#define CORONA_CFG(TYPE, PRINT, STAT) cprintf(#STAT ": " PRINT "\n", this->STAT);
	#define CORONA_CFG_OBJ(TYPE, STAT) cprintf(#STAT ": "); this->STAT->print_params(stdout); cprintf("\n");
	#define CORONA_CFG_VECTOR(TYPE, PRINT, LIST, STAT, N) for (uint32_t i=0; i<N; i++) { cprintf(#STAT ".%s: " PRINT "\n", LIST##_str(i), this->STAT[i]); }
	#define CORONA_CFG_MATRIX(TYPE, PRINT, PRINTFUNC, STAT, NROWS, NCOLS) for (uint32_t i=0; i<NROWS; i++) { for (uint32_t j=0; j<NCOLS; j++) { cprintf(#STAT ".%s: " PRINT "\n", PRINTFUNC(i, j), this->STAT[i][j]); } }
	#include <cfg.h>
	#undef CORONA_CFG
	#undef CORONA_CFG_OBJ
	#undef CORONA_CFG_VECTOR
	#undef CORONA_CFG_MATRIX
//exit(1);
}