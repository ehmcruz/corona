#include <corona.h>
#include <parser.h>

#include <random>
#include <algorithm>

csv_ages_t *csv;

void cfg_t::scenery_setup ()
{
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

	this->family_size_mean = 3.0;
	this->family_size_stddev = 1.0;
	
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
}

void region_t::setup_region ()
{
	uint32_t i, cat, n;

	uint32_t reported_deaths_per_age[AGE_CATS_N] = { // deaths according to the "Boletim da Pandemia"
		2,         // 0-9
		2,         // 10-19
		9,         // 20-29
		40,        // 30-39
		59,        // 40-49
		104,       // 50-59
		198,       // 60-69
		211,       // 70-79
		176,       // 80-89
		48         // 90+
	};


	std::string name;

	name = "Paranavai";

	this->set_name(name);

	this->set_population_number( csv->get_population((char*)"Paranavai") );

	cprintf("pvai total de habitantes: " PU64 "\n", this->get_npopulation());

	for (i=csv->get_first_age(); i<=csv->get_last_age(); i++) {
		n = csv->get_population_per_age((char*)"Paranavai", i);
		cprintf("pvai habitantes idade %i -> %i\n", i, n);
		this->add_people(n, i);
	}
	//exit(1);

	this->adjust_population_infection_state_rate_per_age(reported_deaths_per_age);
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
