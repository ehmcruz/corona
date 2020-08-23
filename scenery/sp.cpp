#include <corona.h>
#include <parser.h>

#include <random>
#include <algorithm>

#ifndef SCHOOL_STRATEGY
	#define SCHOOL_STRATEGY 0
#endif

static double sp_school_weight = 2.0;

#ifndef SP_CYCLES_TO_SIM
	#define SP_CYCLES_TO_SIM 360.0
#endif

#ifndef SP_CYCLE_TO_OPEN_SCHOOL
	#define SP_CYCLE_TO_OPEN_SCHOOL 210.0
#endif

static csv_ages_t *csv;

//static health_unit_t santa_casa_uti(100000, ST_CRITICAL);
//static health_unit_t santa_casa_enfermaria(20000000, ST_SEVERE);

static health_unit_t uti(1000000000, ST_CRITICAL);
static health_unit_t enfermaria(1000000000, ST_SEVERE);

static int32_t stages_green = 0;

#if SCHOOL_STRATEGY == 1 || SCHOOL_STRATEGY == 2
	static uint32_t sp_school_div = 3;
#endif

void sp_setup_infection_state_rate ();

void sp_create_school_relation_contingency (std::vector<person_t*>& students,
                                            uint32_t age_ini,
                                            uint32_t age_end,
                                            dist_double_t& dist_class_size,
                                            dist_double_t& dist_school_size,
                                            region_t *prof_region,
                                            dist_double_t& dist_prof_age,
                                            double intra_class_ratio,
                                            double inter_class_ratio,
                                            uint32_t room_div,
                                            report_progress_t *report);

void setup_cmd_line_args (boost::program_options::options_description& cmd_line_args)
{
	cmd_line_args.add_options()
		("schoolweight", boost::program_options::value<double>()->notifier( [] (double v) { sp_school_weight = v; } ), "School weight");
}

void cfg_t::scenery_setup ()
{

	DMSG("sp_school_weight: " << sp_school_weight << std::endl)
exit(0);
	this->network_type = NETWORK_TYPE_NETWORK;
	this->cycles_to_simulate = SP_CYCLES_TO_SIM;

	for (uint32_t r=RELATION_SCHOOL; r<=RELATION_SCHOOL_4; r++)
		this->relation_type_weights[r] = sp_school_weight;

	this->r0 = 3.0;

	this->probability_asymptomatic = 0.85;
	this->probability_mild = 0.809 * (1.0 - this->probability_asymptomatic);
	this->probability_critical = 0.044 * (1.0 - this->probability_asymptomatic);

	csv = new csv_ages_t((char*)"data/sp-grande-sp.csv");
	csv->dump();

	this->n_regions = csv->get_ncities();
//	this->n_regions = 1;

	DMSG("number of cities" << this->n_regions << std::endl);

#ifdef SP_RESULTS_FILE
	scenery_results_fname = "-";
	scenery_results_fname += SP_RESULTS_FILE;
#endif

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
		normal_double_dist_t dist_family_size(3.0, 1.0, 1.0, 10.0);
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
		double school_ratio = 0.9;
		normal_double_dist_t dist_school_class_size(30.0, 5.0, 10.0, 50.0);
		const_double_dist_t dist_school_size(2000.0);

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

		normal_double_dist_t dist_school_prof_age(40.0, 10.0, 25.0, 70.0);

		rname = this->get_name();
		rname += " school loading...";
		report_progress_t progress_school(rname.c_str(), students.size(), 10000);

	#if SCHOOL_STRATEGY == 0
		network_create_school_relation_v2(students,
		                                  age_ini,
		                                  age_end,
		                                  dist_school_class_size,
                                          dist_school_size,
                                          this,
                                          dist_school_prof_age,
                                          0.2,
                                          0.003,
                                          &progress_school);
	#elif SCHOOL_STRATEGY == 1 || SCHOOL_STRATEGY == 2
		sp_create_school_relation_contingency(students,
		                                  age_ini,
		                                  age_end,
		                                  dist_school_class_size,
                                          dist_school_size,
                                          this,
                                          dist_school_prof_age,
                                          0.2,
                                          0.003,
                                          sp_school_div,
                                          &progress_school);
	#endif
	}
}

void setup_inter_region_relations ()
{
	if (cfg->network_type == NETWORK_TYPE_NETWORK) {
		for (auto it=regions.begin(); it!=regions.end(); ++it) {
			for (auto jt=it+1; jt!=regions.end(); ++jt) {
				region_t *s, *t;
				uint64_t sn, tn;

				s = *it;
				t = *jt;

				if (s->get_name() == "SaoPaulo") {
					tn = (uint64_t)((double)t->get_npopulation() * 0.05);
					sn = tn;
				}
				else if (t->get_name() == "SaoPaulo") {
					sn = (uint64_t)((double)s->get_npopulation() * 0.05);
					tn = sn;
				}
				else {
					sn = (uint64_t)((double)s->get_npopulation() * 0.01);
					tn = (uint64_t)((double)t->get_npopulation() * 0.01);
				}

				if (tn < sn)
					sn = tn;
				else
					tn = sn;

				cprintf("creating links between cities %s-%s " PU64 "...\n", s->get_name().c_str(), t->get_name().c_str(), sn);
				
				network_create_inter_city_relation(s, t, sn, RELATION_UNKNOWN);
			}
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
		mask.set(RELATION_SCHOOL);
		mask.set(RELATION_SCHOOL_0);
		mask.set(RELATION_SCHOOL_1);
		mask.set(RELATION_SCHOOL_2);
		mask.set(RELATION_SCHOOL_3);
		mask.set(RELATION_SCHOOL_4);

		for (person_t *p: population) {
			if ((network_vertex_data(p).flags & mask).any())
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

	printf("r0 cycle %.2f: %.2f\n", current_cycle, get_affective_r0());
}

static void adjust_r_open_schools ()
{
	printf("r0 cycle %.2f: %.2f\n", current_cycle, get_affective_r0());
	printf("r0 cycle %.2f-student: %.2f\n", current_cycle, get_affective_r0( {RELATION_SCHOOL} ));

	cfg->relation_type_transmit_rate[RELATION_SCHOOL] = sp_school_weight * cfg->relation_type_transmit_rate[RELATION_UNKNOWN];

	printf("r0 cycle %.2f: %.2f\n", current_cycle, get_affective_r0());
	printf("r0 cycle %.2f-student: %.2f\n", current_cycle, get_affective_r0( {RELATION_SCHOOL} ));
}

void callback_before_cycle (double cycle)
{
	const uint64_t people_warmup = 500;
	const double warmup = 30.0;
	const double cycle_open_school = SP_CYCLE_TO_OPEN_SCHOOL;
	static uint32_t day = 0;
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
#if SCHOOL_STRATEGY == 0
	else if (cycle == cycle_open_school) {
		adjust_r_open_schools();
		stages_green++;
	}
#elif SCHOOL_STRATEGY == 1 || SCHOOL_STRATEGY == 2
	else if (cycle >= cycle_open_school && modf(cycle, &intpart) == 0.0) {
		dprintf("day = %u\n", day);

		uint32_t relation = day + RELATION_SCHOOL_0;

		cfg->relation_type_transmit_rate[RELATION_SCHOOL] = sp_school_weight * cfg->relation_type_transmit_rate[RELATION_UNKNOWN];

		for (uint32_t r=RELATION_SCHOOL_0; r<=RELATION_SCHOOL_4; r++)
			cfg->relation_type_transmit_rate[r] = 0.0;

		cfg->relation_type_transmit_rate[relation] = sp_school_weight * cfg->relation_type_transmit_rate[RELATION_UNKNOWN];
	
	#if SCHOOL_STRATEGY == 2
		uint32_t next = day + 1;

		if (next >= (sp_school_div))
			next = 0;

		uint32_t relation_next = next + RELATION_SCHOOL_0;

		cfg->relation_type_transmit_rate[relation_next] = sp_school_weight * cfg->relation_type_transmit_rate[RELATION_UNKNOWN];
	#endif

		cprintf("opening schools for students of day %i  type %s\n", day, relation_type_str(relation));
		
		day++;

		if (day >= (sp_school_div))
			day = 0;
	}
#endif
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
	n_students = get_n_population_per_relation_flag( {RELATION_SCHOOL} );

	cprintf("amount of professors: " PU64 "\n", n_profs);
	cprintf("amount of students: " PU64 "\n", n_students);

	for (i=0; i<NUMBER_OF_RELATIONS; i++) {
		cprintf("relation-%s: " PU64 "\n", relation_type_str(i), cfg->relation_type_number[i]);
	}

	cprintf("amount of school relations per student: %.2f\n", (double)cfg->relation_type_number[RELATION_SCHOOL] / (double)n_students);
}

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

struct sp_room_t {
	std::vector<person_t*> students;
	uint32_t size;
	uint32_t i;
	uint32_t room_i;
	person_t *prof;
};

static person_t* sp_find_professor (region_t *prof_region, dist_double_t& dist_prof_age)
{
	uint32_t prof_age = (uint32_t)dist_prof_age.generate();

	for (person_t *prof: prof_region->get_people()) {
		if (prof->get_age() == prof_age && network_vertex_data(prof).flags.test(RELATION_SCHOOL) == false) {
			network_vertex_data(prof).flags.set(VFLAG_PROFESSOR);
			network_vertex_data(prof).flags.set(RELATION_SCHOOL); // to prevent selecting the same professor twice
			return prof;
		}
	}

	C_ASSERT(false)

	return nullptr;
}

static void sp_create_school_relation_professor (std::vector<person_t*>& students, person_t *prof)
{
	network_create_connection_one_to_all(prof, students, RELATION_SCHOOL);
}

void sp_create_school_relation_contingency (std::vector<person_t*>& students,
                                            uint32_t age_ini,
                                            uint32_t age_end,
                                            dist_double_t& dist_class_size,
                                            dist_double_t& dist_school_size,
                                            region_t *prof_region,
                                            dist_double_t& dist_prof_age,
                                            double intra_class_ratio,
                                            double inter_class_ratio,
                                            uint32_t room_div,
                                            report_progress_t *report)
{
	std::vector<person_t*> school_students;

	uint32_t school_i = 0;
	uint32_t school_size = (uint32_t)dist_school_size.generate();

	std::vector<sp_room_t> class_students;
	
	school_students.resize( (uint32_t)dist_school_size.get_max(), nullptr );

	C_ASSERT(school_size <= school_students.size())
	
	class_students.resize( age_end + 1 );

	dprintf("total amount of students to go to school: " PU64 "\n", students.size());

	for (uint32_t age=age_ini; age<=age_end; age++) {
		class_students[age].students.resize( (uint32_t)dist_class_size.get_max(), nullptr );
		class_students[age].size = (uint32_t)dist_class_size.generate();
		class_students[age].i = 0;
		class_students[age].room_i = 0;

		C_ASSERT(class_students[age].size <= class_students[age].students.size())

		//for (uint32_t i=0; i<class_students[age].size(); i++)
		//	class_students[age][i] = nullptr;

		//class_ocupancy[age].first = 0;     // target amount of students
		//class_ocupancy[age].second = 0;    // current amount of students
	}

	for (person_t *p: students) {
		uint32_t age = p->get_age();

		C_ASSERT(age >= age_ini && age <= age_end)

		C_ASSERT(class_students[age].i < class_students[age].size)
		C_ASSERT(school_i < school_size)

		C_ASSERT(class_students[age].i < class_students[age].students.size())
		C_ASSERT(school_i < school_students.size())

		class_students[age].students[ class_students[age].i++ ] = p;
		school_students[ school_i++ ] = p;

		if (report != nullptr)
			report->check_report(1);

		if (class_students[age].i == class_students[age].size || school_i == school_size) {
		#ifdef SANITY_ASSERT
		{
			bool was_null = false;
			for (auto p: class_students[age].students) {
				if (was_null == false) {
					was_null = (p == nullptr);
				}
				else {
					C_ASSERT(p == nullptr)
				}
			}
		}
		#endif

//			dprintf("created class room with %u students\n", class_students[age].i);

			C_ASSERT(class_students[age].room_i < room_div)

			uint32_t school_rtype = static_cast<uint32_t>(RELATION_SCHOOL_0) + class_students[age].room_i;

			if (class_students[age].room_i == 0)
				class_students[age].prof = sp_find_professor(prof_region, dist_prof_age);

			network_create_connection_between_people(class_students[age].students, static_cast<relation_type_t>(school_rtype), intra_class_ratio);

#if 0
network_print_population_graph( {RELATION_SCHOOL} );
for (auto p: class_students[age].students) if (p) dprintf("%i, ", p->get_id());
dprintf("       %.4f\n", intra_class_ratio);
panic("in\n");
#endif

			sp_create_school_relation_professor(class_students[age].students, class_students[age].prof);

			class_students[age].room_i++;

			if (class_students[age].room_i == room_div)
				class_students[age].room_i = 0;

#if 0
network_print_population_graph( {RELATION_SCHOOL} );
for (auto p: class_students[age].students) if (p) dprintf("%i, ", p->get_id());
dprintf("\n");
panic("in\n");
#endif

			for (auto& p: class_students[age].students)
				p = nullptr;

			class_students[age].size = (uint32_t)dist_class_size.generate();
			class_students[age].i = 0;

			C_ASSERT(class_students[age].size <= class_students[age].students.size())

			if (school_i == school_size) {
				//dprintf("created school with %u students\n", school_i);
				network_create_connection_between_people(school_students, RELATION_SCHOOL, inter_class_ratio);

				for (auto& p: school_students)
					p = nullptr;

				school_i = 0;
				school_size = (uint32_t)dist_school_size.generate();

				C_ASSERT(school_size <= school_students.size())
			}
		}
	}

	for (uint32_t age=age_ini; age<=age_end; age++) {
		if (class_students[age].i > 0) {
//			dprintf("created class room with %u students\n", class_students[age].i);
			C_ASSERT(class_students[age].room_i < room_div)

			uint32_t school_rtype = static_cast<uint32_t>(RELATION_SCHOOL_0) + class_students[age].room_i;

			if (class_students[age].room_i == 0)
				class_students[age].prof = sp_find_professor(prof_region, dist_prof_age);
			
			network_create_connection_between_people(class_students[age].students, static_cast<relation_type_t>(school_rtype), intra_class_ratio);
			
			sp_create_school_relation_professor(class_students[age].students, class_students[age].prof);
		}
	}

	if (school_i > 0) {
		//dprintf("created school with %u students\n", school_i);
		network_create_connection_between_people(school_students, RELATION_SCHOOL, inter_class_ratio);
	}
}