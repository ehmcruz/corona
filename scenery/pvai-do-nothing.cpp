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
	uint32_t people_per_age[AGE_CATS_N];

	uint32_t deaths_per_age[AGE_CATS_N] = { // deaths according to the "Boletim da Pandemia"
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

	uint32_t deaths_per_age_sum;

	double deaths_weight_per_age[AGE_CATS_N];
	double predicted_critical_per_age[AGE_CATS_N];
	double k, sum;
	double pmild, psevere, pcritical;

	for (i=0; i<AGE_CATS_N; i++)
		people_per_age[i] = 0;

	this->set_population_number( csv->get_population((char*)"Paranavai") );

	cprintf("pvai total de habitantes: " PU64 "\n", this->get_npopulation());

	for (i=csv->get_first_age(); i<=csv->get_last_age(); i++) {
		n = csv->get_population_per_age((char*)"Paranavai", i);
		cprintf("pvai habitantes idade %i -> %i\n", i, n);
		this->add_people(n, i);

		cat = get_age_cat(i);
		people_per_age[cat] += n;
	}

	for (i=0; i<AGE_CATS_N; i++) {
		if (people_per_age[i] == 0)
			deaths_per_age[i] = 0;
	}

	sum = 0.0;
	deaths_per_age_sum = 0;
	for (i=0; i<AGE_CATS_N; i++) {
		if (people_per_age[i])
			deaths_weight_per_age[i] = (double)deaths_per_age[i] / (double)people_per_age[i];
		else
			deaths_weight_per_age[i] = 0.0;

		deaths_per_age_sum += deaths_per_age[i];
		
		//sum += (double)deaths_weight_per_age[i] * (double)people_per_age[i];
		sum += (double)deaths_per_age[i];
		
		cprintf("pvai habitantes por idade (%i ate %i): %u death weight:%.4f\n", (i*10), (i*10 + 9), people_per_age[i], deaths_weight_per_age[i]);
	}

	/*
		(k * (sum weight*people_per_age) / total_people) = critical_rate
		k * (sum weight*people_per_age) = critical_rate * total_people
		k = (critical_rate * total_people) / (sum weight*people_per_age)
	*/

	k = (cfg.probability_critical * (double)this->get_npopulation()) / sum;

	cprintf("sum:%.4f k:%.4f deaths_per_age_sum:%u\n", sum, k, deaths_per_age_sum);

	sum = 0.0;
	for (i=0; i<AGE_CATS_N; i++) {
		deaths_weight_per_age[i] *= k;
		predicted_critical_per_age[i] = (double)people_per_age[i] * deaths_weight_per_age[i];
		sum += predicted_critical_per_age[i];

		cprintf("pvai habitantes por idade (%i ate %i): %u death weight:%.4f critical:%2.f\n", (i*10), (i*10 + 9), people_per_age[i], deaths_weight_per_age[i], predicted_critical_per_age[i]);
	}

	cprintf("total critical predicted:%.4f sum:%.2f critical_rate:%.4f critical_rate_test:%.4f\n",
	       (cfg.probability_critical * (double)this->get_npopulation()),
	       sum,
	       cfg.probability_critical,
	       sum / (double)this->get_npopulation()
	       );

	std::shuffle(this->people.begin(), this->people.end(), rgenerator);

	for (i=0; i<this->get_npopulation(); i++) {
		person_t *p;

		p = this->get_person(i);

		cat = get_age_cat( p->get_age() );

		pcritical = deaths_weight_per_age[cat];
		//pcritical = cfg.probability_critical;
		psevere = pcritical * (cfg.probability_severe / cfg.probability_critical);
		pmild = pcritical * (cfg.probability_mild / cfg.probability_critical);

		p->setup_infection_probabilities(pmild, psevere, pcritical);
	}

	//exit(1);
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
