#include <corona.h>
#include <parser.h>

#include <math.h>

#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics.hpp>

using namespace boost::accumulators;

static csv_ages_t *csv;

//static accumulator_set<double, features<tag::mean, tag::median, tag::variance, tag::count, tag::sum> > acc_quarentine;

void cfg_t::scenery_setup ()
{
	csv = new csv_ages_t((char*)"data/distribuicao-etaria-paranavai.csv");
	csv->dump();
}

void region_t::setup_region ()
{
	uint32_t i, n;

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

	this->set_population_number( csv->get_population(this->get_name()) );

	cprintf("%s total de habitantes: " PU64 "\n", this->get_name().c_str(), this->get_npopulation());

	for (i=csv->get_first_age(); i<=csv->get_last_age(); i++) {
		n = csv->get_population_per_age(this->get_name(), i);
		cprintf("%s habitantes idade %i -> %i\n", this->get_name().c_str(), i, n);
		this->add_people(n, i);
	}
	//exit(1);

	this->adjust_population_infection_state_rate_per_age(reported_deaths_per_age);
}

void region_t::callback_before_cycle (uint32_t cycle)
{
	static int32_t has_already_locked = 0, lock_start_cycle;

	if (has_already_locked == 0 && cycle_stats->ac_state[ST_INFECTED] >= 2) {
		has_already_locked = 1;
		cfg.global_r0_factor = 0.35;

		lock_start_cycle = cycle;
	}
	else if (has_already_locked == 1 && cycle >= (lock_start_cycle+30)) {
		cfg.global_r0_factor = 1.0;
	}
}

void region_t::callback_after_cycle (uint32_t cycle)
{

}

void region_t::callback_end ()
{
	
}