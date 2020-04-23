#include <corona.h>

cfg_t::cfg_t ()
{
	this->scenery_setup();
	this->load_derived();
}

void cfg_t::load_derived ()
{
	int32_t i;

	for (i=0; i<NUMBER_OF_INFECTED_STATES; i++)
		r0_factor_per_group[i] = 1.0;
	r0_factor_per_group[ST_ASYMPTOMATIC] = this->r0_asymptomatic_factor;

	this->probability_infect_per_cycle = this->r0 / this->cycles_contagious;
	this->probability_death_per_cycle = this->death_rate / this->cycles_contagious;
	this->probability_severe = 1.0 - (this->probability_asymptomatic + this->probability_mild + this->probability_critical);

	this->prob_ac_asymptomatic = this->probability_asymptomatic;
	this->prob_ac_mild = this->prob_ac_asymptomatic + this->probability_mild;
	this->prob_ac_severe = this->prob_ac_mild + this->probability_severe;
	this->prob_ac_critical = this->prob_ac_severe + this->probability_critical;
}

void cfg_t::dump ()
{
	dprintf("# ro = %0.4f\n", this->r0);
	dprintf("# death_rate = %0.4f\n", this->death_rate);
	dprintf("# cycles_contagious = %0.4f\n", this->cycles_contagious);
	
	dprintf("# cycles_incubation_mean = %0.4f\n", this->cycles_incubation_mean);
	dprintf("# cycles_incubation_stddev = %0.4f\n", this->cycles_incubation_stddev);
	
	dprintf("# cycles_to_simulate = %.2f\n", this->cycles_to_simulate);
	dprintf("# probability_asymptomatic = %0.4f\n", this->probability_asymptomatic);
	dprintf("# probability_mild = %0.4f\n", this->probability_mild);
	dprintf("# probability_severe = %0.4f\n", this->probability_severe);
	dprintf("# probability_critical = %0.4f\n", this->probability_critical);

	dprintf("# probability_infect_per_cycle = %0.4f\n", this->probability_infect_per_cycle);
	dprintf("# probability_death_per_cycle = %0.4f\n", this->probability_death_per_cycle);

	dprintf("# prob_ac_asymptomatic = %0.4f\n", this->prob_ac_asymptomatic);
	dprintf("# prob_ac_mild = %0.4f\n", this->prob_ac_mild);
	dprintf("# prob_ac_severe = %0.4f\n", this->prob_ac_severe);
	dprintf("# prob_ac_critical = %0.4f\n", this->prob_ac_critical);
}