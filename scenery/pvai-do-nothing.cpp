#include <corona.h>
#include <parser.h>

csv_ages_t *csv;

void cfg_t::scenery_setup ()
{
	uint32_t i;

	this->r0 = 3.0;
	this->death_rate = 0.02;
	this->cycles_contagious = 4.0;
	this->cycles_to_simulate = 180;

	this->cycles_incubation_mean = 4.58;
	this->cycles_incubation_stddev = 3.24;

	this->cycles_severe_in_hospital = 8.0;
	this->cycles_critical_in_icu = 8.0;
	this->cycles_before_hospitalization = 5.0;
	this->global_r0_factor = 1.0;
	this->probability_summon_per_cycle = 0.0;
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

	csv = new csv_ages_t((char*)"data/distribuicao-etaria-paranavai.csv");
	csv->dump();

	for (i=csv->get_first_age(); i<=csv->get_last_age(); i++) {
		cprintf("pvai habitantes idade %i -> %i\n", i, csv->get_population_per_age((char*)"Paranavai", i));
	}

	exit(0);
}

void region_t::setup_region ()
{
	this->set_population_number(100000);

	this->add_people(100000, 20);
}

void region_t::callback_before_cycle (uint32_t cycle)
{

}

void region_t::callback_after_cycle (uint32_t cycle)
{

}

void region_t::callback_end ()
{

}
