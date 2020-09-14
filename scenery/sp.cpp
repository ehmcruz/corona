#include <corona.h>
#include <parser.h>

#include <random>
#include <algorithm>
#include <numeric>

#define X_SP_SCHOOL_STRATEGY   \
	X_SP_SCHOOL_STRATEGY_(SCHOOL_CLOSED) \
	X_SP_SCHOOL_STRATEGY_(SCHOOL_OPEN_33) \
	X_SP_SCHOOL_STRATEGY_(SCHOOL_OPEN_66) \
	X_SP_SCHOOL_STRATEGY_(SCHOOL_OPEN_100) \
	X_SP_SCHOOL_STRATEGY_(SCHOOL_OPEN_PLANNED)

enum sp_school_strategy_t {
	#define X_SP_SCHOOL_STRATEGY_(S) S,
	X_SP_SCHOOL_STRATEGY
	#undef X_SP_SCHOOL_STRATEGY_
};

static const char* sp_school_strategy_str (sp_school_strategy_t s)
{
	static const char *list[] = {
		#define X_SP_SCHOOL_STRATEGY_(S) #S,
		X_SP_SCHOOL_STRATEGY
		#undef X_SP_SCHOOL_STRATEGY_
	};

	return list[s];
}

enum class sp_plan_t {
	phase_0,
	phase_33,
	phase_66,
	phase_100
};

static sp_plan_t sp_plan = sp_plan_t::phase_0;

static sp_school_strategy_t sp_school_strategy = SCHOOL_CLOSED;

static double sp_school_weight = 2.0;

static double sp_cycle_to_open_school = 210.0;

static double sp_cycles_between_phases = 7.0 * 6.0; // 6 weeks default

static double sp_ratio_student_intra_class_contingency = 0.2;

static const uint32_t sp_school_div = 3;

static double vaccine_cycle = 999999.0; // default no vaccine

static double vaccine_immune_cycles = 14.0;

static double vaccine_immunity_rate = 0.9;

static uint32_t vaccine_per_cycle = 300000;

static csv_ages_t *csv;

//static health_unit_t santa_casa_uti(100000, ST_CRITICAL);
//static health_unit_t santa_casa_enfermaria(20000000, ST_SEVERE);

static health_unit_t uti(1000000000, ST_CRITICAL);
static health_unit_t enfermaria(1000000000, ST_SEVERE);

static int32_t stages_green = 0;

void sp_setup_infection_state_rate ();
void sp_configure_school ();

void setup_cmd_line_args (boost::program_options::options_description& cmd_line_args)
{
	cmd_line_args.add_options()
		("schoolweight,w", boost::program_options::value<double>()->notifier( [] (double v) {
				sp_school_weight = v;
			} ), "School weight")
		("schoolopencycle,o", boost::program_options::value<double>()->notifier( [] (double v) {
				sp_cycle_to_open_school = v;
			} ), "Cycle to open schools")
		("schoolcyclesbetweenphases,b", boost::program_options::value<double>()->notifier( [] (double v) {
				sp_cycles_between_phases = v;
			} ), "Cycles between phases of SP plan to open schools")
		("vaccinecycle,v", boost::program_options::value<double>()->notifier( [] (double v) {
				vaccine_cycle = v;
			} ), "Cycle to start the vaccines")
		("schoolstrat,s", boost::program_options::value<std::string>()->notifier( [] (std::string v) {
				if (v == "0")
					sp_school_strategy = SCHOOL_CLOSED;
				else if (v == "33")
					sp_school_strategy = SCHOOL_OPEN_33;
				else if (v == "66")
					sp_school_strategy = SCHOOL_OPEN_66;
				else if (v == "100")
					sp_school_strategy = SCHOOL_OPEN_100;
				else if (v == "planned")
					sp_school_strategy = SCHOOL_OPEN_PLANNED;
				else {
					CMSG("schoolstrat should be 0, 33, 66, 100 or planned" << std::endl)
					exit(1);
				}
			} ), "School opening strategy, should be 0, 33, 66, 100 or planned")
		("schoolnewintraprob,p", boost::program_options::value<double>()->notifier( [] (double v) {
				sp_ratio_student_intra_class_contingency = v;
			} ), "For partial school opening strategies (33, 66 and planned), the new probability of 2 students of the same class to interact with each other");
}

void cfg_t::scenery_setup ()
{
	C_ASSERT(sp_school_weight >= 0.0)

	this->network_type = NETWORK_TYPE_NETWORK;
	this->cycles_to_simulate = 360.0;

	for (uint32_t r=RELATION_SCHOOL; r<=RELATION_SCHOOL_4; r++) {
		this->relation_type_weights[r] = 2.0;

		this->factor_per_relation_group[r][ST_MILD] = 0.0;
		this->factor_per_relation_group[r][ST_SEVERE] = 0.0;
		this->factor_per_relation_group[r][ST_CRITICAL] = 0.0;
	}

	this->r0 = 3.0;

	this->probability_asymptomatic = 0.85;
	this->probability_mild = 0.809 * (1.0 - this->probability_asymptomatic);
	this->probability_critical = 0.044 * (1.0 - this->probability_asymptomatic);

	csv = new csv_ages_t((char*)"data/sp-grande-sp.csv");
	csv->dump();

	this->n_regions = csv->get_ncities();
//	this->n_regions = 1;

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

	vaccine_cycle += vaccine_immune_cycles;
	vaccine_per_cycle /= this->cycle_division;

	DMSG("sp_school_weight: " << sp_school_weight << std::endl)
	DMSG("sp_school_strategy: " << sp_school_strategy_str(sp_school_strategy) << std::endl)
	DMSG("sp_cycle_to_open_school: " << sp_cycle_to_open_school << std::endl)
	DMSG("sp_cycles_between_phases: " << sp_cycles_between_phases << std::endl)
	DMSG("sp_ratio_student_intra_class_contingency: " << sp_ratio_student_intra_class_contingency << std::endl)
	DMSG("vaccine_cycle: " << vaccine_cycle << std::endl)
}

void region_t::setup_population ()
{
	uint32_t i, n;

	std::string name;

	name = csv->get_city_name( this->get_id() );
	//name = "SaoPaulo";

	this->set_name(name);

	this->set_population_number( csv->get_population(this->get_name()) );

	cprintf("%s total de habitantes: " PU64 "\n", this->get_name().c_str(), this->get_npopulation());

	for (i=csv->get_first_age(); i<=csv->get_last_age(); i++) {
		n = csv->get_population_per_age(this->get_name(), i);
		//cprintf("%s habitantes idade %i -> %i\n", this->get_name().c_str(), i, n);
		this->add_people(n, i);
	}
	//exit(1);

//	if (this->get_name() == "Paranavai")
//		this->track_stats();
}

void region_t::setup_health_units ()
{
//sp_setup_infection_state_rate(); exit(1);
	this->add_health_unit( &uti );
	this->add_health_unit( &enfermaria );
}

void region_t::setup_relations ()
{
	if (cfg->network_type == NETWORK_TYPE_NETWORK) {
		gamma_double_dist_t dist_family_size(3.26, 1.69, 1.0, 10.0);
		normal_double_dist_t dist_number_random_connections(20.0, 5.0, 5.0, 100.0);

		dprintf("creating families for city %s...\n", this->get_name().c_str());
		this->create_families(dist_family_size);

		dprintf("creating random connections for city %s...\n", this->get_name().c_str());

		std::string rname(this->get_name());
		rname += " random loading...";
		report_progress_t progress_random(rname.c_str(), this->get_npopulation(), 10000);

		this->create_random_connections(dist_number_random_connections, RELATION_UNKNOWN, &progress_random);
//return;
		dprintf("creating schools for city %s...\n", this->get_name().c_str());

		std::vector<person_t*> students;
		uint32_t age_ini = 4;
		uint32_t age_end = 18;
		double school_ratio = 0.778;
		const_double_dist_t dist_school_size(2000.0);
		normal_double_dist_t dist_school_class_size(30.0, 5.0, 10.0, 50.0);

		struct school_per_age_t {
			uint32_t age;
			uint32_t n, i;
		};

		std::vector<school_per_age_t> v;
		v.resize(age_end+1);

		for (uint32_t age=age_ini; age<=age_end; age++) {
			v[age].n = (uint32_t)((double)this->get_region_people_per_age(age) * school_ratio);
			v[age].age = age;
			v[age].i = 0;
		}

		students.reserve(this->people.size());

		std::vector<person_t*> people_ = this->people;

		std::shuffle(people_.begin(), people_.end(), rgenerator);

		for (person_t *p: people_) {
			uint32_t age = p->get_age();

			if (age >= age_ini && age <= age_end && v[age].i < v[age].n) {
				students.push_back(p);
				v[age].i++;
			}
		}

		gamma_double_dist_t dist_school_prof_age(41.15, 9.87, 20.0, 70.0);

		rname = this->get_name();
		rname += " school loading...";
		report_progress_t progress_school(rname.c_str(), students.size(), 10000);

		if (sp_school_strategy == SCHOOL_OPEN_100 || sp_school_strategy == SCHOOL_CLOSED) {
			network_create_school_relation(students,
			                                  age_ini,
			                                  age_end,
			                                  dist_school_class_size,
	                                          dist_school_size,
	                                          this,
	                                          dist_school_prof_age,
	                                          0.5,
	                                          0.002,
	                                          RELATION_SCHOOL,
	                                          RELATION_SCHOOL,
	                                          RELATION_SCHOOL,
	                                          &progress_school);
		}
		else if (sp_school_strategy == SCHOOL_OPEN_33 || sp_school_strategy == SCHOOL_OPEN_66 || sp_school_strategy == SCHOOL_OPEN_PLANNED) {
			network_create_school_relation(students,
			                                  age_ini,
			                                  age_end,
			                                  dist_school_class_size,
	                                          dist_school_size,
	                                          this,
	                                          dist_school_prof_age,
	                                          0.5,
	                                          0.002,
	                                          RELATION_SCHOOL,
	                                          RELATION_SCHOOL,
	                                          RELATION_SCHOOL_4,
	                                          &progress_school);
		}
	}
}

void setup_inter_region_relations ()
{
	double sp_total_mobility_ratio = 0.178;
	double sp_mobility_ratio_to_capital = 0.9;

	struct moving_people_t {
		uint32_t weight;
		uint32_t value;
	};

	std::vector<moving_people_t> moving_people;

	moving_people.resize(cfg->n_regions);

	if (cfg->network_type == NETWORK_TYPE_NETWORK) {
		for (auto it=regions.begin(); it!=regions.end(); ++it) {
			region_t *s = *it;

			for (auto jt=regions.begin(); jt!=regions.end(); ++jt) {
				region_t *t = *jt;

				moving_people[ t->get_id() ].weight = t->get_npopulation();
			}

			moving_people[ s->get_id() ].weight = 0;

			if (s->get_name() == "SaoPaulo") {
				adjust_values_to_fit_mean(moving_people,
						sp_total_mobility_ratio * static_cast<double>(s->get_npopulation()));
			}
			else {
				moving_people[ region_t::get("SaoPaulo")->get_id() ].weight = 0;

				adjust_values_to_fit_mean(moving_people,
						sp_total_mobility_ratio * (1.0 - sp_mobility_ratio_to_capital) * static_cast<double>(s->get_npopulation()));

				moving_people[ region_t::get("SaoPaulo")->get_id() ].value =
					static_cast<uint32_t>( sp_total_mobility_ratio * sp_mobility_ratio_to_capital * static_cast<double>(s->get_npopulation()) );
			}

			for (auto jt=regions.begin(); jt!=regions.end(); ++jt) {
				region_t *t = *jt;
				uint32_t n;

				if (unlikely(s == t))
					continue;

				n = moving_people[t->get_id()].value;

				CMSG("creating links between cities " << s->get_name() << "-" << t->get_name() << ": " << n << std::endl);
				
				network_create_inter_city_relation(s, t, n, RELATION_UNKNOWN);
			}

			uint32_t total = 0;

			std::for_each(moving_people.begin(), moving_people.end(), [&total] (auto& v) {
				total += v.value;
			});

			uint32_t total_test = static_cast<uint32_t>( sp_total_mobility_ratio * static_cast<double>(s->get_npopulation()) );

			DMSG("sum of " << s->get_name() << ": " << total << " test " << total_test << std::endl)
		}
	}
//exit(1);
}

void setup_extra_relations ()
{
	if (cfg->network_type == NETWORK_TYPE_NETWORK) {
		stats_zone_t *zone = create_new_stats_zone();
		zone->get_name() = "school";

		std::bitset<NUMBER_OF_FLAGS> mask;
		for (uint32_t r=RELATION_SCHOOL; r<=RELATION_SCHOOL_4; r++)
			mask.set(r);

		for (person_t *p: population) {
			if ((network_vertex_data(p).flags & mask).any())
				zone->add_person(p);
		}

		zone = create_new_stats_zone();
		zone->get_name() = "professors";

		for (person_t *p: population) {
			if (network_vertex_data(p).flags.test(VFLAG_PROFESSOR))
				zone->add_person(p);
		}
	}

	sp_setup_infection_state_rate();
}

static double backup_school_r0;

static void adjust_r_no_school (double target_r0)
{
	double family_r0, unknown_r0, factor;

	backup_school_r0 = cfg->relation_type_transmit_rate[RELATION_SCHOOL];

	for (uint32_t r=RELATION_SCHOOL; r<=RELATION_SCHOOL_4; r++)
		cfg->relation_type_transmit_rate[r] = 0.0;

	//target_r0 /= cfg->cycles_contagious;
	//target_r0 *= (double)population.size();

	/*
	f*others + family = target
	f*others = (target-family)
	*/

	family_r0 = cfg->relation_type_transmit_rate[RELATION_FAMILY] * (double)cfg->relation_type_number[RELATION_FAMILY];
	family_r0 *= cfg->cycles_contagious->get_expected();
	family_r0 *= cfg->cycle_division;
	family_r0 /= (double)population.size();

	unknown_r0 = cfg->relation_type_transmit_rate[RELATION_UNKNOWN] * (double)cfg->relation_type_number[RELATION_UNKNOWN];
	unknown_r0 *= cfg->cycles_contagious->get_expected();
	unknown_r0 *= cfg->cycle_division;
	unknown_r0 /= (double)population.size();

	printf("r0 cycle %.2f family_r0: %.2f\n", current_cycle, family_r0);
	printf("r0 cycle %.2f unknown_r0: %.2f\n", current_cycle, unknown_r0);
	
	factor = (target_r0 - family_r0) / unknown_r0;

	cfg->relation_type_transmit_rate[RELATION_UNKNOWN] *= factor;

	unknown_r0 = cfg->relation_type_transmit_rate[RELATION_UNKNOWN] * (double)cfg->relation_type_number[RELATION_UNKNOWN];
	unknown_r0 *= cfg->cycles_contagious->get_expected();
	unknown_r0 *= cfg->cycle_division;
	unknown_r0 /= (double)population.size();

	printf("r0 cycle %.2f unknown_r0: %.2f\n", current_cycle, unknown_r0);

	printf("r0 cycle %.2f: %.2f\n", current_cycle, get_affective_r0_fast());
}

static void adjust_r_open_schools ()
{
	printf("r0 cycle %.2f: %.2f\n", current_cycle, get_affective_r0_fast());
	printf("r0 cycle %.2f-student: %.2f\n", current_cycle, get_affective_r0( {RELATION_SCHOOL} ));

	cfg->relation_type_transmit_rate[RELATION_SCHOOL] = sp_school_weight * cfg->relation_type_transmit_rate[RELATION_UNKNOWN];

	printf("r0 cycle %.2f: %.2f\n", current_cycle, get_affective_r0_fast());
	printf("r0 cycle %.2f-student: %.2f\n", current_cycle, get_affective_r0( {RELATION_SCHOOL} ));
}

static void check_vaccine (double cycle)
{
	if (cycle >= vaccine_cycle) {
		static bool first = false;
		static std::vector<person_t*>::iterator it;
		static std::vector<person_t*> pop;

		uint32_t i;

		if (!first) {
			first = true;

			pop = population;
			std::shuffle(pop.begin(), pop.end(), rgenerator);
			it = pop.begin();
		}

		i = 0;
		while (it != pop.end()) {
			C_ASSERT(i <= vaccine_per_cycle)

			if (unlikely(i == vaccine_per_cycle))
				break;

			person_t *p = *it;

			i += p->take_vaccine();

			++it;
		}
	}
}

void callback_before_cycle (double cycle)
{
	const uint64_t people_warmup = 500;
	const double warmup = 30.0;
	static uint32_t day = 0;
	static double sp_plan_next_cycle_target;
	double intpart;

	// target is 49000 infected after 47 days

	if (cycle < warmup) {
		static bool test = false;

		uint32_t summon_per_cycle = (uint64_t)((double)people_warmup / (warmup * cfg->cycle_division));

dprintf("cycle %.2f summon_per_cycle %u\n", cycle, summon_per_cycle);

		for (uint32_t i=0; i<summon_per_cycle; ) {
			person_t *p = pick_random_person();

			if (p->get_state() == ST_HEALTHY) {
				p->force_infect();
				i++;
			}
		}

//printf("r0 cycle 0-student: %.2f\n", get_affective_r0( {RELATION_SCHOOL} ));
		if (!test) {
			test = true;
			stages_green++;
		}
	}
	else if (cycle == warmup) {
//		cfg->global_r0_factor = 1.05;
		printf("r0 cycle %.2f: %.2f\n", cycle, get_affective_r0());
		adjust_r_no_school(1.5);
//		backup = cfg->relation_type_transmit_rate[RELATION_SCHOOL];
//		cfg->relation_type_transmit_rate[RELATION_SCHOOL] = 0.0;
//		cfg->global_r0_factor = 0.9 / (network_get_affective_r0_fast() / cfg->global_r0_factor);
//		cfg->global_r0_factor = 0.9 / cfg->r0;
//printf("r0 cycle 30: %.2f\n", get_affective_r0());
		stages_green++;
	}
	else if (cycle == 45.0) {
		adjust_r_no_school(1.28);
		//cfg->global_r0_factor = 1.15 / (network_get_affective_r0_fast() / cfg->global_r0_factor);
		//cfg->global_r0_factor = 1.16 / cfg->r0;
//printf("r0 cycle 51: %.2f\n", get_affective_r0());
		stages_green++;
	}
	else if (sp_school_strategy == SCHOOL_OPEN_100 && cycle == sp_cycle_to_open_school) {
		adjust_r_open_schools();
		stages_green++;
	}
	else if ((sp_school_strategy == SCHOOL_OPEN_33 || sp_school_strategy == SCHOOL_OPEN_66) && cycle >= sp_cycle_to_open_school && modf(cycle, &intpart) == 0.0) {
		static bool school_configured = false;

		if (!school_configured) {
			school_configured = true;
			sp_configure_school();
		}

		dprintf("day = %u\n", day);

		for (uint32_t r=RELATION_SCHOOL; r<=RELATION_SCHOOL_4; r++)
			cfg->relation_type_transmit_rate[r] = 0.0;

		cfg->relation_type_transmit_rate[RELATION_SCHOOL] = sp_school_weight * cfg->relation_type_transmit_rate[RELATION_UNKNOWN];
		cfg->relation_type_transmit_rate[RELATION_SCHOOL_4] = sp_school_weight * cfg->relation_type_transmit_rate[RELATION_UNKNOWN];

		uint32_t relation = day + RELATION_SCHOOL_0;

		cfg->relation_type_transmit_rate[relation] = sp_school_weight * cfg->relation_type_transmit_rate[RELATION_UNKNOWN];
	
		if (sp_school_strategy == SCHOOL_OPEN_66) {
			uint32_t next = day + 1;

			if (next >= (sp_school_div))
				next = 0;

			uint32_t relation_next = next + RELATION_SCHOOL_0;

			cfg->relation_type_transmit_rate[relation_next] = sp_school_weight * cfg->relation_type_transmit_rate[RELATION_UNKNOWN];
		}

		cprintf("opening schools for students of day %i  type %s\n", day, relation_type_str(relation));
		
		day++;

		if (day >= (sp_school_div))
			day = 0;
	}
	else if ((sp_school_strategy == SCHOOL_OPEN_PLANNED) && cycle >= sp_cycle_to_open_school && modf(cycle, &intpart) == 0.0) {
		static bool school_configured = false;

		if (!school_configured) {
			school_configured = true;
			sp_configure_school();
		}

		for (uint32_t r=RELATION_SCHOOL; r<=RELATION_SCHOOL_4; r++)
			cfg->relation_type_transmit_rate[r] = 0.0;

		cfg->relation_type_transmit_rate[RELATION_SCHOOL] = sp_school_weight * cfg->relation_type_transmit_rate[RELATION_UNKNOWN];
		cfg->relation_type_transmit_rate[RELATION_SCHOOL_4] = sp_school_weight * cfg->relation_type_transmit_rate[RELATION_UNKNOWN];

		uint32_t relation = day + RELATION_SCHOOL_0;

		switch (sp_plan) {
			case sp_plan_t::phase_0:
				sp_plan_next_cycle_target = cycle + sp_cycles_between_phases;
				sp_plan = sp_plan_t::phase_33;
			break;

			case sp_plan_t::phase_33:
				DMSG("SP school phase 33 type " << relation_type_str(relation) << std::endl)

				cfg->relation_type_transmit_rate[relation] = sp_school_weight * cfg->relation_type_transmit_rate[RELATION_UNKNOWN];

				if (cycle >= sp_plan_next_cycle_target) {
					sp_plan_next_cycle_target = cycle + sp_cycles_between_phases;
					sp_plan = sp_plan_t::phase_66;
				}
			break;

			case sp_plan_t::phase_66: {
				cfg->relation_type_transmit_rate[relation] = sp_school_weight * cfg->relation_type_transmit_rate[RELATION_UNKNOWN];
			
				uint32_t next = day + 1;

				if (next >= (sp_school_div))
					next = 0;

				uint32_t relation_next = next + RELATION_SCHOOL_0;

				DMSG("SP school phase 66 type " << relation_type_str(relation) << " and " << relation_type_str(relation_next) << std::endl)

				cfg->relation_type_transmit_rate[relation_next] = sp_school_weight * cfg->relation_type_transmit_rate[RELATION_UNKNOWN];

				if (cycle >= sp_plan_next_cycle_target) {
					sp_plan_next_cycle_target = cycle + sp_cycles_between_phases;
					sp_plan = sp_plan_t::phase_100;
				}
			}
			break;

			case sp_plan_t::phase_100:
				DMSG("SP school phase 100" << std::endl)

				for (uint32_t r=RELATION_SCHOOL_0; r<=RELATION_SCHOOL_3; r++)
					cfg->relation_type_transmit_rate[r] = sp_school_weight * cfg->relation_type_transmit_rate[RELATION_UNKNOWN];
			break;

			default:
				C_ASSERT(false)
		}

		day++;

		if (day >= (sp_school_div))
			day = 0;
	}

	check_vaccine(cycle);
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

	uint64_t n_relations = 0;

	for (uint32_t r=RELATION_SCHOOL; r<=RELATION_SCHOOL_4; r++)
		n_relations += cfg->relation_type_number[r];

	cprintf("amount of school relations per student: %.2f\n", (double)n_relations / (double)n_students);
}

/**************************************************************/

void sp_setup_infection_state_rate ()
{
	uint32_t reported_critical_per_age[AGE_CATS_N] = {
		198,	//0-9
		146,	//10-19
		617,	//20-29
		1776,	//30-39
		2677,	//40-49
		3667,	//50-59
		4147,	//60-69
		3652,	//70-79
		2136,	//80-89
		498	//90+
	};

	// Total reported critical: 19514

	uint32_t reported_severe_per_age[AGE_CATS_N] = {
		498,	//0-9
		377,	//10-19
		2034,	//20-29
		5573,	//30-39
		7585,	//40-49
		8494,	//50-59
		8093,	//60-69
		5925,	//70-79
		3589,	//80-89
		939	//90+
	};

	// Total reported severe: 43107

	uint32_t reported_mild_per_age[AGE_CATS_N] = {
		7912,	//0-9
		18605,	//10-19
		64751,	//20-29
		91021,	//30-39
		71768,	//40-49
		39903,	//50-59
		14768,	//60-69
		2428,	//70-79
		3000,	//80-89 - was 0
		900	//90+ - was 0
	};

	// Total reported mild: 315056

	uint32_t reported_confirmed_per_age[AGE_CATS_N] = {
		8608,	//0-9
		19128,	//10-19
		67402,	//20-29
		98370,	//30-39
		82030,	//40-49
		52064,	//50-59
		27008,	//60-69
		12005,	//70-79
		5725+3000,	//80-89
		1437+900	//90+
	};

	uint32_t reported_deaths_per_age[AGE_CATS_N] = {
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
	};

	double ratio_critical_per_age[AGE_CATS_N];
	double ratio_severe_per_age[AGE_CATS_N];
	double ratio_mild_per_age[AGE_CATS_N];
	double ratio_deaths_per_age[AGE_CATS_N];
	double ratio_asymptomatic_per_age[AGE_CATS_N];
	double ratio_reported_per_age[AGE_CATS_N];

	double probability_asymp_per_age[AGE_CATS_N];
	double probability_mild_per_age[AGE_CATS_N];
	double probability_severe_per_age[AGE_CATS_N];
	double probability_critical_per_age[AGE_CATS_N];

	cprintf("\n");

	for (uint32_t i=0; i<AGE_CATS_N; i++) {
		ratio_critical_per_age[i] = (double)reported_critical_per_age[i] / (double)reported_confirmed_per_age[i];
		ratio_severe_per_age[i] = (double)reported_severe_per_age[i] / (double)reported_confirmed_per_age[i];
		ratio_mild_per_age[i] = (double)reported_mild_per_age[i] / (double)reported_confirmed_per_age[i];
		ratio_deaths_per_age[i] = (double)reported_deaths_per_age[i] / (double)reported_confirmed_per_age[i];
	}

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

	for (uint32_t i=0; i<AGE_CATS_N; i++)
		cprintf("ratio_deaths_per_age[%s]: %.4f\n", critical_per_age_str(i), ratio_deaths_per_age[i]);

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
		ratio_deaths_per_age[i] *= ratio_reported_per_age[i];
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

	cprintf("age     asymp   mild    sev     crit             table global\n");

	double t_asymp = 0.0, t_mild = 0.0, t_severe = 0.0, t_critical = 0.0;

	for (uint32_t i=0; i<AGE_CATS_N; i++) {
		probability_asymp_per_age[i] = 1.0 - (probability_mild_per_age[i] + probability_severe_per_age[i] + probability_critical_per_age[i]);

		t_asymp += probability_asymp_per_age[i] * (double)people_per_age_cat[i];		
		t_mild += probability_mild_per_age[i] * (double)people_per_age_cat[i];
		t_severe += probability_severe_per_age[i] * (double)people_per_age_cat[i];
		t_critical += probability_critical_per_age[i] * (double)people_per_age_cat[i];

		cprintf("%-5s:  %.4f  %.4f  %.4f  %.4f  %.4f\n", critical_per_age_str(i), probability_asymp_per_age[i], probability_mild_per_age[i], probability_severe_per_age[i], probability_critical_per_age[i], probability_asymp_per_age[i] + probability_mild_per_age[i] + probability_severe_per_age[i] + probability_critical_per_age[i]);
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

	cprintf("setting up infection rates...\n");

	for (person_t *p: population) {
		p->setup_infection_probabilities(probability_mild_per_age[ p->age_cat() ], probability_severe_per_age[ p->age_cat() ], probability_critical_per_age[ p->age_cat() ]);
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

/**************************************************************/

static void sp_reconfigure_class_room (std::vector<person_t*>& school_people, std::vector<person_t*>::iterator& it_begin, std::vector<person_t*>::iterator& it_end, uint32_t& rm)
{
	uint32_t n_students = it_end - it_begin;

	//DMSG("sp_reconfigure_class_room: " << (network_vertex_data(*it_begin).school_class_room) << " students " << n_students << std::endl)

	if (n_students < 10)  // the class room is already small, no need to reconfigure
		return;

	// remove all intra class relations

	for (auto ita=it_begin; ita!=it_end; ++ita) {
		for (auto itb=ita+1; itb!=it_end; ++itb) {
			if (network_check_if_people_are_neighbors(*ita, *itb)) {
				network_delete_edge(*ita, *itb);
				rm++;
//				DMSG("delete edge between " << ((*ita)->get_id()) << " and " << ((*itb)->get_id()) << std::endl)
			}
		}
	}

	// find professor

	std::vector<person_t*>::iterator it_prof = school_people.end();

	for (auto it=it_begin; it!=it_end; ++it) {
		if (network_vertex_data(*it).flags.test(VFLAG_PROFESSOR))
			it_prof = it;
	}

	C_ASSERT(it_prof != school_people.end())

	// move professor to the head

	person_t *tmp = *it_begin;
	person_t *prof = *it_prof;

	*it_begin = prof;
	*it_prof = tmp;

	C_ASSERT(*it_begin == prof)

	// re-create connections between students

	uint32_t new_room_size = n_students / 3;

	network_create_connection_between_people(it_begin+1, it_begin+new_room_size, RELATION_SCHOOL_0, sp_ratio_student_intra_class_contingency);
	network_create_connection_between_people(it_begin+new_room_size, it_begin+new_room_size*2, RELATION_SCHOOL_1, sp_ratio_student_intra_class_contingency);
	network_create_connection_between_people(it_begin+new_room_size*2, it_end, RELATION_SCHOOL_2, sp_ratio_student_intra_class_contingency);

	// re-create connections for professor

	network_create_connection_one_to_all(prof, it_begin+1, it_begin+new_room_size, RELATION_SCHOOL_0);
	network_create_connection_one_to_all(prof, it_begin+new_room_size, it_begin+new_room_size*2, RELATION_SCHOOL_1);
	network_create_connection_one_to_all(prof, it_begin+new_room_size*2, it_end, RELATION_SCHOOL_2);
}

void sp_configure_school ()
{
	DMSG("reconfiguring schools" << std::endl)

	// first find how many school people we have

	uint32_t n_school_people = 0;

	for (person_t *p: population) {
		if (network_vertex_data(p).school_class_room != UNDEFINED32)
			n_school_people++;
	}

	if (n_school_people < 2)
		return;

	DMSG("found " << n_school_people << " students" << std::endl)

	// now create a vector with all school people

	std::vector<person_t*> school_people;
	
	school_people.reserve(n_school_people);

	for (person_t *p: population) {
		if (network_vertex_data(p).school_class_room != UNDEFINED32)
			school_people.push_back(p);
	}

	C_ASSERT(school_people.size() == n_school_people)

	// now sort people acording to their class room

	std::sort(school_people.begin(), school_people.end(), [] (person_t *a, person_t *b) -> bool {
		return (network_vertex_data(a).school_class_room < network_vertex_data(b).school_class_room);
	});

	// now identify each class room

//	for (uint32_t r=RELATION_SCHOOL; r<=RELATION_SCHOOL_4; r++)
//		cfg->relation_type_transmit_rate[r] = 2.0 * cfg->relation_type_transmit_rate[RELATION_UNKNOWN];

	printf("r0 cycle %.2f: %.2f\n", current_cycle, get_affective_r0_fast());
//	printf("r0 cycle %.2f-student: %.2f\n", current_cycle, get_affective_r0( {RELATION_SCHOOL} ));

	uint32_t rm, count, count_other_rooms;

	std::bitset<NUMBER_OF_FLAGS> mask;
	for (uint32_t r=RELATION_SCHOOL; r<=RELATION_SCHOOL_4; r++)
		mask.set(r);
	
	count = 0;
	count_other_rooms = 0;
	network_iterate_over_edges ([&count, &count_other_rooms, &mask] (pop_vertex_t s, pop_vertex_t t, pop_edge_t e) {
		count += (mask.test(network_edge_data(e).type));
		count_other_rooms += (network_edge_data(e).type == RELATION_SCHOOL_4);
	});

	DMSG("there are " << count << " school relations, with other rooms: " << count_other_rooms << std::endl)

	auto it = school_people.begin() + 1;
	auto it_begin = school_people.begin();

	rm = 0;

	// reconfigure class rooms

	while (true) {
		if (it == school_people.end()) {
			sp_reconfigure_class_room(school_people, it_begin, it, rm);
			break;
		}
		else if (network_vertex_data(*it).school_class_room != network_vertex_data(*it_begin).school_class_room) {
			sp_reconfigure_class_room(school_people, it_begin, it, rm);
			it_begin = it;
		}

		++it;
	}

	DMSG("removed " << rm << " school intra-class relations" << std::endl)

	rm = 0;

	// reduce amount of inter class relations to 1/3 of the original

	std::vector<pop_edge_t> edges_to_rm;
	edges_to_rm.reserve(count_other_rooms);

	network_iterate_over_edges ([&edges_to_rm, &rm] (pop_vertex_t s, pop_vertex_t t, pop_edge_t e) {
		if (network_edge_data(e).type == RELATION_SCHOOL_4 && roll_dice(0.66)) {
			edges_to_rm.push_back(e);
			rm++;
		}
	});

	for (pop_edge_t e: edges_to_rm)
		network_delete_edge(e);

	DMSG("removed " << rm << " school inter-class relations" << std::endl)

	count = 0;
	count_other_rooms = 0;
	network_iterate_over_edges ([&count, &count_other_rooms, &mask] (pop_vertex_t s, pop_vertex_t t, pop_edge_t e) {
		count += (mask.test(network_edge_data(e).type));
		count_other_rooms += (network_edge_data(e).type == RELATION_SCHOOL_4);
	});

	DMSG("after re-organizing, there are " << count << " school relations, with other rooms: " << count_other_rooms << std::endl)

	printf("r0 cycle %.2f: %.2f\n", current_cycle, get_affective_r0_fast());
	//printf("r0 cycle %.2f-student: %.2f\n", current_cycle, get_affective_r0( {RELATION_SCHOOL} ));
//exit(1);
}
