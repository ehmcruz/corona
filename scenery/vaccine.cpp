#include <corona.h>
#include <parser.h>

#include <random>
#include <algorithm>
#include <numeric>
#include <fstream>
#include <iomanip>

#define X_VACCINE_STRATEGY   \
	X_VACCINE_STRATEGY_(none) \
	X_VACCINE_STRATEGY_(random) \
	X_VACCINE_STRATEGY_(priorities)

enum class vaccine_plan_t {
	#define X_VACCINE_STRATEGY_(S) S,
	X_VACCINE_STRATEGY
	#undef X_VACCINE_STRATEGY_
};

static const char* vaccine_plan_str (vaccine_plan_t p)
{
	static const char *list[] = {
		#define X_VACCINE_STRATEGY_(S) #S,
		X_VACCINE_STRATEGY
		#undef X_VACCINE_STRATEGY_
	};

	return list[ static_cast<uint32_t>(p) ];
}

static vaccine_plan_t vaccine_plan = vaccine_plan_t::none;

static double vaccine_immunity_rate = 1.0;

static double vaccine_pop_ratio = 0.0;

static uint32_t vaccine_pop = 0;

static double target_r = 3.0;

static csv_ages_t *csv;

static uint32_t sp_icu = 100000000;

static health_unit_t *uti;
static health_unit_t *enfermaria;

static int32_t stages_green = 0;

void sp_setup_infection_state_rate ();

void setup_cmd_line_args (boost::program_options::options_description& cmd_line_args)
{
	cmd_line_args.add_options()
		("vaccine_pop_ratio,v", boost::program_options::value<double>()->notifier( [] (double v) {
				vaccine_pop_ratio = v;
			} ), "ratio of people that will take the vaccine")
		("rep,r", boost::program_options::value<double>()->notifier( [] (double v) {
				target_r = v;
			} ), "target R0")
		("vaccinestrat,z", boost::program_options::value<std::string>()->notifier( [] (std::string v) {
				if (v == "none")
					vaccine_plan = vaccine_plan_t::none;
				else if (v == "random")
					vaccine_plan = vaccine_plan_t::random;
				else if (v == "priorities")
					vaccine_plan = vaccine_plan_t::priorities;
				else {
					CMSG("vaccinestrat should be none, random or priorities" << std::endl)
					exit(1);
				}
			} ), "Vaccine strategy, should be none, random or priorities")
		("spicu", boost::program_options::value<double>()->notifier( [] (uint32_t v) {
				sp_icu = v;
			} ), "Amount of ICUs (default unlimited)")
		;
}

void cfg_t::scenery_setup ()
{
	this->network_type = NETWORK_TYPE_NETWORK_SIMPLE;
	//this->network_type = NETWORK_TYPE_NETWORK;
	this->cycles_to_simulate = 360.0;

	for (uint32_t r=RELATION_SCHOOL; r<=RELATION_SCHOOL_4; r++) {
		this->relation_type_weights[r] = 3.0;

		this->factor_per_relation_group[r][ST_MILD] = 0.0;
		this->factor_per_relation_group[r][ST_SEVERE] = 0.0;
		this->factor_per_relation_group[r][ST_CRITICAL] = 0.0;
	}

	for (uint32_t r=RELATION_WORK; r<=RELATION_WORK_1; r++) {
		this->relation_type_weights[r] = 1.5;

		this->factor_per_relation_group[r][ST_MILD] = 0.0;
		this->factor_per_relation_group[r][ST_SEVERE] = 0.0;
		this->factor_per_relation_group[r][ST_CRITICAL] = 0.0;
	}

	for (uint32_t r: {RELATION_FAMILY}) {
		this->factor_per_relation_group[r][ST_SEVERE] = 0.0;
		this->factor_per_relation_group[r][ST_CRITICAL] = 0.0;
	}

	this->r0 = target_r;

	// these will be by-passed
	this->probability_asymptomatic = 0.85;
	this->probability_mild = 0.809 * (1.0 - this->probability_asymptomatic);
	this->probability_critical = 0.044 * (1.0 - this->probability_asymptomatic);

	delete this->cycles_critical_in_icu;
	delete this->cycles_severe_in_hospital;

	/*
		Although the means are 15 and 6.5, respectively,
		we need to apply a correction to consider the deaths,
		otherwise the actual ICU and HOSPITAL length of stays would be much lower

		The expected number of days is given by:

		expected = distribution_mean * (1 - mean_death_rate)

		distribution_mean = expected / (1 - mean_death_rate)
	*/

	this->cycles_critical_in_icu = new gamma_double_dist_t(23.9, 15, 1.0, 60.0);
	
	this->cycles_severe_in_hospital = new gamma_double_dist_t(7.7, 5.5, 1.0, 60.0);

	csv = new csv_ages_t((char*)"data/distribuicao-etaria-paranavai.csv");
	csv->dump();

//	this->n_regions = csv->get_ncities();
	this->n_regions = 1;

	DMSG("number of cities " << this->n_regions << std::endl);

/*	cprintf("RELATION_SCHOOL = %u\n", RELATION_SCHOOL);
	cprintf("RELATION_SCHOOL_0 = %u\n", RELATION_SCHOOL_0);
	cprintf("RELATION_SCHOOL_1 = %u\n", RELATION_SCHOOL_1);
	cprintf("RELATION_SCHOOL_2 = %u\n", RELATION_SCHOOL_2);
	cprintf("RELATION_SCHOOL_3 = %u\n", RELATION_SCHOOL_3);
	cprintf("RELATION_SCHOOL_4 = %u\n", RELATION_SCHOOL_4);

	uint32_t i = 0;
	for (uint32_t r=RELATION_SCHOOL_0; r<=RELATION_SCHOOL_4; r++)
		cprintf("RELATION_SCHOOL_%i = %u    %u\n", i++, r, r - RELATION_SCHOOL_0);
exit(1);*/

	uti = new health_unit_t(sp_icu, ST_CRITICAL);
	enfermaria = new health_unit_t(100000000, ST_SEVERE);

	DMSG("R0: " << this->r0 << std::endl)
	DMSG("vaccine_plan: " << vaccine_plan_str(vaccine_plan) << std::endl)
	DMSG("vaccine_pop_ratio: " << vaccine_pop_ratio << std::endl)
	DMSG("sp_icu: " << sp_icu << std::endl)
}

void region_t::setup_population ()
{
	uint32_t i, n;
	uint32_t extrapolate = 1000000, total;

	std::string name;

	//name = csv->get_city_name( this->get_id() );
	name = "Paranavai";

	this->set_name(name);

	std::vector<uint32_t> people_age;
	double ratio = static_cast<double>(extrapolate) / static_cast<double>( csv->get_population(this->get_name()) );

	people_age.resize(csv->get_last_age()+1, 0);

	total = 0;
	for (i=csv->get_first_age(); i<=csv->get_last_age(); i++) {
		n = csv->get_population_per_age(this->get_name(), i);
		people_age[i] = static_cast<uint32_t>( static_cast<double>(n) * ratio );
		total += people_age[i];
	}

	this->set_population_number( total );

	cprintf("%s total de habitantes: " PU64 "\n", this->get_name().c_str(), this->get_npopulation());

	for (i=0; i<people_age.size(); i++) {
		n = people_age[i];
		//cprintf("%s habitantes idade %i -> %i\n", this->get_name().c_str(), i, n);
		if (n > 0)
			this->add_people(n, i);
	}
	//exit(1);

//	if (this->get_name() == "Paranavai")
//		this->track_stats();
}

void region_t::setup_health_units ()
{
//	sp_setup_infection_state_rate();

	this->add_health_unit( uti );
	this->add_health_unit( enfermaria );
}

void region_t::setup_relations ()
{
	if (check_if_network()) {
		gamma_double_dist_t dist_family_size(3.3, 1.7, 1.0, 10.0);
		gamma_double_dist_t dist_number_random_connections(15.0, 10.0, 5.0, 50.0);

		dprintf("creating families for city %s...\n", this->get_name().c_str());
		this->create_families(dist_family_size);

		dprintf("creating random connections for city %s...\n", this->get_name().c_str());

		std::string rname(this->get_name());
		rname += " random loading...";
		report_progress_t progress_random(rname.c_str(), this->get_npopulation(), 100000);

		this->create_random_connections(dist_number_random_connections, RELATION_UNKNOWN, &progress_random);

		// ----------------- work relations

		/*
			According to IBGE
			source: https://agenciadenoticias.ibge.gov.br/agencia-sala-de-imprensa/2013-agencia-de-noticias/releases/26740-pnad-continua-taxa-de-desocupacao-e-de-11-0-e-taxa-de-subutilizacao-e-de-23-0-no-trimestre-encerrado-em-dezembro
			pnad continua dez/2019

			the only services affected by the quarentine was the commerce
			industry, agriculture, etc did not "lockdown"

			- the total amount of formal workers was 33.7 million
			- the total amount of informal workers was 11.9 million
			- people that work for themselves was 24.6 million
			- employers: 4.4 million
			- domestic workers: 6.4 million
			- public employees: 11.6 million
			- TOTAL: 92.6 million

			people that work in places modeled as workplace relations
			- approximately 18 million people in Brazil work in the commerce
			- public employees: 11.6 million
			- industry: 12 million people
			- TOTAL: 41.6 million, which corresponds to 45% of total workers
			- the other workers would be considered in the random relations, since they mostly work in open environments or with less people in the same room

			from that amount, people that were affected by the quarentine
			- approximately 18 million people in Brazil work in the commerce
			- public employees: 11.6 million
			- TOTAL: 29.6 million, which corresponds to 71% of workplace relations

			Therefore, we can estimate that fof every 3 workplace relations, 2 were affcted by quarentine.
			However, we need to remember that several commerce and public employees continnued working normally.
			This is basically impossible to know.
			Hence, let's say that 50% of workplace relations were affected by the quarentine.

			Summarizing, 45% of total workers we model as workplace relations.
			The other workers would be considered in the random relations, since they mostly work in open environments or with less people in the same room.
			Inside this 45%, only 50% of the relations were affected by quarentine.

			According to IBGE data, 64% of people of age >= 18 work
		*/

		CMSG("creating workplace relations for city " << this->get_name() << std::endl);

		std::vector<person_t*> people_ = this->people;

		std::shuffle(people_.begin(), people_.end(), rgenerator);

	{
		gamma_double_dist_t dist_workplace_size(8.0, 7.0, 2.0, 50.0);
		const double employment_rate = 0.64;
		const double work_that_is_modeled_as_work_relation = 0.45;
		const uint32_t age_ini = 18;
		const uint32_t age_end = 65;
		const double relation_ratio = 0.5;

		uint32_t n = 0, i;

		for (person_t *p: people_) {
			if (p->get_age() >= age_ini && p->get_age() <= age_end)
				n++;
		}

		n = static_cast<uint32_t>(static_cast<double>(n) * employment_rate * work_that_is_modeled_as_work_relation);

		std::vector<person_t*> tmp_workers;

		tmp_workers.reserve(n);

		i = 0;
		for (person_t *p: people_) {
			C_ASSERT(i <= n)

			if (bunlikely(i == n))
				break;

			if (p->get_age() >= age_ini && p->get_age() <= age_end) {
				tmp_workers.push_back(p);
				i++;
			}
		}

		network_create_clusters(tmp_workers, dist_workplace_size, { RELATION_WORK_0, RELATION_WORK_1 }, relation_ratio, nullptr);
	}

		// ----------------- school relations

//return;
		dprintf("creating schools for city %s...\n", this->get_name().c_str());

		std::vector<person_t*> students;
		uint32_t age_ini = 1;
		uint32_t age_end = 18;
		double school_ratio = 0.778;

		//gamma_double_dist_t dist_school_size(438.9, 449.3, 200.0, 4500.0);
		//gamma_double_dist_t dist_school_class_size(22.1, 10.5, 10.0, 50.0);

		gamma_double_dist_t dist_school_size(500.0, 500.0, 300.0, 4000.0);
		gamma_double_dist_t dist_school_class_size(30.0, 10.0, 15.0, 50.0);

		struct school_per_age_t {
			uint32_t age;
			uint32_t n, i;
		};

		std::vector<school_per_age_t> v;
		v.resize(age_end+1);

		std::shuffle(people_.begin(), people_.end(), rgenerator);

		for (uint32_t age=age_ini; age<=age_end; age++) {
			v[age].n = (uint32_t)((double)this->get_region_people_per_age(age) * school_ratio);
			v[age].age = age;
			v[age].i = 0;
		}

		students.reserve(this->people.size());

		for (person_t *p: people_) {
			uint32_t age = p->get_age();

			if (age >= age_ini && age <= age_end && v[age].i < v[age].n) {
				students.push_back(p);
				v[age].i++;
			}
		}

		gamma_double_dist_t dist_school_prof_age(41.2, 9.9, 20.0, 70.0);

		rname = this->get_name();
		rname += " school loading...";
		report_progress_t progress_school(rname.c_str(), students.size(), 100000);

		network_create_school_relation(students,
		                                  age_ini,
		                                  age_end,
		                                  dist_school_class_size,
                                          dist_school_size,
                                          this,
                                          dist_school_prof_age,
                                          0.5,
                                          0.005,
                                          RELATION_SCHOOL,
                                          RELATION_SCHOOL,
                                          RELATION_SCHOOL_4,
                                          &progress_school);
	}
}

void setup_inter_region_relations ()
{
	vaccine_pop = static_cast<uint32_t>(vaccine_pop_ratio * static_cast<double>(population.size()));

	DMSG("Amount of people to be vaccinated: " << vaccine_pop << std::endl)

	if (check_if_network()) {
	}
//exit(1);
}

void setup_extra_relations ()
{
	if (check_if_network()) {
	}

	sp_setup_infection_state_rate();
//exit(1);
}

static void apply_vaccine (double cycle)
{
	if (vaccine_plan != vaccine_plan_t::none) {
		static bool first = false;
		static std::vector<person_t*>::iterator it;
		static std::vector<person_t*> pop;

		uint32_t i;

		if (!first) {
			first = true;

			switch (vaccine_plan) {
				case vaccine_plan_t::random:
					pop = population;
					std::shuffle(pop.begin(), pop.end(), rgenerator);
				break;

				case vaccine_plan_t::priorities: {
					static std::vector<person_t*> tmp = population;

					std::shuffle(tmp.begin(), tmp.end(), rgenerator);

					pop.reserve( population.size() );

					std::bitset<NUMBER_OF_FLAGS> mask_school;
					for (uint32_t r=RELATION_SCHOOL; r<=RELATION_SCHOOL_4; r++)
						mask_school.set(r);

					// set that no one took the vaccine

					for (person_t *p: tmp)
						p->set_foo(0);

					// first to get the vaccine are the old

					for (person_t *p: tmp) {
						if (p->get_age() >= 50) {
							pop.push_back(p);
							p->set_foo(1);
						}
					}

					// now the professors

					for (person_t *p: tmp) {
						if (p->get_foo() == 0 && (network_vertex_data(p).flags.test(VFLAG_PROFESSOR))) {
							pop.push_back(p);
							p->set_foo(1);
						}
					}

					// now the students

/*					for (person_t *p: tmp) {
						if (p->get_foo() == 0 && (network_vertex_data(p).flags & mask_school).any()) {
							pop.push_back(p);
							p->set_foo(1);
						}
					}*/

					// now the rest

					for (person_t *p: tmp) {
						if (p->get_foo() == 0) {
							pop.push_back(p);
							//p->set_foo(1);
						}
					}
				}
				break;

				default:
					C_ASSERT(0)
			}

			C_ASSERT(pop.size() == population.size())

			it = pop.begin();
		}

		i = 0;
		while (it != pop.end()) {
			C_ASSERT(i <= vaccine_pop)

			if (bunlikely(i == vaccine_pop))
				break;

			person_t *p = *it;

			i += p->take_vaccine(vaccine_immunity_rate);

			++it;
		}
	}
}

void callback_before_cycle (double cycle)
{
	const uint32_t people_warmup = 50;

	if (cycle == 1.0) {
//		apply_vaccine(cycle);
	}
	else if (cycle == 2.0) {
		std::vector<person_t*> tmp = population;
		uint32_t i = 0;

		std::shuffle(tmp.begin(), tmp.end(), rgenerator);

		dprintf("cycle %.2f summon %u\n", cycle, people_warmup);

		for (person_t *p: tmp) {
			if (i >= people_warmup)
				break;
			if (p->get_state() == ST_HEALTHY) {
				p->force_infect();
				i++;
			}
		}
	}
}

void callback_after_cycle (double cycle)
{

}

void callback_end ()
{
	uint32_t i;
	uint64_t n_students, n_profs;

//	C_ASSERT(stages_green == 4)

	n_profs = get_n_population_per_relation_flag( {VFLAG_PROFESSOR} );

	std::bitset<NUMBER_OF_FLAGS> mask;
	for (uint32_t r=RELATION_SCHOOL; r<=RELATION_SCHOOL_4; r++)
		mask.set(r);

	n_students = get_n_population_per_relation_flag(mask);

	cprintf("amount of professors: " PU64 "\n", n_profs);
	cprintf("amount of students: " PU64 "\n", n_students);

	for (i=0; i<NUMBER_OF_RELATIONS; i++) {
		cprintf("relation-%s: " PU64 "\n", relation_type_str(i), cfg->relation_type_number[i]);
	}

	uint64_t n_relations;

	n_relations = 0;
	for (uint32_t r=RELATION_SCHOOL; r<=RELATION_SCHOOL_4; r++)
		n_relations += cfg->relation_type_number[r];

	cprintf("amount of school relations per student: %.2f  (students " PU64 "  rel " PU64 ")\n", (double)n_relations / (double)n_students, n_students, n_relations);

	n_relations = 0;
	for (uint32_t r=RELATION_SCHOOL; r<=RELATION_SCHOOL_3; r++)
		n_relations += cfg->relation_type_number[r];

	cprintf("amount of intra-class school relations per student: %.2f  (students " PU64 "  rel " PU64 ")\n", (double)n_relations / (double)n_students, n_students, n_relations);

	n_relations = cfg->relation_type_number[RELATION_SCHOOL_4];

	cprintf("amount of inter-class school relations per student: %.2f  (students " PU64 "  rel " PU64 ")\n", (double)n_relations / (double)n_students, n_students, n_relations);
}

/**************************************************************/

void sp_setup_infection_state_rate ()
{
	// date: 2020-09-07
	// +- 6 months after pandemic start

	uint32_t reported_critical_per_age[AGE_CATS_N] = {
		239,	//0-9
		169,	//10-19
		695,	//20-29
		2044,	//30-39
		3105,	//40-49
		4237,	//50-59
		4910,	//60-69
		4322,	//70-79
		2494,	//80-89
		592	//90+
	};

	// Total reported critical: 22807

	uint32_t reported_severe_per_age[AGE_CATS_N] = {
		584,	//0-9
		449,	//10-19
		2349,	//20-29
		6236,	//30-39
		8689,	//40-49
		9737,	//50-59
		9399,	//60-69
		6949,	//70-79
		4177,	//80-89
		1068	//90+
	};

	// Total reported severe: 49637

	uint32_t reported_mild_per_age[AGE_CATS_N] = {
		9925,	//0-9
		23304,	//10-19
		78272,	//20-29
		108144,	//30-39
		85087,	//40-49
		48573,	//50-59
		18245,	//60-69
		2865,	//70-79
		0,	//80-89, was 0
		273	//90+, was 273
	};

	// Total reported mild: 376915

	uint32_t reported_confirmed_per_age[AGE_CATS_N];
	/* = {
		10748,	//0-9
		23922,	//10-19
		81316,	//20-29
		116424,	//30-39
		96881,	//40-49
		62547,	//50-59
		32554,	//60-69
		14136,	//70-79
		6671+2000,	//80-89
		1933+500	//90+
	};*/

/*	uint32_t reported_deaths_per_age[AGE_CATS_N] = {
		25,	//0-9
		40,	//10-19
		161,	//20-29
		584,	//30-39
		1211,	//40-49
		2372,	//50-59
		3814,	//60-69
		3988,	//70-79
		3033,	//80-89
		870	//90+
	};*/

	uint32_t reported_uti_deaths_per_age[AGE_CATS_N] = {
		19,	//0-9
		29,	//10-19
		132,	//20-29
		400,	//30-39
		840,	//40-49
		1598,	//50-59
		2610,	//60-69
		2709,	//70-79
		1692,	//80-89
		404	//90+
	};

	uint32_t reported_infirmary_deaths_per_age[AGE_CATS_N] = {
		7,	//0-9
		15,	//10-19
		56,	//20-29
		238,	//30-39
		549,	//40-49
		1109,	//50-59
		1865,	//60-69
		2020,	//70-79
		1812,	//80-89
		614	//90+
	};

	// total deaths: 18718

	double ratio_critical_per_age[AGE_CATS_N];
	double ratio_severe_per_age[AGE_CATS_N];
	double ratio_mild_per_age[AGE_CATS_N];
//	double ratio_deaths_per_age[AGE_CATS_N];
	double ratio_asymptomatic_per_age[AGE_CATS_N];
	double ratio_reported_per_age[AGE_CATS_N];

	double probability_critical_death_per_age[AGE_CATS_N];
	double probability_severe_death_per_age[AGE_CATS_N];

	double probability_asymp_per_age[AGE_CATS_N];
	double probability_mild_per_age[AGE_CATS_N];
	double probability_severe_per_age[AGE_CATS_N];
	double probability_critical_per_age[AGE_CATS_N];

	for (uint32_t i=0; i<AGE_CATS_N; i++) {
		//C_ASSERT(reported_confirmed_per_age[i] == (reported_critical_per_age[i]+reported_severe_per_age[i]+reported_mild_per_age[i]))
		reported_confirmed_per_age[i] =
			reported_critical_per_age[i] +
			reported_severe_per_age[i] +
			reported_mild_per_age[i];
	}

	/*
		we need to manipulate this a bit to get the same amount during the simulation
	*/

	for (uint32_t i=0; i<AGE_CATS_N; i++) {
		reported_critical_per_age[i] = multiply_int_by_double(reported_critical_per_age[i], 1.1);
		reported_severe_per_age[i] = multiply_int_by_double(reported_severe_per_age[i], 1.1);

		reported_uti_deaths_per_age[i] = multiply_int_by_double(reported_uti_deaths_per_age[i], 0.9);
		probability_severe_death_per_age[i] = multiply_int_by_double(probability_severe_death_per_age[i], 0.9);
	}
	
	for (uint32_t i=0; i<AGE_CATS_N; i++) {
		int32_t v;

		v = static_cast<int32_t>(reported_confirmed_per_age[i])
			- static_cast<int32_t>(reported_critical_per_age[i] + reported_severe_per_age[i]);

		if (i <= 7) {
			C_ASSERT (v > 500);
			reported_mild_per_age[i] = v;
		}
		else if (i == 8) {
			if (v < 2000) {
				// here we force some infections, since the official reported data is wrong and show too few mild cases
				reported_mild_per_age[i] = 2000;

				reported_confirmed_per_age[i] =
					reported_critical_per_age[i] +
					reported_severe_per_age[i] +
					reported_mild_per_age[i];
			}
			else
				reported_mild_per_age[i] = v;
		}
		else if (i == 9) {
			if (v < 500) {
				// here we force some infections, since the official reported data is wrong and show too few mild cases
				reported_mild_per_age[i] = 500;

				reported_confirmed_per_age[i] =
					reported_critical_per_age[i] +
					reported_severe_per_age[i] +
					reported_mild_per_age[i];
			}
			else
				reported_mild_per_age[i] = v;
		}
	}

	for (uint32_t i=0; i<AGE_CATS_N; i++) {
		C_ASSERT(reported_confirmed_per_age[i] == (reported_critical_per_age[i]+reported_severe_per_age[i]+reported_mild_per_age[i]))
	}

	cprintf("\n");

	uint32_t total_death_severe = 0;
	uint32_t total_death_critical = 0;

	for (uint32_t i=0; i<AGE_CATS_N; i++) {
		probability_critical_death_per_age[i] = static_cast<double>(reported_uti_deaths_per_age[i]) / static_cast<double>(reported_critical_per_age[i]);
		probability_severe_death_per_age[i] = static_cast<double>(reported_infirmary_deaths_per_age[i]) / static_cast<double>(reported_severe_per_age[i]);

		total_death_severe += reported_infirmary_deaths_per_age[i];
		total_death_critical += reported_uti_deaths_per_age[i];
	}

	for (uint32_t i=0; i<AGE_CATS_N; i++) {
		ratio_critical_per_age[i] = (double)reported_critical_per_age[i] / (double)reported_confirmed_per_age[i];
		ratio_severe_per_age[i] = (double)reported_severe_per_age[i] / (double)reported_confirmed_per_age[i];
		ratio_mild_per_age[i] = (double)reported_mild_per_age[i] / (double)reported_confirmed_per_age[i];
		//ratio_deaths_per_age[i] = (double)reported_deaths_per_age[i] / (double)reported_confirmed_per_age[i];
	}

	cprintf("Total reported deths: %u\n", (total_death_severe+total_death_critical));

	cprintf("\n");

	uint32_t total_confirmed = 0;

	for (uint32_t i=0; i<AGE_CATS_N; i++)
		total_confirmed += reported_confirmed_per_age[i];

	uint32_t total_critical = 0;
	for (uint32_t i=0; i<AGE_CATS_N; i++) {
		total_critical += reported_critical_per_age[i];
		cprintf("ratio_critical_per_age[%s]: %.4f\n", critical_per_age_str(i), ratio_critical_per_age[i]);
	}
	cprintf("Total reported critical: %u    Mean critical: %.4f\n", total_critical, (double)total_critical / (double)total_confirmed);

	cprintf("\n");

	uint32_t total_severe = 0;
	for (uint32_t i=0; i<AGE_CATS_N; i++) {
		total_severe += reported_severe_per_age[i];
		cprintf("ratio_severe_per_age[%s]: %.4f\n", critical_per_age_str(i), ratio_severe_per_age[i]);
	}
	cprintf("Total reported severe: %u    Mean severe: %.4f\n", total_severe, (double)total_severe / (double)total_confirmed);

	cprintf("\n");

	uint32_t total_mild = 0;
	for (uint32_t i=0; i<AGE_CATS_N; i++) {
		total_mild += reported_mild_per_age[i];
		cprintf("ratio_mild_per_age[%s]: %.4f\n", critical_per_age_str(i), ratio_mild_per_age[i]);
	}
	cprintf("Total reported mild: %u    Mean mild: %.4f\n", total_mild, (double)total_mild / (double)total_confirmed);

	cprintf("\n");

	//for (uint32_t i=0; i<AGE_CATS_N; i++)
	//	cprintf("ratio_deaths_per_age[%s]: %.4f\n", critical_per_age_str(i), ratio_deaths_per_age[i]);

	/* asymp(age) + mild(age) + severe(age) + critical(age) = 1.0
	   
	   asymp(age) + mild(age) = 1.0 - severe(age) - critical(age)

	   mild(age) + severe(age) + critical(age) = reported(age)

	   mild(age) = reported(age) / (severe(age) + critical(age))
	   

	   we wiil consider that the probability of being asymp is directly proportional of being mild
	   we need to do this because the equation has 2 degrees of freedom

	   we calculate the asymp ratio such that the average asymp ratio will be our global asymp ratio
	*/

	cprintf("\n");

/*	double weights_asymp[] = {
		25,	//0-9
		25,	//10-19
		25,	//20-29
		25,	//30-39
		25,	//40-49
		25,	//50-59
		25,	//60-69
		25,	//70-79
		25,	//80-89
		25	//90+
	};*/

	//double *weight = weights_asymp;
	double *weight = ratio_mild_per_age;

	uint64_t n = 0;
	for (uint32_t i=0; i<AGE_CATS_N; i++) {
		n += people_per_age_cat[i];
		cprintf("people_per_age_cat[%s]: " PU64 "  %.4f\n", critical_per_age_str(i), people_per_age_cat[i], weight[i]);
	}

	C_ASSERT(population.size() == n)

/*	adjust_weights_to_fit_mean<double, uint64_t, AGE_CATS_N> (
		weight,
		people_per_age_cat,
		cfg->probability_asymptomatic * (double)population.size(),
		ratio_asymptomatic_per_age
		);*/

	adjust_weights_to_fit_mean<double, uint64_t, AGE_CATS_N> (
		weight,
		people_per_age_cat,
		cfg->probability_asymptomatic * (double)population.size(),
		ratio_asymptomatic_per_age
		);

	cprintf("\n");

	for (uint32_t i=0; i<AGE_CATS_N; i++)
		ratio_reported_per_age[i] = 1.0 - ratio_asymptomatic_per_age[i];

	for (uint32_t i=0; i<AGE_CATS_N; i++)
		cprintf("ratio_asymptomatic_per_age[%s]: %.4f   (reported %.4f)\n", critical_per_age_str(i), ratio_asymptomatic_per_age[i], ratio_reported_per_age[i]);

	for (uint32_t i=0; i<AGE_CATS_N; i++) {
		ratio_critical_per_age[i] *= ratio_reported_per_age[i];
		ratio_severe_per_age[i] *= ratio_reported_per_age[i];
		ratio_mild_per_age[i] *= ratio_reported_per_age[i];
//		ratio_mild_per_age[i] = 1.0 - (ratio_critical_per_age[i] + ratio_severe_per_age[i] + ratio_asymptomatic_per_age[i]);
		//ratio_deaths_per_age[i] *= ratio_reported_per_age[i];
	}

/*	cprintf("\n");

	cprintf("age     mild    sev     crit             table reported\n");

	for (uint32_t i=0; i<AGE_CATS_N; i++) {
		cprintf("%-5s:  %.4f  %.4f  %.4f  %.4f\n", critical_per_age_str(i), ratio_mild_per_age[i] / ratio_reported_per_age[i], ratio_severe_per_age[i] / ratio_reported_per_age[i], ratio_critical_per_age[i] / ratio_reported_per_age[i],
			ratio_mild_per_age[i] / ratio_reported_per_age[i] + ratio_severe_per_age[i] / ratio_reported_per_age[i] + ratio_critical_per_age[i] / ratio_reported_per_age[i]);
	}*/

	/*
		transform the ratio considering
	*/

	double target_mild_prob = ((double)total_mild / (double)total_confirmed) * (1.0 - cfg->probability_asymptomatic);

	double target_severe_prob = ((double)total_severe / (double)total_confirmed) * (1.0 - cfg->probability_asymptomatic);

	double target_critical_prob = ((double)total_critical / (double)total_confirmed) * (1.0 - cfg->probability_asymptomatic);

	cprintf("\n");
	cprintf("target_mild_prob: %.4f\n", target_mild_prob);
	cprintf("target_severe_prob: %.4f\n", target_severe_prob);
	cprintf("target_critical_prob: %.4f\n", target_critical_prob);

	adjust_weights_to_fit_mean<double, uint64_t, AGE_CATS_N> (
		ratio_mild_per_age,
		people_per_age_cat,
		target_mild_prob * (double)population.size(),
		probability_mild_per_age
		);

	adjust_weights_to_fit_mean<double, uint64_t, AGE_CATS_N> (
		ratio_severe_per_age,
		people_per_age_cat,
		target_severe_prob * (double)population.size(),
		probability_severe_per_age
		);

	adjust_weights_to_fit_mean<double, uint64_t, AGE_CATS_N> (
		ratio_critical_per_age,
		people_per_age_cat,
		target_critical_prob * (double)population.size(),
		probability_critical_per_age
		);

	cprintf("\n");

	for (uint32_t i=0; i<AGE_CATS_N; i++) {
		//probability_critical_per_age[i] = (ratio_critical_per_age[i] * (double)population.size()) / (double)people_per_age_cat[i];
		cprintf("probability_mild_per_age[%s]: %.4f    ratio_mild_per_age[%s]: %.4f\n", critical_per_age_str(i), probability_mild_per_age[i], critical_per_age_str(i), ratio_mild_per_age[i]);
	}

	cprintf("\n");

	for (uint32_t i=0; i<AGE_CATS_N; i++) {
		//probability_critical_per_age[i] = (ratio_critical_per_age[i] * (double)population.size()) / (double)people_per_age_cat[i];
		cprintf("probability_severe_per_age[%s]: %.4f    ratio_severe_per_age[%s]: %.4f\n", critical_per_age_str(i), probability_severe_per_age[i], critical_per_age_str(i), ratio_severe_per_age[i]);
	}

	cprintf("\n");

	for (uint32_t i=0; i<AGE_CATS_N; i++) {
		//probability_critical_per_age[i] = (ratio_critical_per_age[i] * (double)population.size()) / (double)people_per_age_cat[i];
		cprintf("probability_critical_per_age[%s]: %.4f    ratio_critical_per_age[%s]: %.4f\n", critical_per_age_str(i), probability_critical_per_age[i], critical_per_age_str(i), ratio_critical_per_age[i]);
	}

	cprintf("\n");

/*	CMSG("ratio_asymptomatic_per_age:" << std::endl);

	for (uint32_t i=0; i<AGE_CATS_N; i++) {
		CMSG(critical_per_age_str(i) << ": " << ratio_asymptomatic_per_age[i] << std::endl);
	}*/

	cprintf("\n");

	cprintf("age     asymp   mild    sev     crit    global  death-s death-c\n");

	double t_asymp = 0.0, t_mild = 0.0, t_severe = 0.0, t_critical = 0.0;

	for (uint32_t i=0; i<AGE_CATS_N; i++) {
		probability_asymp_per_age[i] = 1.0 - (probability_mild_per_age[i] + probability_severe_per_age[i] + probability_critical_per_age[i]);

		t_asymp += probability_asymp_per_age[i] * (double)people_per_age_cat[i];		
		t_mild += probability_mild_per_age[i] * (double)people_per_age_cat[i];
		t_severe += probability_severe_per_age[i] * (double)people_per_age_cat[i];
		t_critical += probability_critical_per_age[i] * (double)people_per_age_cat[i];

		cprintf("%-5s:  %.4f  %.4f  %.4f  %.4f  %.4f  %.4f  %.4f\n",
			critical_per_age_str(i),
			probability_asymp_per_age[i],
			probability_mild_per_age[i],
			probability_severe_per_age[i],
			probability_critical_per_age[i],
			probability_asymp_per_age[i] + probability_mild_per_age[i] + probability_severe_per_age[i] + probability_critical_per_age[i],
			probability_severe_death_per_age[i],
			probability_critical_death_per_age[i]);
	}
	
	t_asymp /= (double)population.size();
	t_mild /= (double)population.size();
	t_severe /= (double)population.size();
	t_critical /= (double)population.size();

	cprintf("\n");
	cprintf("MEAN :  %.4f  %.4f  %.4f  %.4f\n", t_asymp, t_mild, t_severe, t_critical);
	cprintf("REP  :  %.4f  %.4f  %.4f  %.4f\n", 0.0, t_mild / (1.0 - cfg->probability_asymptomatic), t_severe / (1.0 - cfg->probability_asymptomatic), t_critical / (1.0 - cfg->probability_asymptomatic));

	cfg->probability_mild = t_mild;
	cfg->probability_severe = t_severe;
	cfg->probability_critical = t_critical;
	cfg->probability_asymptomatic = 1.0 - (cfg->probability_mild + cfg->probability_severe + cfg->probability_critical);

#if 0
	double asymp_test = 0.0;

	cfg->probability_mild = 0.0;
	cfg->probability_severe = 0.0;
	cfg->probability_critical = 0.0;

	for (uint32_t i=0; i<AGE_CATS_N; i++) {
		asymp_test += ratio_asymptomatic_per_age[i] * (double)people_per_age_cat[i];

		cfg->probability_mild += ratio_mild_per_age[i] * (double)people_per_age_cat[i];
		cfg->probability_severe += ratio_severe_per_age[i] * (double)people_per_age_cat[i];
		cfg->probability_critical += ratio_critical_per_age[i] * (double)people_per_age_cat[i];
//		cfg->probability_critical += ratio_critical_per_age[i] * (double)reported_confirmed_per_age[i];
	}

	asymp_test /= (double)population.size();
	cfg->probability_mild /= (double)population.size();
	cfg->probability_severe /= (double)population.size();
	cfg->probability_critical /= (double)population.size();
//	cfg->probability_critical /= total_confirmed;

	cfg->probability_asymptomatic = 1.0 - (cfg->probability_mild + cfg->probability_severe + cfg->probability_critical);

	cprintf("\n");

	cprintf("asymp_test: %.4f\n", asymp_test);

	cprintf("\n");

	cprintf("age     asymp   mild    sev     crit             table global\n");

	for (uint32_t i=0; i<AGE_CATS_N; i++) {
		cprintf("%-5s:  %.4f  %.4f  %.4f  %.4f  %.4f\n", critical_per_age_str(i), ratio_asymptomatic_per_age[i], ratio_mild_per_age[i], ratio_severe_per_age[i], ratio_critical_per_age[i], ratio_asymptomatic_per_age[i] + ratio_mild_per_age[i] + ratio_severe_per_age[i] + ratio_critical_per_age[i]);
	}
#endif

	cprintf("cfg->probability_asymptomatic: %.4f\n", cfg->probability_asymptomatic);
	cprintf("cfg->probability_mild: %.4f      (reported %.4f   original %.4f)\n", cfg->probability_mild, cfg->probability_mild / (1.0 - cfg->probability_asymptomatic), (double)total_mild / (double)total_confirmed);
	cprintf("cfg->probability_severe: %.4f      (reported %.4f   original %.4f)\n", cfg->probability_severe, cfg->probability_severe / (1.0 - cfg->probability_asymptomatic), (double)total_severe / (double)total_confirmed);
	cprintf("cfg->probability_critical: %.4f      (reported %.4f   original %.4f)\n", cfg->probability_critical, cfg->probability_critical / (1.0 - cfg->probability_asymptomatic), (double)total_critical / (double)total_confirmed);

	CMSG("death rate in icu:" << (static_cast<double>(total_death_critical) / static_cast<double>(total_critical)) << std::endl)
	CMSG("death rate in infirmary:" << (static_cast<double>(total_death_severe) / static_cast<double>(total_severe)) << std::endl)

	cprintf("setting up infection rates...\n");

	for (person_t *p: population) {
		p->setup_infection_probabilities(probability_mild_per_age[ p->age_cat() ], probability_severe_per_age[ p->age_cat() ], probability_critical_per_age[ p->age_cat() ]);
		p->set_death_probability_severe_in_hospital_per_cycle_step( probability_severe_death_per_age[ p->age_cat() ] );
		p->set_death_probability_critical_in_icu_per_cycle_step( probability_critical_death_per_age[ p->age_cat() ] );
	}

#ifdef SANITY_CHECK
{
	double mild_test = 0.0, severe_test = 0.0, critical_test = 0.0;
	double asymp_test = 0.0;

	for (person_t *p: population) {
		asymp_test += p->get_probability_asymptomatic();
		mild_test += p->get_probability_mild();
		severe_test += p->get_probability_severe();
		critical_test += p->get_probability_critical();
	}

	asymp_test /= (double)population.size();
	mild_test /= (double)population.size();
	severe_test /= (double)population.size();
	critical_test /= (double)population.size();

	cprintf("\n");

	cprintf("asymp_test: %.4f\n", asymp_test);
	cprintf("mild_test: %.4f\n", mild_test);
	cprintf("severe_test: %.4f\n", severe_test);
	cprintf("critical_test: %.4f\n", critical_test);
}
#endif
}
